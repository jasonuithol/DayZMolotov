// Molotov test bed (loads @Molotov, see mods.txt). Players spawn on an empty stretch of
// the east-coast beach (BEACH) with molotovs, their ingredients and lighters at their feet,
// and a haystack, a lumber pile and a stack of wooden pallets laid out along the beach
// ahead of them. Every (re)spawn lays out a fresh kit.
// The headless self-test (-missiontest) also uses the Chernogorsk quay (SPOT): a fuelled
// hatchback and boat, fireplaces, infected and the nearest map pump, hay, shed, lumber
// and straw stack.

void main()
{
	Hive ce = CreateHive();
	if (ce)
		ce.InitOffline();

	GetGame().GetWorld().SetDate(2026, 9, 20, 10, 0);
}

class CustomMission: MissionServer
{
	static const vector SPOT = "7572 0 2855";
	static const vector ALONG = "0.48 0 0.877";   // along the quay, towards the north-east
	//! open beach on the east coast, 250 m from the nearest building (found by -beachprobe)
	static const vector BEACH = "13480 0 5700";
	protected bool m_Test;
	protected vector m_BeachAlong;   //!< along the shoreline
	protected vector m_BeachLand;    //!< away from the sea
	protected PileOfWoodenPlanks m_BeachLumber;
	protected string m_TreeShape;
	protected vector m_SpreadTo;
	protected string m_BushShape;

	//! Everything that burns, laid out along the beach ahead of the spawn in three rows (4,
	//! 18 and 32 m inland), each 12 m clear of the next so one fire doesn't light another. Map meshes are spawned straight from
	//! their models; the lumber pile is a CE event in the wild, so it's an entity
	static const string M = "dz\\structures\\";
	static const ref array<string> BEACH_MODELS = {
		M + "industrial\\farms\\farm_strawstack.p3d",
		M + "residential\\misc\\misc_haystack.p3d",
		M + "industrial\\misc\\misc_haybale.p3d",
		M + "industrial\\misc\\misc_haybale_decayed.p3d",
		"dz\\structures_bliss\\industrial\\misc\\misc_haybale_packed.p3d",
		"dz\\structures_bliss\\industrial\\misc\\misc_haybale_packed_stack.p3d",
		M + "industrial\\misc\\misc_pallets3.p3d",
		"PileOfWoodenPlanks",
		M + "residential\\sheds\\shed_w1.p3d",
		M + "residential\\sheds\\shed_w2.p3d",
		M + "residential\\sheds\\shed_w3.p3d",
		M + "residential\\sheds\\shed_w4.p3d",
		M + "residential\\sheds\\shed_w5.p3d",
		M + "residential\\sheds\\shed_w6.p3d",
		M + "residential\\misc\\misc_feedrack.p3d",
		M + "residential\\misc\\misc_feedshack.p3d",
		M + "residential\\misc\\misc_deerstand1.p3d",
		M + "residential\\misc\\misc_deerstand2.p3d",
		M + "residential\\houses\\house_1w01.p3d",
		M + "residential\\houses\\house_1w05.p3d",
		M + "residential\\houses\\house_2w01.p3d",
		M + "residential\\houses\\house_2w03.p3d"
	};
	static const float BEACH_GAP = 12;
	static const int BOATS = 6;
	static const float VEHICLE_GAP = 1.5;
	static const ref array<string> VEHICLES = {"OffroadHatchback", "Hatchback_02", "CivilianSedan", "Sedan_02", "Offroad_02", "Truck_01_Covered", "Motorbike_01_Red", "Motorbike_02_Blue"};
	static const float BOAT_APART = 3.2;   //!< side by side: about a metre between them

	void LayBeach()
	{
		vector b = Ground(BEACH);
		m_BeachLand = "1 0 0";
		for (int a = 0; a < 16; a++)
		{
			float ang = a * Math.PI2 / 16;
			vector dir = Vector(Math.Cos(ang), 0, Math.Sin(ang));
			vector q = b + dir * 60;
			if (GetGame().SurfaceIsSea(q[0], q[2]))
			{
				m_BeachLand = -dir;
				break;
			}
		}
		m_BeachAlong = Vector(m_BeachLand[2], 0, -m_BeachLand[0]);

		// three rows, the next thing going in whichever row is shortest
		float rowEnd[3] = {8, 8, 8};
		float rowIn[3] = {4, 18, 32};
		foreach (string what : BEACH_MODELS)
		{
			int row = 0;
			for (int r = 1; r < 3; r++)
			{
				if (rowEnd[r] < rowEnd[row])
					row = r;
			}
			float len = BeachPlace(what, b + m_BeachAlong * rowEnd[row] + m_BeachLand * rowIn[row]);
			rowEnd[row] = rowEnd[row] + len + BEACH_GAP;
		}
		CountCopies("after laying out");
		bool fresh = Census(b);
		if (!m_Test && fresh)
		{
			// one of every vanilla vehicle, nose to tail a short gap apart, along the beach
			// behind the spawn
			float behind = 12;
			for (int vi = 0; vi < VEHICLES.Count(); vi++)
			{
				vector front;
				vector rear;
				Length(VEHICLES[vi], front, rear);
				float fz = front[2];
				float rz = rear[2];
				SpawnCarAt(Ground(b - m_BeachAlong * (behind + fz) + m_BeachLand * 14), m_BeachAlong, VEHICLES[vi]);
				behind = behind + fz - rz + VEHICLE_GAP;
			}
			// and a line of boats side by side from the shore out to sea, for a chain reaction
			vector shore = Shore(b, -m_BeachLand);
			for (int bi = 0; bi < BOATS; bi++)
			{
				float seaward = BOAT_APART * bi;
				vector bp = shore - m_BeachLand * seaward;
				float bx = bp[0];
				float bz = bp[2];
				float floor = GetGame().SurfaceY(bx, bz);
				bp[1] = floor;
				float depth = GetGame().GetWaterDepth(bp);
				if (depth > 0)
					floor = floor + depth;
				bp[1] = floor;
				BoatScript boat = SpawnBoatAt(bp);
				if (boat)
					boat.SetDirection(m_BeachAlong);
			}
		}
		if (m_Test)
		{
			// two hay bales 4 m apart past the end of the rows: light one, the other should
			// catch from it (the self-test pins the spread chance to 1)
			vector sa = Ground(b + m_BeachAlong * 175 + m_BeachLand * 4);
			vector sb = Ground(b + m_BeachAlong * 179 + m_BeachLand * 4);
			Object ha = GetGame().CreateStaticObjectUsingP3D(M + "industrial\\misc\\misc_haybale.p3d", sa, "0 0 0");
			Object hb = GetGame().CreateStaticObjectUsingP3D(M + "industrial\\misc\\misc_haybale.p3d", sb, "0 0 0");
			if (ha && hb)
			{
				m_ThrowAt.Set("spread first", Ground(sa - m_BeachAlong * 1.5));
				m_SpreadTo = hb.GetPosition();
			}
		}
		// and the nearest real tree and shrub, for the self-test to burn
		array<Object> near = new array<Object>;
		GetGame().GetObjectsAtPosition(b, 200, near, null);
		float bestTree = 1e9;
		float bestBush = 1e9;
		foreach (Object n : near)
		{
			float nd = vector.Distance(n.GetPosition(), b);
			if (n.IsTree() && nd < bestTree)
			{
				bestTree = nd;
				m_StaticAt.Set("tree", n.GetPosition());
				m_ThrowAt.Set("tree", Ground(n.GetPosition() + "0.8 0 0"));
				m_TreeShape = n.GetShapeName();
			}
			else if (n.IsBush() && nd < bestBush)
			{
				bestBush = nd;
				m_StaticAt.Set("bush", n.GetPosition());
				m_ThrowAt.Set("bush", Ground(n.GetPosition() + "0.3 0 0"));
				m_BushShape = n.GetShapeName();
			}
		}
		Print("[Molotov] nearest tree " + m_TreeShape + " " + bestTree + " m, shrub " + m_BushShape + " " + bestBush + " m");
		Print("[Molotov] beach laid out at " + b + ", along " + m_BeachAlong + ": " + m_StaticAt.Count() + " things, rows " + rowEnd[0] + ", " + rowEnd[1] + " and " + rowEnd[2] + " m long");
	}

	//! Place one thing with its near end at start, long side along the beach; its length
	float BeachPlace(string what, vector start)
	{
		string key = what.Substring(what.LastIndexOf("\\") + 1, what.Length() - what.LastIndexOf("\\") - 1);
		if (what.IndexOf("\\") < 0)
		{
			vector at = Ground(start + m_BeachAlong * 3);
			PileOfWoodenPlanks pile = PileOfWoodenPlanks.Cast(GetGame().CreateObjectEx(what, at, ECE_PLACE_ON_SURFACE));
			if (!pile)
				return 0;
			pile.SetDirection(m_BeachAlong);
			m_BeachLumber = pile;
			m_StaticAt.Set("beach lumber", pile.GetPosition());
			m_ThrowAt.Set("beach lumber", Ground(at + m_BeachAlong * 3));
			return 6;
		}
		// Work out where it goes on a server-only copy, then spawn the real one there and leave
		// it be: clients only ever see where a map object was created, never a later move
		vector ori = Vector(Math.Atan2(m_BeachAlong[0], m_BeachAlong[2]) * Math.RAD2DEG, 0, 0);
		Object o = GetGame().CreateStaticObjectUsingP3D(what, Ground(start), ori * Math.DEG2RAD, 1.0, true);   // it takes radians
		if (!o)
		{
			Print("[Molotov] FAIL could not place " + what);
			return 0;
		}
		vector box[2];
		if (!o.GetCollisionBox(box))
		{
			Print("[Molotov] " + what + " has no collision box");
			GetGame().ObjectDelete(o);
			return 4;
		}
		vector lo = box[0];
		vector hi = box[1];
		// the long side along the beach: turn it if it's the model's x
		float len = hi[2] - lo[2];
		bool turned = hi[0] - lo[0] > len;
		if (turned)
		{
			len = hi[0] - lo[0];
			ori = ori + "90 0 0";
			o.SetOrientation(ori);
		}
		vector mid = (lo + hi) * 0.5;
		mid[1] = 0;
		// the terrain, not the road surface: that would be the floor of what was just spawned here
		vector centre = start + m_BeachAlong * (len * 0.5);
		centre[1] = GetGame().SurfaceY(centre[0], centre[2]);
		// model centre over that spot, bottom on the sand
		vector p = centre - (o.ModelToWorld(mid) - o.GetPosition());
		p[1] = centre[1] - lo[1];
		o.SetPosition(p);
		// throw from just past its far end
		vector end = mid;
		if (turned)
			end[0] = hi[0] + 1;
		else
			end[2] = hi[2] + 1;
		vector throwAt = Ground(o.ModelToWorld(end));
		GetGame().ObjectDelete(o);

		// the engine shifts a model it creates from where it was asked (by up to 3 m, up
		// too): measure the shift on another server-only copy and ask for that much less
		o = GetGame().CreateStaticObjectUsingP3D(what, p, ori * Math.DEG2RAD, 1.0, true);
		vector shift = "0 0 0";
		if (o)
		{
			shift = o.GetPosition() - p;
			GetGame().ObjectDelete(o);
		}
		o = GetGame().CreateStaticObjectUsingP3D(what, p - shift, ori * Math.DEG2RAD);
		if (!o)
		{
			Print("[Molotov] FAIL could not place " + what);
			return 0;
		}
		if (vector.Distance(o.GetPosition(), p) > 0.01)
			Print("[Molotov] " + key + " asked for " + p + ", landed at " + o.GetPosition());
		array<Object> here = new array<Object>;
		GetGame().GetObjectsAtPosition(o.GetPosition(), 2, here, null);
		int copies;
		foreach (Object h : here)
		{
			if (h.GetShapeName() == what)
				copies++;
		}
		if (copies != 1)
			Print("[Molotov] FAIL " + copies + " copies of " + key + " at " + p);
		m_StaticAt.Set("beach " + key, o.GetPosition());
		m_ThrowAt.Set("beach " + key, throwAt);
		vector tm[4];
		o.GetTransform(tm);
		Print("[MolotovTest] facing " + key + ": asked " + ori + ", GetOrientation " + o.GetOrientation() + ", GetDirection " + o.GetDirection() + ", transform x " + tm[0] + " y " + tm[1] + " z " + tm[2]);
		Print("[Molotov] placed " + key + " at " + o.GetPosition() + ", " + len + " m long, burns " + MolotovIgnition.IsFlammable(o) + ", box " + lo + " .. " + hi);
		return len;
	}

	// The kit, 3 of each: ready-made molotovs (crafting is tested) and lighters - "Light
	// the rag" with a lighter/matches in the inventory, then throw.
	// "GlassBottle+" = a bottle already filled with petrol, if crafting needs testing again.
	static const ref array<string> KIT = {
		"Molotov", "Molotov", "Molotov", "Molotov", "PetrolLighter", "Matchbox",
		"GlassBottle", "GlassBottle+", "Rag", "CanisterGasoline"
	};
	static const ref array<string> INFECTED = {
		"ZmbM_HermitSkinny_Beige", "ZmbF_CitizenANormal_Blue", "ZmbM_PatrolNormal_Autumn",
		"ZmbM_CitizenASkinny_Red", "ZmbF_JournalistNormal_White"
	};

	void CustomMission()
	{
		string value;
		m_Test = GetGame().CommandlineGetParam("missiontest", value);
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(LayBeach, 3000, false);
		if (m_Test)
		{
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(SpawnCar, 2000, false);
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(Setup, 3000, false);
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(SelfTest, 5000, false);
		}
	}

	//! Find the nearest fuel pump, hay, wooden shed and lumber pile; float a fuelled boat
	//! off the quay; lay a fireplace with firewood behind the spawn
	void Setup()
	{
		array<Object> far = new array<Object>;
		GetGame().GetObjectsAtPosition(Ground(SPOT), 2500, far, null);
		map<string, float> best = new map<string, float>;
		foreach (Object f : far)
		{
			string kind = "";
			string shape = f.GetShapeName();
			shape.ToLower();
			if (Land_FuelStation_Feed.Cast(f))
				kind = "pump";
			else if (f.GetType() == "PileOfWoodenPlanks")
				kind = "lumber";
			else if (f.GetType().IndexOf("Land_Shed_W") == 0)
				kind = "shed";
			else if (shape.Contains("strawstack"))
				kind = "straw";
			else if (shape.Contains("hay") && MolotovIgnition.IsFlammable(f))
				kind = "hay";
			if (kind == "")
				continue;
			float d = vector.Distance(f.GetPosition(), SPOT);
			if (!best.Contains(kind) || d < best.Get(kind))
			{
				best.Set(kind, d);
				m_Statics.Set(kind, f);
			}
		}
		foreach (string k, Object target : m_Statics)
		{
			m_StaticAt.Set(k, target.GetPosition());
			// 1 m past the end of its long side: the fire has to reach it from outside
			vector box[2];
			vector throwAt = target.GetPosition() + "1.5 0 0";
			if (target.GetCollisionBox(box))
			{
				vector lo = box[0];
				vector hi = box[1];
				vector mid = (lo + hi) * 0.5;
				if (hi[0] - lo[0] >= hi[2] - lo[2])
					mid[0] = hi[0] + 1;
				else
					mid[2] = hi[2] + 1;
				throwAt = target.ModelToWorld(mid);
			}
			m_ThrowAt.Set(k, Ground(throwAt));
			if (k == "straw" && target.GetCollisionBox(box))
			{
				vector shi = box[1];
				m_StrawTop = target.ModelToWorld(Vector(0, shi[1] + 0.3, 0));
			}
			Print("[Molotov] nearest " + k + ": " + target.GetType() + target.GetShapeName() + " at " + target.GetPosition() + ", " + best.Get(k) + " m away");
		}
		m_Pump = Land_FuelStation_Feed.Cast(m_Statics.Get("pump"));
		m_Lumber = PileOfWoodenPlanks.Cast(m_Statics.Get("lumber"));
		m_Statics.Clear();   // hay has no stable script object: keep positions only

		m_Boat = SpawnBoat(SPOT);

		// firewood but no kindling: the molotov is both lighter and kindling
		m_WoodFire = FireplaceBase.Cast(GetGame().CreateObjectEx("Fireplace", Ground(SPOT - ALONG * 14 + Side() * 9), ECE_PLACE_ON_SURFACE));
		if (m_WoodFire)
			m_WoodFire.GetInventory().CreateAttachment("Firewood");
		// sticks only, 4 m on: the molotov's rag goes in as the kindling
		FireplaceBase sticks = FireplaceBase.Cast(GetGame().CreateObjectEx("Fireplace", Ground(SPOT - ALONG * 14 + Side() * 13), ECE_PLACE_ON_SURFACE));
		if (sticks)
			sticks.GetInventory().CreateAttachment("WoodenStick");
	}

	protected PlayerBase m_Dummy;
	protected CarScript m_Car;
	protected PlayerBase m_Bystander;
	protected vector m_CarAt;
	protected float m_DummyHealth;
	protected float m_CarEngine;
	protected ZombieBase m_Zombie;
	protected Land_FuelStation_Feed m_Pump;
	protected BoatScript m_Boat;
	protected vector m_BoatAt;
	protected vector m_BikeAt;
	protected FireplaceBase m_WoodFire;
	protected ref map<string, Object> m_Statics = new map<string, Object>;   //!< only while searching: hay has no stable script object
	protected ref map<string, vector> m_StaticAt = new map<string, vector>;
	protected ref map<string, vector> m_ThrowAt = new map<string, vector>;   //!< just past the end of each site's collision box
	protected PileOfWoodenPlanks m_Lumber;
	protected vector m_StrawTop;        //!< on top of the straw stack: someone standing there must burn
	protected PlayerBase m_OnStraw;
	protected FireplaceBase m_ActionFire;   //!< sticks only, lit by a molotov hitting it
	protected ref array<EntityAI> m_CarLoad = new array<EntityAI>;

	//! -missiontest: kit out an identity-less dummy, drop a lit molotov on it and another
	//! on the car, check the fires and their damage, quit
	//! -burnprobe: list the burnable map objects (hay, woodpiles, wooden sheds) across the map
	//! and how the engine names them; the nearest of each kind to SPOT is a test target
	void BurnProbe()
	{
		map<string, int> counts = new map<string, int>;
		map<string, string> nearest = new map<string, string>;
		map<string, float> best = new map<string, float>;
		array<Object> objs = new array<Object>;
		for (int x = 500; x < 15500; x += 1000)
		{
			for (int z = 500; z < 15500; z += 1000)
			{
				objs.Clear();
				GetGame().GetObjectsAtPosition(Vector(x, GetGame().SurfaceY(x, z), z), 710, objs, null);
				foreach (Object o : objs)
				{
					string shape = o.GetShapeName();
					string key = o.GetType() + " | " + shape;
					string low = key;
					low.ToLower();
					if (low.IndexOf("hay") < 0 && low.IndexOf("woodpile") < 0 && low.IndexOf("shed_w") < 0 && low.IndexOf("toilet_dry") < 0 && low.IndexOf("guardshed") < 0 && low.IndexOf("pileofwoodenplanks") < 0)
						continue;
					counts.Set(key, counts.Get(key) + 1);
					float d = vector.Distance(o.GetPosition(), SPOT);
					if (!best.Contains(key) || d < best.Get(key))
					{
						best.Set(key, d);
						nearest.Set(key, o.GetPosition().ToString() + " debug=" + o.GetDebugNameNative());
					}
				}
			}
		}
		foreach (string k, int n : counts)
			Print("[MolotovProbe] " + n + " x " + k + " nearest " + best.Get(k) + " m at " + nearest.Get(k));
		GetGame().RequestExit(0);
	}

	//! -beachprobe: open beach along the south and east coasts - land just above the sea,
	//! the sea within 40 m, nothing at all within 35 m and no building within 500 m
	void BeachProbe()
	{
		Print("[MolotovProbe] beach probe start");
		array<Object> objs = new array<Object>;
		int found;
		int coast;
		int clear;
		for (float x = 1000; x < 15000 && found < 30; x += 40)
		{
			for (float z = 1800; z < 11000 && found < 30; z += 20)
			{
				if (x < 12000 && z > 2800)
					break;   // the south coast, then the east coast

				if (GetGame().SurfaceIsSea(x, z) || GetGame().SurfaceIsPond(x, z))
					continue;
				float y = GetGame().SurfaceY(x, z);
				if (y < 0.8 || y > 3)
					continue;
				bool sea = GetGame().SurfaceIsSea(x + 40, z) || GetGame().SurfaceIsSea(x - 40, z) || GetGame().SurfaceIsSea(x, z + 40) || GetGame().SurfaceIsSea(x, z - 40);
				if (!sea)
					continue;
				vector p = Vector(x, y, z);
				objs.Clear();
				coast++;
				GetGame().GetObjectsAtPosition(p, 20, objs, null);
				if (objs.Count() > 0)
					continue;
				clear++;
				objs.Clear();
				GetGame().GetObjectsAtPosition(p, 400, objs, null);
				float nearest = 400;
				string what = "";
				foreach (Object o : objs)
				{
					if (!o.IsBuilding() || o.IsTree() || o.IsBush() || o.IsRock())
						continue;
					float dd = vector.Distance(o.GetPosition(), p);
					if (dd < nearest)
					{
						nearest = dd;
						what = o.GetShapeName();
					}
				}
				if (nearest < 150)
					continue;
				Print("[MolotovProbe] beach " + p + " nearest building " + nearest + " m " + what);
				found++;
				z += 300;
			}
		}
		Print("[MolotovProbe] coast points " + coast + ", clear " + clear + ", found " + found);
		GetGame().RequestExit(0);
	}

	//! -hideprobe: what sticks on a map object - delete, move, scale, flags?
	Object MapHay(vector at, float r)
	{
		array<Object> objs = new array<Object>;
		GetGame().GetObjectsAtPosition(at, r, objs, null);
		foreach (Object o : objs)
		{
			string sh = o.GetShapeName();
			if (sh.Contains("misc_haystack"))
				return o;
		}
		return null;
	}

	static const ref array<vector> PROBE_HAYS = {"7803.625 7.722 3206.058", "7811.8125 9.908 3226.722", "7823.21875 9.152 3208.544", "7836.047 9.405 3210.552"};
	static const ref array<string> PROBE_OPS = {"delete", "move down 50 m", "clear flags", "scale 0.01"};

	void HideProbe()
	{
		GetGame().CreatePlayer(null, "SurvivorM_Mirek", "7818 9 3215", 0, "NONE");
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(HideProbeGo, 5000, false);
	}

	Object HayAt(vector at)
	{
		array<Object> objs = new array<Object>;
		GetGame().GetObjectsAtPosition(at, 1, objs, null);
		foreach (Object o : objs)
		{
			string sh = o.GetShapeName();
			if (sh.Contains("misc_haystack") && vector.Distance(o.GetPosition(), at) < 0.1)
				return o;
		}
		return null;
	}

	void HideProbeGo()
	{
		Rays("before");
		for (int i = 0; i < 4; i++)
		{
			Object o = HayAt(PROBE_HAYS[i]);
			if (!o)
			{
				Print("[MolotovProbe] no hay at " + PROBE_HAYS[i]);
				continue;
			}
			if (i == 0)
				GetGame().ObjectDelete(o);
			else if (i == 1)
			{
				o.SetPosition(o.GetPosition() - "0 50 0");
				o.Update();
			}
			else if (i == 2)
				o.ClearFlags(EntityFlags.VISIBLE | EntityFlags.SOLID, true);
			else
				o.SetScale(0.01);
		}
		Rays("right after");
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(Rays, 3000, false, "3 s on");
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(Rays, 20000, false, "20 s on");
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(HideProbe3, 22000, false);
	}

	void Rays(string when)
	{
		for (int i = 0; i < 4; i++)
		{
			vector at = PROBE_HAYS[i];
			vector from = at + "0 10 0";
			vector to = at - "0 5 0";
			vector hit;
			vector normal;
			int comp;
			set<Object> objs = new set<Object>;
			DayZPhysics.RaycastRV(from, to, hit, normal, comp, objs, null, null, false, false, ObjIntersectGeom);
			string what = "terrain";
			if (objs.Count() > 0)
				what = objs[0].GetShapeName();
			Object o = HayAt(at);
			string found = "none";
			if (o)
				found = "pos " + o.GetPosition() + " flags " + o.GetFlags() + " scale " + o.GetScale();
			Print("[MolotovProbe] " + when + " / " + PROBE_OPS[i] + ": ray hits " + what + "; query finds " + found);
		}
	}

	protected vector m_ProbeTree;
	protected string m_ProbeShape;
	void TreeLook(string when)
	{
		bool found = MolotovGone.Find(m_ProbeShape, m_ProbeTree) != null;
		Print("[MolotovProbe] " + when + ": query finds the tree " + found + "; physics ray down " + MolotovGone.PhysicsBelow(m_ProbeTree + "0 20 0", 40, null));
	}

	void HideProbe3()
	{
		GetGame().RequestExit(0);
	}

	void ListHay(string when)
	{
		array<Object> objs = new array<Object>;
		GetGame().GetObjectsAtPosition("7815 8 3215", 80, objs, null);
		foreach (Object o : objs)
		{
			string sh = o.GetShapeName();
			if (sh.Contains("misc_haystack"))
				Print("[MolotovProbe] " + when + ": hay at " + o.GetPosition() + " flags " + o.GetFlags() + " scale " + o.GetScale() + " todelete " + o.ToDelete());
		}
	}

	void SelfTest()
	{
		MolotovIgnition.s_ChanceOverride = 1;   // spreading always happens in the self-test
		MolotovGone.s_HotMs = 25000;            // and ruins cool within it

		string probe;
		if (GetGame().CommandlineGetParam("nearprobe", probe))
		{
			// -nearprobe=x,z: everything within 15 m of a spot, nearest first
			TStringArray xz = new TStringArray;
			probe.Split(",", xz);
			vector spot = Ground(Vector(xz[0].ToFloat(), 0, xz[1].ToFloat()));
			array<Object> nearObjs = new array<Object>;
			GetGame().GetObjectsAtPosition(spot, 15, nearObjs, null);
			foreach (Object no : nearObjs)
			{
				vector nbox[2];
				string box = "";
				if (no.GetCollisionBox(nbox))
					box = " box " + nbox[0] + " .. " + nbox[1];
				vector nclip[2];
				no.ClippingInfo(nclip);
				box = box + " clip " + nclip[0] + " .. " + nclip[1];
				Print("[MolotovProbe] " + vector.Distance(no.GetPosition(), spot) + " m: '" + no.GetType() + "' " + no.GetShapeName() + " at " + no.GetPosition() + box);
			}
			GetGame().RequestExit(0);
			return;
		}
		if (GetGame().CommandlineGetParam("treeprobe", probe))
		{
			// -treeprobe: does deleting a tree (no felling) stick?
			array<Object> tobjs = new array<Object>;
			GetGame().GetObjectsAtPosition(Ground(BEACH), 120, tobjs, null);
			foreach (Object tt : tobjs)
			{
				if (!tt.IsTree())
					continue;
				m_ProbeTree = tt.GetPosition();
				m_ProbeShape = tt.GetShapeName();
				Print("[MolotovProbe] tree " + m_ProbeShape + " at " + m_ProbeTree + ": deleting");
				GetGame().ObjectDelete(tt);
				break;
			}
			TreeLook("right after");
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(TreeLook, 3000, false, "3 s on");
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(TreeLook, 15000, false, "15 s on");
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(HideProbe3, 16000, false);
			return;
		}
		if (GetGame().CommandlineGetParam("hideprobe", probe))
		{
			HideProbe();
			return;
		}
		if (GetGame().CommandlineGetParam("beachprobe", probe))
		{
			BeachProbe();
			return;
		}
		if (GetGame().CommandlineGetParam("burnprobe", probe))
		{
			BurnProbe();
			return;
		}
		if (GetGame().CommandlineGetParam("bottlebox", probe))
		{
			EntityAI b = EntityAI.Cast(GetGame().CreateObjectEx("GlassBottle", Ground(SPOT) + "0 1 0", ECE_NONE));
			vector mm[2];
			b.GetCollisionBox(mm);
			Print("[MolotovBox] collision " + mm[0] + " .. " + mm[1]);
			vector cm[2];
			b.ClippingInfo(cm);
			Print("[MolotovBox] clipping " + cm[0] + " .. " + cm[1]);
			Print("[MolotovBox] centre of mass " + dBodyGetCenterOfMass(b) + " mem invView " + b.GetMemoryPointPos("invView") + " ce_center " + b.GetMemoryPointPos("ce_center"));
		}
		m_Dummy = PlayerBase.Cast(GetGame().CreatePlayer(null, "SurvivorM_Mirek", Ground(SPOT), 0, "NONE"));
		if (m_Dummy)
		{
			StartingEquipSetup(m_Dummy, false);
			m_Dummy.GetInventory().CreateInInventory("Jeans_Blue");
			m_Dummy.GetInventory().CreateInInventory("Hoodie_Blue");
			m_Dummy.GetInventory().CreateInInventory("WorkingBoots_Grey");
		}
		ListNear();
		m_Zombie = ZombieBase.Cast(GetGame().CreateObjectEx("ZmbM_PatrolNormal_Autumn", Ground(SPOT - ALONG * 15), ECE_PLACE_ON_SURFACE | ECE_INITAI | ECE_EQUIP));

		array<Object> objs = new array<Object>;
		GetGame().GetObjectsAtPosition(Ground(SPOT), 30, objs, null);
		foreach (Object o : objs)
		{
			if (CarScript.Cast(o))
				m_Car = CarScript.Cast(o);
		}
		m_DummyHealth = m_Dummy.GetHealth("", "Health");
		if (m_Car)
			m_CarEngine = m_Car.GetHealth("Engine", "Health");
		DropLit(m_Dummy.GetPosition() + "0.5 3 0.5");
		if (m_Car)
			DropLit(m_Car.GetPosition() + "0 4 0");
		if (m_Zombie)
			DropLit(m_Zombie.GetPosition() + "0 2 0");
		// a motorbike across the road from the car, to fly apart
		m_BikeAt = Ground(SPOT + ALONG * 10 - Side() * 6);
		SpawnCarAt(m_BikeAt, ALONG, "Motorbike_02_Red");
		DropLit(m_BikeAt + "0 3 0");

		// the test sites Setup found: a fire beside each
		foreach (string k, vector at : m_ThrowAt)
			DropLit(at + "0 1 0");
		if (!m_Pump)
			Print("[MolotovTest] FAIL no fuel pump within 2.5 km");

		// a load in the car: what does going up do to it?
		if (m_Car)
		{
			array<string> loads = {"Hatchet", "BakedBeansCan", "TacticalBaconCan", "Rag", "WaterBottle"};
			foreach (string load : loads)
			{
				EntityAI item = m_Car.GetInventory().CreateInInventory(load);
				if (item)
					m_CarLoad.Insert(item);
			}
			Print("[MolotovTest] car fuel " + m_Car.GetFluidFraction(CarFluid.FUEL) + ", " + m_CarLoad.Count() + " items in its cargo");
			// someone standing beside it, out of the fire but not the blast
			m_CarAt = m_Car.GetPosition();
			m_Bystander = PlayerBase.Cast(GetGame().CreatePlayer(null, "SurvivorM_Mirek", Ground(m_CarAt + Side() * 5), 0, "NONE"));
		}

		if (m_StrawTop != vector.Zero)
		{
			m_OnStraw = PlayerBase.Cast(GetGame().CreatePlayer(null, "SurvivorM_Mirek", m_StrawTop, 0, "NONE"));
			if (m_OnStraw)
				Print("[MolotovTest] survivor on top of the straw stack at " + m_OnStraw.GetPosition());
		}
		if (m_Boat)
		{
			m_BoatAt = m_Boat.GetPosition();
			DropLit(m_Boat.GetPosition() + "0 3 0");
		}
		else
			Print("[MolotovTest] FAIL no boat");
		if (m_WoodFire)
			DropLit(m_WoodFire.GetPosition() + "0.8 2 0");
		ActionTest();
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(CheckFires, 10000, false);   // vehicles and pumps cook off for 3 s
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(CheckSpread, 5000, false);
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(CheckSpread, 20000, false);
	}

	void DropLit(vector at)
	{
		Molotov m = Molotov.Cast(GetGame().CreateObjectEx("Molotov", at, ECE_CREATEPHYSICS));
		if (!m)
		{
			Print("[MolotovTest] FAIL could not create a Molotov");
			return;
		}
		m.Light();
		dBodyActive(m, ActiveState.ACTIVE);
		m.ThrowPhysically(null, "0 -1 0");
		Print("[MolotovTest] lit=" + m.IsLit() + " dropped at " + at);
	}

	//! The second hay bale: not caught yet 5 s in (it takes a few seconds), caught by 20 s
	void CheckSpread()
	{
		if (m_SpreadTo != vector.Zero)
			Print("[MolotovTest] spread at " + (GetGame().GetTime() / 1000) + " s: the second hay bale has " + Infernos(m_SpreadTo, 1.5) + " big fires at it (want 0 at the first look, some at the second; chance pinned to 1)");
	}

	void CheckFires()
	{
		array<Object> objs = new array<Object>;
		GetGame().GetObjectsAtPosition(Ground(SPOT), 40, objs, null);
		int fires;
		int blasts;
		int molotovs;
		foreach (Object o : objs)
		{
			if (o.GetType() == "MolotovFire")
				fires++;
			else if (o.GetType() == "MolotovBlast")
				blasts++;
			else if (o.IsInherited(Molotov))
				molotovs++;
		}
		Print("[MolotovTest] fires burning near the spawn: " + fires + " (want 7: dummy, car, motorbike, infected, 2 fireplaces, boat), blasts: " + blasts + " (want 3: car, motorbike, boat), molotovs left: " + molotovs + " (want 13: 12 in the kit, an unlit one by the fireplaces)");
		float pinned = MolotovIgnition.s_ChanceOverride;
		MolotovIgnition.s_ChanceOverride = -1;
		Print("[MolotovTest] spread chance at the edge of a fire's reach: dry " + MolotovIgnition.SpreadChance(0) + " (want 0.35), light rain " + MolotovIgnition.SpreadChance(0.2) + " (want 0.175), heavy rain " + MolotovIgnition.SpreadChance(0.7) + " (want 0)");
		Print("[MolotovTest] spread chance touching the flames: dry " + MolotovIgnition.SpreadChance(0, 0) + " (want 0.8), half way " + MolotovIgnition.SpreadChance(0, 0.5) + " (want 0.575), light rain touching " + MolotovIgnition.SpreadChance(0.2, 0) + " (want 0.4)");
		MolotovIgnition.s_ChanceOverride = pinned;
		if (m_Pump)
			Print("[MolotovTest] pump ruined=" + m_Pump.IsRuined() + " (want 1)");
		foreach (string k, vector at : m_StaticAt)
			Print("[MolotovTest] " + k + ": big fires at it " + Infernos(at) + " (want 1, straw 3)");
		Print("[MolotovTest] boat gone=" + (m_Boat == null) + " (want 1), blasts where it was " + Infernos(m_BoatAt) + " (want 1)");
		array<Object> sunk = new array<Object>;
		GetGame().GetObjectsAtPosition(m_BoatAt, 40, sunk, null);
		foreach (Object so : sunk)
		{
			if (so.GetType().IndexOf("BetterExplosions_Boat") == 0 || so.GetType() == "MolotovBlast")
			{
				vector sp = so.GetPosition();
				Print("[MolotovTest] " + so.GetType() + " at " + sp + ", " + vector.Distance(sp, m_BoatAt) + " m from the boat, seabed " + GetGame().SurfaceY(sp[0], sp[2]) + ", sea " + GetGame().SurfaceGetSeaLevel() + " (want the hull and engine on the seabed, the engine some 10-15 m off; the blast on the sea)");
			}
		}
		if (m_WoodFire)
			Print("[MolotovTest] fireplace with firewood burning=" + m_WoodFire.IsBurning() + " (want 1)");
		if (m_ActionFire)
			Print("[MolotovTest] fireplace with sticks only, hit by a molotov: burning " + m_ActionFire.IsBurning() + " (want 1)");
		if (m_OnStraw)
			Print("[MolotovTest] survivor on the straw stack: health " + m_OnStraw.GetHealth("", "Health") + " (want under 100) at " + m_OnStraw.GetPosition());
		int ruined;
		int gone;
		string healths = "";
		foreach (EntityAI it : m_CarLoad)
		{
			if (!it)
			{
				gone++;
				continue;
			}
			if (it.IsRuined())
				ruined++;
			healths += " " + it.GetType() + "=" + it.GetHealth01("", "");
		}
		Print("[MolotovTest] car load after going up (want each 0.25 or less): " + ruined + " ruined, " + gone + " gone, of " + m_CarLoad.Count() + ":" + healths);
		int onGround;
		foreach (EntityAI dropped : m_CarLoad)
		{
			if (dropped && !dropped.GetHierarchyParent())
				onGround++;
		}
		string wrecks = "";
		array<Object> near = new array<Object>;
		GetGame().GetObjectsAtPosition(m_CarAt, 3, near, null);
		foreach (Object w : near)
		{
			if (w.GetType().IndexOf("Land_Wreck_") == 0)
				wrecks += " " + w.GetType();
		}
		near.Clear();
		GetGame().GetObjectsAtPosition(m_CarAt, 20, near, null);
		foreach (Object db : near)
		{
			if (db.GetType().IndexOf("BetterExplosions_") == 0 && db.GetType().IndexOf("Motorbike") < 0 && db.GetType().IndexOf("Boat") < 0)
				Print("[MolotovTest] " + db.GetType() + " landed " + vector.Distance(db.GetPosition(), m_CarAt) + " m from the car, " + (db.GetPosition()[1] - GetGame().SurfaceY(db.GetPosition()[0], db.GetPosition()[2])) + " m over the ground (want a wheel and a hood, ~8 m off)");
		}
		Print("[MolotovTest] car gone=" + (m_Car == null) + " (want 1), wreck where it stood:" + wrecks + " (want one), its load on the ground " + onGround + " (want " + m_CarLoad.Count() + ")");
		near.Clear();
		GetGame().GetObjectsAtPosition(m_BikeAt, 25, near, null);
		string bits = "";
		int bikes;
		foreach (Object bo : near)
		{
			if (bo.GetType().IndexOf("BetterExplosions_Motorbike") == 0)
				bits += " " + bo.GetType().Substring(17, bo.GetType().Length() - 17) + "@" + vector.Distance(bo.GetPosition(), m_BikeAt);
			if (bo.IsInherited(MotorbikeScript))
				bikes++;
		}
		Print("[MolotovTest] motorbike: " + bikes + " left (want 0), bits flown:" + bits + " (want 8, some 5-12 m off)");
		if (m_Bystander)
			Print("[MolotovTest] bystander 5 m from the car: health " + m_Bystander.GetHealth("", "Health") + " (want well under 100: the blast reaches them)");
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(CheckLater, 58000, false);
	}

	//! A fireplace with sticks and no kindling, hit by a lit molotov: the molotov is the
	//! lighter and the kindling. And a hand drill must not light a molotov, either way
	void ActionTest()
	{
		m_ActionFire = FireplaceBase.Cast(GetGame().CreateObjectEx("Fireplace", Ground(SPOT - ALONG * 22 + Side() * 9), ECE_PLACE_ON_SURFACE));
		Molotov unlit = Molotov.Cast(GetGame().CreateObjectEx("Molotov", Ground(SPOT - ALONG * 22 + Side() * 5), ECE_PLACE_ON_SURFACE));
		ItemBase drill = ItemBase.Cast(GetGame().CreateObjectEx("HandDrillKit", Ground(SPOT - ALONG * 22 + Side() * 4), ECE_PLACE_ON_SURFACE));
		ItemBase matches = ItemBase.Cast(GetGame().CreateObjectEx("Matchbox", Ground(SPOT - ALONG * 22 + Side() * 3), ECE_PLACE_ON_SURFACE));
		if (!m_ActionFire || !unlit || !drill || !matches || !m_Dummy)
		{
			Print("[MolotovTest] FAIL fireplace/drill test: something did not spawn");
			return;
		}
		m_ActionFire.GetInventory().CreateAttachment("WoodenStick");
		DropLit(m_ActionFire.GetPosition() + "0.3 2 0");

		bool byDrill = unlit.CanBeIgnitedBy(drill);
		bool byMatches = unlit.CanBeIgnitedBy(matches);
		Print("[MolotovTest] molotov can be lit by a hand drill " + byDrill + " (want 0), by matches " + byMatches + " (want 1)");
		ActionLightItemOnFire ignite = new ActionLightItemOnFire;
		ActionTarget onGround = new ActionTarget(drill, null, -1, drill.GetPosition(), 0);
		bool drillOnGround = ignite.ActionCondition(m_Dummy, onGround, unlit);
		ActionTarget molotovOnGround = new ActionTarget(unlit, null, -1, unlit.GetPosition(), 0);
		bool drillInHands = ignite.ActionCondition(m_Dummy, molotovOnGround, drill);
		Print("[MolotovTest] vanilla ignite offered: molotov in hands at a hand drill " + drillOnGround + " (want 0), hand drill in hands at a molotov " + drillInHands + " (want 0)");
		ActionTarget fireTarget = new ActionTarget(m_ActionFire, null, -1, m_ActionFire.GetPosition(), 0);
		Print("[MolotovTest] vanilla ignite offered: molotov in hands at the fireplace " + ignite.ActionCondition(m_Dummy, fireTarget, unlit) + " (want 0)");
	}

	//! Later still: the ruins have cooled
	void CheckCooled()
	{
		array<Object> all = new array<Object>;
		GetGame().GetObjectsAtPosition(Ground(BEACH) + m_BeachAlong * 80, 160, all, null);
		int hot;
		int cold;
		int smoulders;
		int hotAsh;
		foreach (Object ao : all)
		{
			string ash = ao.GetShapeName();
			if (ao.GetPosition()[1] < -20)
				continue;
			if (ash.Contains("_hot.p3d"))
				hot++;
			else if (ash.Contains("molotov\\ruins\\"))
				cold++;
			if (ao.IsInherited(MolotovSmoulder))
				smoulders++;
		}
		Print("[MolotovTest] two minutes on, on the beach: " + hot + " hot ruins (want 0), " + cold + " cooled ruins (want some), " + smoulders + " smoulders (want 0)");
		GetGame().RequestExit(0);
	}

	//! A minute on: the big fires are out and the lumber pile has burned away
	void CheckLater()
	{
		CountCopies("a minute on");

		foreach (string rk, vector rat : m_StaticAt)
		{
			if (!rk.Contains("house") && rk != "tree" && rk != "bush")
				continue;
			vector rhit;
			vector rnormal;
			int rcomp;
			set<Object> rhits = new set<Object>;
			DayZPhysics.RaycastRV(rat + "0 30 0", rat - "0 30 0", rhit, rnormal, rcomp, rhits, null, null, false, false, ObjIntersectGeom);
			string rwhat = "terrain";
			if (rhits.Count() > 0)
				rwhat = rhits[0].GetShapeName() + " at " + rhits[0].GetPosition();
			Print("[MolotovTest] ray down through " + rk + " hits " + rwhat + " at height " + rhit[1]);
			Object bobj;
			vector bhit;
			vector bnorm;
			float bfrac;
			bool bany = DayZPhysics.RayCastBullet(rat + "0 30 0", rat - "0 30 0", PhxInteractionLayers.BUILDING | PhxInteractionLayers.ROADWAY | PhxInteractionLayers.TERRAIN, null, bobj, bhit, bnorm, bfrac);
			string bwhat = "nothing";
			if (bobj)
				bwhat = bobj.GetShapeName() + " at " + bobj.GetPosition();
			else if (bany)
				bwhat = "something with no object (terrain?)";
			Print("[MolotovTest] physics ray down through " + rk + " hits " + bwhat + " at height " + bhit[1] + ", terrain " + GetGame().SurfaceY(rat[0], rat[2]));
		}
		foreach (string hk, vector hat : m_StaticAt)
		{
			if (!hk.Contains("house") && !hk.Contains("shed"))
				continue;
			array<Object> left = new array<Object>;
			GetGame().GetObjectsAtPosition(hat, 10, left, null);
			foreach (Object lo : left)
			{
				string lsh = lo.GetShapeName();
				if (lo.IsTree() || lo.IsBush() || lo.IsInherited(MolotovAshes) || lsh == "")
					continue;
				Print("[MolotovTest] left at " + hk + ": '" + lo.GetType() + "' " + lsh + " at " + lo.GetPosition());
			}
		}
		Print("[MolotovTest] a minute on: lumber pile gone=" + (m_Lumber == null) + " (want 1), on the beach gone=" + (m_BeachLumber == null) + " (want 1)");
		Print("[MolotovTest] a minute on: big fires at the hay " + Infernos(m_StaticAt.Get("hay")) + " (want 0, burnt out and not relit)");
		foreach (string k, vector at : m_StaticAt)
		{
			if (k == "pump")
				continue;
			Print("[MolotovTest] a minute on: " + k + " still standing " + Standing(at) + " (want 0: gone until restart), heaps of ash " + Ashes(at) + " (want some, unless it left a ruin)");
		}
		if (m_WoodFire)
			Print("[MolotovTest] a minute on: fireplace with firewood burning=" + m_WoodFire.IsBurning() + " (want 1)");
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(CheckCooled, 45000, false);
	}

	//! every model on the beach and every ruin: how many of it are within 15 m, above ground
	void CountCopies(string when)
	{
		array<Object> objs = new array<Object>;
		GetGame().GetObjectsAtPosition(Ground(BEACH) + m_BeachAlong * 80, 140, objs, null);
		map<string, int> n = new map<string, int>;
		foreach (Object o : objs)
		{
			string sh = o.GetShapeName();
			if (o.GetPosition()[1] < -20)
				continue;
			if (BEACH_MODELS.Find(sh) >= 0 || sh.Contains("ruin_"))
				n.Set(sh, n.Get(sh) + 1);
		}
		foreach (string k, int c : n)
		{
			if (c != 1)
				Print("[MolotovTest] " + when + ": " + c + " x " + k);
		}
		Print("[MolotovTest] " + when + ": " + n.Count() + " models counted");
	}

	//! anything that burns still at this spot
	bool Standing(vector at)
	{
		array<Object> objs = new array<Object>;
		GetGame().GetObjectsAtPosition(at, 1, objs, null);
		foreach (Object o : objs)
		{
			if (MolotovIgnition.IsFlammable(o) && vector.Distance(o.GetPosition(), at) < 0.1 && !o.IsDamageDestroyed())
			{
				Print("[MolotovTest] still standing at " + at + ": '" + o.GetType() + "' " + o.GetShapeName() + " at " + o.GetPosition() + ", damage " + o.GetDamage() + ", alive " + o.IsAlive());
				return true;
			}
		}
		return false;
	}

	int Ashes(vector at)
	{
		array<Object> objs = new array<Object>;
		GetGame().GetObjectsAtPosition(at, 12, objs, null);
		int n;
		foreach (Object o : objs)
		{
			if (o.IsInherited(MolotovAshes))
				n++;
		}
		return n;
	}

	int Infernos(vector at, float within = 12)
	{
		array<Object> objs = new array<Object>;
		GetGame().GetObjectsAtPosition(at, within, objs, null);
		int n;
		foreach (Object o : objs)
		{
			if (o.IsInherited(MolotovInferno))
				n++;
		}
		return n;
	}

	//! Open sea off the quay: 3 m deep or more, nothing (jetties, buoys) within 8 m
	vector FindSea(vector from)
	{
		array<Object> objs = new array<Object>;
		for (int d = 25; d <= 120; d += 5)
		{
			for (int a = 0; a < 24; a++)
			{
				float ang = a * Math.PI2 / 24;
				vector p = from + Vector(Math.Cos(ang) * d, 0, Math.Sin(ang) * d);
				if (!GetGame().SurfaceIsSea(p[0], p[2]))
					continue;
				p[1] = GetGame().SurfaceY(p[0], p[2]);
				float depth = GetGame().GetWaterDepth(p);
				if (depth < 3)
					continue;
				p[1] = p[1] + depth;
				objs.Clear();
				GetGame().GetObjectsAtPosition(p, 8, objs, null);
				if (objs.Count() == 0)
					return p;
			}
		}
		return vector.Zero;
	}

	void ListNear()
	{
		array<Object> near = new array<Object>;
		GetGame().GetObjectsAtPosition(Ground(SPOT), 50, near, null);
		map<string, int> counts = new map<string, int>;
		foreach (Object o : near)
		{
			if (o.IsInherited(ItemBase) || o.IsInherited(CarScript) || o.IsInherited(ZombieBase))
				counts.Set(o.GetType(), counts.Get(o.GetType()) + 1);
		}
		foreach (string type, int count : counts)
			Print("[MolotovTest] " + count + " x " + type);
	}

	vector Ground(vector p)
	{
		p[1] = GetGame().SurfaceRoadY(p[0], p[2]);
		return p;
	}

	vector Side()
	{
		return Vector(ALONG[2], 0, -ALONG[0]);
	}

	//! The first spot of sea about knee deep, walking from from towards dir
	vector Shore(vector from, vector dir)
	{
		for (int d = 0; d < 150; d++)
		{
			vector p = from + dir * (float)d;
			if (!GetGame().SurfaceIsSea(p[0], p[2]))
				continue;
			p[1] = GetGame().SurfaceY(p[0], p[2]);
			float depth = GetGame().GetWaterDepth(p);
			if (depth >= 0.7)
			{
				p[1] = p[1] + depth;
				return p;
			}
		}
		return vector.Zero;
	}

	//! A fuelled boat floating near from (a clear spot of sea at least 3 m deep)
	BoatScript SpawnBoat(vector from)
	{
		vector sea = FindSea(from);
		if (sea == vector.Zero)
		{
			Print("[Molotov] no sea near " + from + " for the boat");
			return null;
		}
		return SpawnBoatAt(sea);
	}

	BoatScript SpawnBoatAt(vector sea)
	{
		if (sea == vector.Zero)
		{
			Print("[Molotov] nowhere to float the boat");
			return null;
		}
		BoatScript boat = BoatScript.Cast(GetGame().CreateObjectEx("Boat_01_Orange", sea, ECE_CREATEPHYSICS | ECE_TRACE));
		if (boat)
		{
			boat.OnDebugSpawn();
			boat.SetLifetimeMax(3888000);
			boat.SetLifetime(3888000);
			Print("[Molotov] boat at " + boat.GetPosition());
		}
		return boat;
	}

	void SpawnCar()
	{
		SpawnCarAt(Ground(SPOT + ALONG * 10 + Side() * 4), ALONG);
	}

	//! A vehicle type's collision box, measured on a copy made far off and deleted
	void Length(string type, out vector hi, out vector lo)
	{
		hi = "0 0 2.5";
		lo = "0 0 -2.5";
		Object probe = GetGame().CreateObjectEx(type, "100 0 100", ECE_LOCAL);
		vector box[2];
		if (probe && probe.GetCollisionBox(box))
		{
			lo = box[0];
			hi = box[1];
		}
		if (probe)
			GetGame().ObjectDelete(probe);
	}

	//! What's on the beach before anything's laid out: after a restart, whatever was kept.
	//! True if there are no vehicles from last time (a fresh world: lay them out)
	bool Census(vector b)
	{
		array<Object> objs = new array<Object>;
		GetGame().GetObjectsAtPosition(b, 150, objs, null);
		map<string, int> kinds = new map<string, int>;
		int vehicles;
		foreach (Object o : objs)
		{
			string kind = "";
			string t = o.GetType();
			EntityAI e = EntityAI.Cast(o);
			if (Transport.Cast(o))
			{
				kind = "vehicle " + t;
				if (e.IsRuined())
					kind += " (ruined)";
				vehicles++;
			}
			else if (t.IndexOf("Land_Wreck_") == 0 || t.IndexOf("Land_wreck_") == 0)
				kind = "car wreck";
			else if (t.IndexOf("BetterExplosions_") == 0 || BetterExplosionsDebris.Cast(o))
				kind = "flying debris / sunk boat";
			else if (t == "MolotovAshes")
				kind = "ash heap";
			else if (t == "MolotovSmoulder")
				kind = "smoulder";
			else if (t.IndexOf("Molotov") == 0 && !ItemBase.Cast(o))
				kind = "fire " + t;
			else if (ItemBase.Cast(o) && !e.GetHierarchyParent())
				kind = "item on the ground";
			if (kind == "")
				continue;
			kinds.Set(kind, kinds.Get(kind) + 1);
		}
		Print("[MolotovRestart] on the beach at start, before laying out:");
		foreach (string k, int n : kinds)
			Print("[MolotovRestart]   " + n + " x " + k);
		if (kinds.Count() == 0)
			Print("[MolotovRestart]   nothing");
		return vehicles == 0;
	}

	void SpawnCarAt(vector at, vector dir, string type = "OffroadHatchback")
	{
		EntityAI car = EntityAI.Cast(GetGame().CreateObjectEx(type, at, ECE_PLACE_ON_SURFACE));
		if (!car)
		{
			Print("[Molotov] could not create a " + type);
			return;
		}
		car.SetDirection(dir);
		car.SetLifetimeMax(3888000);
		car.SetLifetime(3888000);
		car.OnDebugSpawn();   // wheels, doors, battery, fluids
		Print("[Molotov] " + type + " at " + at);
	}

	override void StartingEquipSetup(PlayerBase player, bool clothesChosen)
	{
		vector pos = Ground(SPOT);
		vector along = ALONG;
		if (!m_Test)
		{
			pos = Ground(BEACH);
			along = m_BeachAlong;
		}
		vector side = Vector(along[2], 0, -along[0]);
		player.SetPosition(pos);
		player.SetDirection(along);

		// The kit, in a row on the ground to the player's left
		for (int i = 0; i < KIT.Count(); i++)
		{
			for (int n = 0; n < 3; n++)
			{
				vector p = Ground(pos - side * (1.5 + n * 0.6) + along * (i * 0.8 - 1));
				string type = KIT[i];
				bool filled = type.IndexOf("+") == type.Length() - 1;
				if (filled)
					type = type.Substring(0, type.Length() - 1);
				ItemBase item = ItemBase.Cast(GetGame().CreateObjectEx(type, p, ECE_PLACE_ON_SURFACE));
				if (!item)
				{
					Print("[Molotov] unknown kit item " + type);
					break;
				}
				if (filled)
				{
					item.SetLiquidType(LIQUID_GASOLINE);
					item.SetQuantityMax();
				}
				else if (item.HasQuantity() && !item.IsLiquidContainer())
				{
					item.SetQuantityMax();   // rag stacks, matches
				}
				else if (item.IsKindOf("CanisterGasoline"))
				{
					item.SetQuantityMax();
				}
			}
		}

		// Infected in a loose arc ahead (the quay only: the beach is for burning things)
		for (int z = 0; z < INFECTED.Count() && m_Test; z++)
		{
			vector zp = Ground(pos + ALONG * 40 + Side() * ((z - 2) * 6));
			GetGame().CreateObjectEx(INFECTED[z], zp, ECE_PLACE_ON_SURFACE | ECE_INITAI | ECE_EQUIP);
		}
		Print("[Molotov] player kitted at " + pos);
	}
};

Mission CreateCustomMission(string path)
{
	return new CustomMission();
}
