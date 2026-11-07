//! Burnt-out map objects (sheds, hay, straw stacks, pallets, woodpiles) are gone until the
//! server restarts: sunk out of sight underground. Map objects aren't networked, and the engine streams them back in from
//! the map whenever their area reloads (within a couple of seconds on an empty server, and
//! on a client as the player moves about), so a one-off delete doesn't stick. Server and
//! clients each keep the list and sweep it every second; the server tells every client,
//! now and when they join.
class MolotovGone
{
	static const int RPC_GONE = 0x4D4F4C01;   //!< "MOL" 1
	static const int SWEEP_MS = 1000;
	static const vector SINK = "0 50 0";
	static const int TREE_DESTROY_MS = 2000;   //!< clients sink a burnt tree, then the server destroys it
	static int s_HotMs = 240000;               //!< ruins and ash glow and smoke this long, then they're cold charcoal (the self-test shortens it)

	//! Server: destroy a tree or shrub as chopping does
	static void Destroy(string model, vector pos)
	{
		Object o = Find(model, pos);
		if (!o)
			return;
		o.DecreaseHealth("", "", o.GetMaxHealth("", "") * 2, o.CanBeAutoDeleted());
		Print("[Molotov] " + model + " destroyed: " + o.IsDamageDestroyed());
	}

	//! model + position of everything burnt away (server: all of it; client: what it's been told)
	protected static ref array<string> s_Models = new array<string>;
	protected static ref array<vector> s_Positions = new array<vector>;
	protected static bool s_Sweeping;

	//! Server: the fire on model at pos has gone out - remove it here and on every client
	static void BurnAway(string model, vector pos)
	{
		if (!GetGame().IsServer())
			return;
		Print("[Molotov] " + model + " burned away at " + pos);
		Object o = Find(model, pos);
		if (o)
		{
			string wreck = MolotovIgnition.Wreck(model);
			if (wreck == "" && o.IsTree())
				wreck = BurntTree(o);
			if (wreck == "" || !LeaveWreck(o, wreck))
				MolotovAshes.Spread(o);
			ClearLoot(o);
			// trees and shrubs won't move on the server: destroy them the way chopping one
			// down does, which the engine keeps in step on every client (and gone until
			// restart). A destroyed tree falls as if felled, so clients sink their own copy
			// first (below), and only then does the server destroy it - nothing left to fall
			if (o.IsBush())
			{
				Destroy(model, pos);
				return;
			}
			if (o.IsTree())
				GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(MolotovGone.Destroy, TREE_DESTROY_MS, false, model, pos);
		}
		Add(model, pos);
		if (GetGame().IsMultiplayer())
			GetGame().RPCSingleParam(null, RPC_GONE, new Param2<string, vector>(model, pos), true, null);
	}

	//! Server: a player has connected - tell them what's already gone
	static void SendAll(PlayerIdentity identity)
	{
		if (!identity)
			return;
		for (int i = 0; i < s_Models.Count(); i++)
			GetGame().RPCSingleParam(null, RPC_GONE, new Param2<string, vector>(s_Models[i], s_Positions[i]), true, identity);
	}

	//! Client: the server says it's gone (DayZGame.Event_OnRPC, hooked by MissionGameplay)
	static void OnGameRPC(PlayerIdentity sender, Object target, int rpc_type, ParamsReadContext ctx)
	{
		if (rpc_type != RPC_GONE || GetGame().IsServer())
			return;
		Param2<string, vector> p = new Param2<string, vector>("", vector.Zero);
		if (!ctx.Read(p))
			return;
		Add(p.param1, p.param2);
	}

	static void Add(string model, vector pos)
	{
		for (int i = 0; i < s_Models.Count(); i++)
		{
			if (s_Models[i] == model && vector.Distance(s_Positions[i], pos) < 0.1)
				return;
		}
		s_Models.Insert(model);
		s_Positions.Insert(pos);
		bool found = Delete(model, pos);
		if (!GetGame().IsServer())
		{
			Print("[Molotov] told " + model + " at " + pos + " burned away: sunk " + found);
			if (!found)
				ListNear(pos);
		}
		if (!s_Sweeping)
		{
			s_Sweeping = true;
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(MolotovGone.Sweep, SWEEP_MS, true);
		}
	}

	static const int SOLID_LAYERS = PhxInteractionLayers.BUILDING | PhxInteractionLayers.ROADWAY | PhxInteractionLayers.TERRAIN | PhxInteractionLayers.FENCE | PhxInteractionLayers.DEFAULT;

	//! What the physics (what characters stand on) has straight below from: "" if nothing
	//! (the self-test's check that a burnt object leaves nothing solid behind)
	static string PhysicsBelow(vector from, float depth, Object ignore)
	{
		Object obj;
		vector hit;
		vector normal;
		float frac;
		if (!DayZPhysics.RayCastBullet(from, from - Vector(0, depth, 0), SOLID_LAYERS, ignore, obj, hit, normal, frac))
			return "";
		if (!obj)
			return "terrain at " + hit[1];
		return "'" + obj.GetType() + "' " + obj.GetShapeName() + " at " + obj.GetPosition() + " (hit at " + hit[1] + ")";
	}

	//! Client: what's there instead, when the object to sink isn't found
	static void ListNear(vector pos)
	{
		array<Object> objs = new array<Object>;
		GetGame().GetObjectsAtPosition(pos, 3, objs, null);
		foreach (Object o : objs)
			Print("[Molotov]   near it: '" + o.GetType() + "' " + o.GetShapeName() + " at " + o.GetPosition());
	}

	//! Delete whatever has streamed back in
	static void Sweep()
	{
		for (int i = 0; i < s_Models.Count(); i++)
		{
			if (Delete(s_Models[i], s_Positions[i]) && !GetGame().IsServer())
				Print("[Molotov] " + s_Models[i] + " at " + s_Positions[i] + " streamed back in: sunk again");
		}
	}

	//! Our charred snag nearest a burnt tree's height (blender/build_ruins.py: 5, 9, 14 m)
	static string BurntTree(Object o)
	{
		vector clip[2];
		o.ClippingInfo(clip);
		vector lo = clip[0];
		vector hi = clip[1];
		float tall = hi[1] - lo[1];
		string size = "large";
		if (tall < 7)
			size = "small";
		else if (tall < 11.5)
			size = "medium";
		return "Molotov\\ruins\\tree_" + size + "_burnt.p3d";
	}

	//! Server: the burnt-out ruin where o stood, facing the same way, the middle of its
	//! footprint over the middle of the house's and its bottom on the house's bottom. A ruin
	//! is far lower than its house and its model's origin sits elsewhere, so the fit is
	//! worked out on a server-only copy (which also takes care of the engine shifting a
	//! model it spawns)
	static bool LeaveWreck(Object o, string wreck)
	{
		vector pos = o.GetPosition();
		vector ori = o.GetOrientation();
		// ours comes hot (glowing) first and is swapped for the cold one later
		bool own = wreck.Contains("Molotov\\ruins\\");
		string spawn = wreck;
		if (own)
			spawn.Replace("_burnt.p3d", "_hot.p3d");
		Object probe = GetGame().CreateStaticObjectUsingP3D(spawn, pos, ori * Math.DEG2RAD, 1.0, true);   // it takes radians
		if (!probe)
		{
			Print("[Molotov] no ruin model " + spawn);
			return false;
		}
		vector delta = "0 0 0";
		vector hb[2];
		vector rb[2];
		if (o.GetCollisionBox(hb) && probe.GetCollisionBox(rb))
		{
			vector hlo = hb[0];
			vector hhi = hb[1];
			vector rlo = rb[0];
			vector rhi = rb[1];
			vector houseBase = o.ModelToWorld(Vector((hlo[0] + hhi[0]) * 0.5, hlo[1], (hlo[2] + hhi[2]) * 0.5));
			if (o.IsTree())
			{
				// a tree's box runs deep underground: its stump stands on the ground at its foot
				houseBase = pos;
				houseBase[1] = GetGame().SurfaceY(pos[0], pos[2]) - 0.15;
			}
			vector ruinBase = probe.ModelToWorld(Vector((rlo[0] + rhi[0]) * 0.5, rlo[1], (rlo[2] + rhi[2]) * 0.5));
			delta = houseBase - ruinBase;
		}
		vector landed = probe.GetPosition() + delta;
		GetGame().ObjectDelete(probe);
		Object ruin = GetGame().CreateStaticObjectUsingP3D(spawn, pos + delta, ori * Math.DEG2RAD);
		if (!ruin)
			return false;
		Print("[Molotov] " + o.GetShapeName() + " burned down to " + spawn + " at " + ruin.GetPosition() + " (aimed for " + landed + "), facing " + ruin.GetOrientation() + " (it faced " + ori + ")");
		if (own)
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(MolotovGone.Cool, s_HotMs, false, spawn, ruin.GetPosition(), pos + delta, ori, wreck);
		// smoke and a glow while it's hot; a Livonia ruin's own wood can't glow, so embers on it
		Smoulder(o, !own);
		return true;
	}

	//! Server: a hot ruin of ours has cooled - the glowing one goes (here and on clients),
	//! the cold one goes in its place (the same shape, so the same spot)
	static void Cool(string hot, vector hotAt, vector request, vector ori, string cold)
	{
		Add(hot, hotAt);
		if (GetGame().IsMultiplayer())
			GetGame().RPCSingleParam(null, RPC_GONE, new Param2<string, vector>(hot, hotAt), true, null);
		Object c = GetGame().CreateStaticObjectUsingP3D(cold, request, ori * Math.DEG2RAD);
		Print("[Molotov] " + hot + " at " + hotAt + " has cooled to " + cold + " at " + c.GetPosition());
	}

	//! Server: smoke (and a faint glow) over o's footprint for s_HotMs; embers too if asked
	static void Smoulder(Object o, bool embers)
	{
		vector box[2];
		vector lo = "-0.5 0 -0.5";
		vector hi = "0.5 0 0.5";
		if (o.GetCollisionBox(box))
		{
			lo = box[0];
			hi = box[1];
		}
		float area = (hi[0] - lo[0]) * (hi[2] - lo[2]);
		int n = Math.Clamp(Math.Round(area / 30), 1, 3);
		if (embers)
			n = Math.Clamp(Math.Round(area / 25), 3, 6);
		if (o.IsTree())
		{
			n = 1;
			lo = "-0.3 0 -0.3";
			hi = "0.3 0 0.3";
		}
		for (int i = 0; i < n; i++)
		{
			vector m = Vector(Math.RandomFloat(lo[0], hi[0]) * 0.8, 0, Math.RandomFloat(lo[2], hi[2]) * 0.8);
			if (o.IsTree())
				m = "0 0 0";
			vector at = o.ModelToWorld(m);
			if (o.IsTree())
				at = o.GetPosition();
			at[1] = GetGame().SurfaceY(at[0], at[2]);
			GetGame().CreateObjectEx("MolotovSmoulder", at, ECE_PLACE_ON_SURFACE | ECE_NOLIFETIME);
			if (!embers)
				continue;
			MolotovAshes heap = MolotovAshes.Cast(GetGame().CreateObjectEx("MolotovAshes", at, ECE_PLACE_ON_SURFACE | ECE_NOLIFETIME));
			if (heap)
			{
				heap.SetOrientation(Vector(Math.RandomFloat(0, 360), 0, 0));
				heap.Smoulder(s_HotMs);
			}
		}
	}

	//! Server: loose items inside o's footprint burn with it (they'd float where its floors were)
	static void ClearLoot(Object o)
	{
		vector box[2];
		if (!o.GetCollisionBox(box))
			return;
		vector lo = box[0];
		vector hi = box[1];
		array<Object> objs = new array<Object>;
		GetGame().GetObjectsAtPosition(o.GetPosition(), vector.Distance(lo, hi) * 0.5 + 1, objs, null);
		int burnt;
		foreach (Object x : objs)
		{
			ItemBase item = ItemBase.Cast(x);
			if (!item || item.GetHierarchyParent())
				continue;
			vector m = o.WorldToModel(item.GetPosition());
			if (m[0] < lo[0] || m[0] > hi[0] || m[1] < lo[1] - 0.5 || m[1] > hi[1] || m[2] < lo[2] || m[2] > hi[2])
				continue;
			item.DeleteSafe();
			burnt++;
		}
		if (burnt > 0)
			Print("[Molotov] " + burnt + " loose items burned with " + o.GetShapeName());
	}

	//! Model paths compare without case: the engine reports them in lower case
	static bool SameModel(string a, string b)
	{
		a.ToLower();
		b.ToLower();
		return a == b;
	}

	static Object Find(string model, vector pos)
	{
		array<Object> objs = new array<Object>;
		GetGame().GetObjectsAtPosition(pos, 1, objs, null);
		foreach (Object o : objs)
		{
			if (SameModel(o.GetShapeName(), model) && vector.Distance(o.GetPosition(), pos) < 0.3)
				return o;
		}
		return null;
	}

	//! Put the map object with this model at this position 50 m underground; false if there's
	//! none. ObjectDelete, clearing its flags and scaling it all leave a map object standing
	//! (tried 2026-11-07); moving it is what sticks
	static bool Delete(string model, vector pos)
	{
		array<Object> objs = new array<Object>;
		GetGame().GetObjectsAtPosition(pos, 1, objs, null);
		foreach (Object o : objs)
		{
			if (SameModel(o.GetShapeName(), model) && vector.Distance(o.GetPosition(), pos) < 0.3)
			{
				// the server destroys trees and shrubs instead (see BurnAway); moving one
				// there would only lose track of it
				if (GetGame().IsServer() && (o.IsTree() || o.IsBush()))
					return true;
				o.SetPosition(pos - SINK);
				o.Update();
				return true;
			}
		}
		return false;
	}
}

//! A heap of ash where something burned away (see config.cpp); MolotovAshes.Spread covers
//! a burnt object's footprint with them
class MolotovAshes: House
{
	static const float AREA_PER_HEAP = 12;   //!< square metres of footprint per heap
	static const float MIN_APART = 1.5;      //!< a heap is about a metre across
	static const int MAX_HEAPS = 8;

	static const string HOT_RVMAT = "dz\\gear\\cooking\\data\\stoneground.rvmat";        //!< glowing embers
	static const string COLD_RVMAT = "dz\\gear\\cooking\\data\\stonegroundnoemit.rvmat";

	protected bool m_Hot;

	void MolotovAshes()
	{
		RegisterNetSyncVariableBool("m_Hot");
	}

	//! Server: glowing now, plain ash after ms
	void Smoulder(int ms)
	{
		m_Hot = true;
		SetSynchDirty();
		ShowHeat();
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(CoolDown, ms, false);
	}

	void CoolDown()
	{
		m_Hot = false;
		SetSynchDirty();
		ShowHeat();
	}

	override void OnVariablesSynchronized()
	{
		super.OnVariablesSynchronized();
		ShowHeat();
	}

	void ShowHeat()
	{
		if (m_Hot)
			SetObjectMaterial(0, HOT_RVMAT);
		else
			SetObjectMaterial(0, COLD_RVMAT);
	}

	override void EEDelete(EntityAI parent)
	{
		super.EEDelete(parent);
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).Remove(CoolDown);
	}

	override void EEInit()
	{
		super.EEInit();
		SetAnimationPhase("Ashes", 0);
		ShowHeat();
		// burnt logs in about half of them - from the position, so server and clients agree
		vector p = GetPosition();
		int cell = Math.Floor(p[0] * 3) + Math.Floor(p[2] * 3);
		if (cell % 2 == 0)
			SetAnimationPhase("BurntWood", 0);
	}

	//! Server: ash where o stood - one heap for something small; for something big a few,
	//! scattered at random over its footprint (a grid of them looks laid out, not burnt)
	static void Spread(Object o)
	{
		if (!GetGame().IsServer())
			return;
		vector box[2];
		vector lo = "-0.5 0 -0.5";
		vector hi = "0.5 0 0.5";
		if (o.GetCollisionBox(box))
		{
			lo = box[0];
			hi = box[1];
		}
		float area = (hi[0] - lo[0]) * (hi[2] - lo[2]);
		int want = Math.Clamp(Math.Round(area / AREA_PER_HEAP), 1, MAX_HEAPS);
		if (o.IsBush())
			want = 1;   // a burnt shrub: one heap where it stood
		array<vector> spots = new array<vector>;
		if (want == 1)
			spots.Insert((lo + hi) * 0.5);
		for (int tries = 0; spots.Count() < want && tries < want * 20; tries++)
		{
			vector m = Vector(Math.RandomFloat(lo[0], hi[0]), 0, Math.RandomFloat(lo[2], hi[2]));
			bool clear = true;
			foreach (vector s : spots)
			{
				if (vector.Distance(s, m) < MIN_APART)
					clear = false;
			}
			if (clear)
				spots.Insert(m);
		}
		int made;
		foreach (vector sp : spots)
		{
			vector mp = sp;
			mp[1] = 0;
			vector at = o.ModelToWorld(mp);
			at[1] = GetGame().SurfaceY(at[0], at[2]);
			if (GetGame().GetWaterDepth(at) > 0)
				continue;
			Object heap = GetGame().CreateObjectEx("MolotovAshes", at, ECE_PLACE_ON_SURFACE | ECE_NOLIFETIME);
			if (!heap)
				continue;
			heap.SetOrientation(Vector(Math.RandomFloat(0, 360), 0, 0));
			MolotovAshes ash = MolotovAshes.Cast(heap);
			if (ash)
				ash.Smoulder(MolotovGone.s_HotMs);
			made++;
		}
		Print("[Molotov] " + made + " heaps of ash where " + o.GetShapeName() + " stood");
	}
}

//! A ruin cooling down: a thread of smoke and a faint orange glow for MolotovGone.s_HotMs,
//! then it goes
class MolotovSmoulder: House
{
	protected Particle m_Smoke;
	protected ScriptedLightBase m_Light;

	override void EEInit()
	{
		super.EEInit();
		if (GetGame().IsServer())
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(GetGame().ObjectDelete, MolotovGone.s_HotMs, false, this);
		if (!GetGame().IsDedicatedServer())
		{
			vector p = GetPosition();
			m_Smoke = ParticleManager.GetInstance().PlayInWorld(ParticleList.CAMP_SMALL_SMOKE, p);
			m_Light = ScriptedLightBase.CreateLight(FireplaceLight, p + "0 0.3 0");
			if (m_Light)
			{
				m_Light.SetRadiusTo(3);
				m_Light.SetBrightnessTo(0.4);
			}
		}
	}

	override void EEDelete(EntityAI parent)
	{
		super.EEDelete(parent);
		if (m_Smoke)
			m_Smoke.Stop();
		if (m_Light)
			m_Light.FadeOut(3);
	}
}
