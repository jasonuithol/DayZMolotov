// Molotov_Expansion test bed: Chernarus with Expansion Vehicles, Molotov and the add-on.
// Players spawn in a field (SPOT) beside one fuelled vehicle of each kind the add-on knows,
// fully kitted (every wheel, door and part), with 20 molotovs at their feet and a lighter.
//
// Headless (-missiontest, tools/test.sh molotov_expansion): blows each vehicle up the way a
// molotov fire does (MolotovIgnition.GoUp) and checks it became its wreck, where it stood,
// facing the same way, with its cargo thrown out; logs [MolotovExpTest] lines and quits.
void main()
{
	Hive ce = CreateHive();
	if (ce)
		ce.InitOffline();
	GetGame().GetWorld().SetDate(2026, 9, 20, 10, 0);
}

class CustomMission: MissionServer
{
	static const vector SPOT = "2000 0 12000";
	static const ref array<string> TYPES = {"ExpansionTractor_Red", "ExpansionBus_Blue", "ExpansionUAZ_Black", "Expansion_Landrover_Green", "ExpansionUh1h", "OffroadHatchback"};
	static const ref array<string> WANT = {"Land_Wreck_Tractor", "Land_Wreck_Ikarus", "Land_Wreck_Uaz", "Land_Wreck_LR_TK", "Wreck_UH1Y", "Land_Wreck_hb01_aban"};
	protected ref array<CarScript> m_Cars = new array<CarScript>;
	protected ref array<vector> m_At = new array<vector>;
	protected ref array<vector> m_Mid = new array<vector>;
	protected ref array<vector> m_Dir = new array<vector>;
	protected ref array<float> m_Slope = new array<float>;
	protected ref array<float> m_Len = new array<float>;
	protected ref array<EntityAI> m_Loads = new array<EntityAI>;

	override void OnInit()
	{
		super.OnInit();
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(Park, 5000, false);
		string value;
		if (GetGame().CommandlineGetParam("missiontest", value))
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(Blow, 15000, false);
	}

	override PlayerBase CreateCharacter(PlayerIdentity identity, vector pos, ParamsReadContext ctx, string characterName)
	{
		vector at = SPOT + "0 0 -12";
		at[1] = GetGame().SurfaceY(at[0], at[2]);
		Entity playerEnt = GetGame().CreatePlayer(identity, characterName, at, 0, "NONE");
		Class.CastTo(m_player, playerEnt);
		GetGame().SelectPlayer(identity, m_player);
		return m_player;
	}

	override void StartingEquipSetup(PlayerBase player, bool clothesChosen)
	{
		player.GetInventory().CreateInInventory("TrackSuitJacket_Black");
		player.GetInventory().CreateInInventory("TrackSuitPants_Black");
		player.GetInventory().CreateInInventory("PetrolLighter");
		player.GetInventory().CreateInInventory("Matchbox");
		// a pile of molotovs at their feet, in a ring a metre out
		vector at = player.GetPosition();
		for (int i = 0; i < 20; i++)
		{
			float a = i * Math.PI2 / 20;
			vector p = at + Vector(Math.Cos(a), 0, Math.Sin(a));
			p[1] = GetGame().SurfaceY(p[0], p[2]);
			GetGame().CreateObjectEx("Molotov", p, ECE_PLACE_ON_SURFACE);
		}
	}

	//! one fuelled vehicle of each kind in a row, 25 m apart, each with a can of food in it
	void Park()
	{
		foreach (int i, string type : TYPES)
		{
			vector at = SPOT + Vector(i * 25, 0, 0);
			at[1] = GetGame().SurfaceY(at[0], at[2]) + 0.3;
			CarScript car = CarScript.Cast(GetGame().CreateObjectEx(type, at, ECE_PLACE_ON_SURFACE | ECE_SETUP));
			if (!car)
			{
				Print("[MolotovExpTest] FAIL could not spawn " + type);
				continue;
			}
			car.SetOrientation(Vector(i * 30, 0, 0));   // all sorts of headings
			// every part: wheels, doors, hood, trunk, battery, plugs, radiator, lights
			car.OnDebugSpawn();
			car.Fill(CarFluid.FUEL, car.GetFluidCapacity(CarFluid.FUEL));
			car.Fill(CarFluid.OIL, car.GetFluidCapacity(CarFluid.OIL));
			car.Fill(CarFluid.COOLANT, car.GetFluidCapacity(CarFluid.COOLANT));
			car.Fill(CarFluid.BRAKE, car.GetFluidCapacity(CarFluid.BRAKE));
			Print("[MolotovExpTest] " + type + " parked with " + car.GetInventory().AttachmentCount() + " parts");
			EntityAI load = car.GetInventory().CreateInInventory("BakedBeansCan");
			m_Cars.Insert(car);
			m_Loads.Insert(load);
		}
	}

	//! height of whatever is on top at a world point, over the terrain
	float Top(vector p)
	{
		vector hit;
		vector nrm;
		int comp;
		set<Object> hits = new set<Object>;
		if (!DayZPhysics.RaycastRV(p + "0 15 0", p - "0 2 0", hit, nrm, comp, hits, null, null, false, false, ObjIntersectGeom))
			return 0;
		return hit[1] - GetGame().SurfaceY(p[0], p[2]);
	}

	//! how much higher the back of the thing at at (facing dir, len long) is than its front
	float Slope(vector at, vector dir, float len)
	{
		return Top(at - dir * len * 0.3) - Top(at + dir * len * 0.3);
	}

	void Blow()
	{
		foreach (CarScript car : m_Cars)
		{
			vector box[2];
			car.GetCollisionBox(box);
			vector lo = box[0];
			vector hi = box[1];
			vector dir = car.GetDirection();
			dir[1] = 0;
			dir.Normalize();
			m_At.Insert(car.GetPosition());
			m_Mid.Insert(car.ModelToWorld((lo + hi) * 0.5));
			m_Dir.Insert(dir);
			m_Len.Insert(hi[2] - lo[2]);
			m_Slope.Insert(Slope(car.ModelToWorld((lo + hi) * 0.5), dir, hi[2] - lo[2]));
			Print("[MolotovExpTest] " + car.GetType() + " fuel " + car.GetFluidFraction(CarFluid.FUEL) + ", wreck for it: " + MolotovWreck.For(car) + ", turn " + MolotovWreck.Turn(car));
			MolotovIgnition.GoUp(car);
		}
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(Check, 5000, false);
	}

	void Check()
	{
		int good = 0;
		foreach (int i, string type : TYPES)
		{
			array<Object> near = new array<Object>;
			GetGame().GetObjectsAtPosition(m_Mid[i], 6, near, null);
			Object wreck;
			bool carLeft = false;
			foreach (Object o : near)
			{
				string ot = o.GetType();
				if (ot.IndexOf(WANT[i]) == 0)
					wreck = o;
				if (ot == type)
					carLeft = true;
			}
			bool loadOut = m_Loads[i] && !m_Loads[i].GetHierarchyParent();
			array<Object> around = new array<Object>;
			GetGame().GetObjectsAtPosition(m_Mid[i], 18, around, null);
			string flown = "";
			foreach (Object fo : around)
			{
				if (fo.GetType().IndexOf("MolotovDebris_") == 0)
					flown += " " + fo.GetType() + "@" + vector.Distance(fo.GetPosition(), m_Mid[i]).ToString();
			}
			Print("[MolotovExpTest] " + type + " flung:" + flown);
			if (!wreck)
			{
				Print("[MolotovExpTest] FAIL " + type + ": no " + WANT[i] + " (vehicle still there " + carLeft + ")");
				continue;
			}
			// middle to middle, across the ground
			vector box[2];
			wreck.GetCollisionBox(box);
			vector wmid = wreck.ModelToWorld((box[0] + box[1]) * 0.5);
			wmid[1] = 0;
			vector cmid = m_Mid[i];
			cmid[1] = 0;
			float off = vector.Distance(wmid, cmid);
			float slope = Slope(m_Mid[i], m_Dir[i], m_Len[i]);
			// the back-to-front height difference keeps its sign if the wreck faces the same way
			bool facing = (slope > 0) == (m_Slope[i] > 0);
			Print("[MolotovExpTest] " + type + " -> " + wreck.GetType() + ", middle " + off + " m from the vehicle's, back-front height " + m_Slope[i] + " -> " + slope + " (same way " + facing + "), vehicle gone " + !carLeft + ", cargo thrown out " + loadOut);
			if (!carLeft && loadOut && off < 0.5 && (facing || type == "ExpansionBus_Blue" || type == "ExpansionUh1h" || type == "Expansion_Landrover_Green"))
				good++;
		}
		Print("[MolotovExpTest] " + good + " of " + TYPES.Count() + " good");
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(GetGame().RequestExit, 1000, false, 0);
	}
}

Mission CreateCustomMission(string path)
{
	return new CustomMission();
}
