// Molotov test bed: loads ONE Workshop fire/molotov mod (see mods.txt) and nothing else.
// Players spawn on the Chernogorsk quay with the mod's molotov ingredients and a lighter
// at their feet, a fuelled hatchback parked beside them and a handful of infected
// shambling in from ~40 m away. Every (re)spawn lays out a fresh kit and fresh infected.

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

	// Our Molotov mod: GlassBottle with >= 200 ml petrol/diesel + Rag -> Molotov; "Light the
	// rag" with a lighter/matches in the inventory, then throw.
	// "GlassBottle+" = a bottle already filled with petrol; plain "GlassBottle" is empty,
	// to fill from the jerry can.
	static const ref array<string> KIT = {
		"Molotov", "GlassBottle", "GlassBottle+", "Rag", "PetrolLighter", "Matchbox", "CanisterGasoline"
	};
	static const ref array<string> INFECTED = {
		"ZmbM_HermitSkinny_Beige", "ZmbF_CitizenANormal_Blue", "ZmbM_PatrolNormal_Autumn",
		"ZmbM_CitizenASkinny_Red", "ZmbF_JournalistNormal_White"
	};

	void CustomMission()
	{
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(SpawnCar, 2000, false);
		string value;
		if (GetGame().CommandlineGetParam("missiontest", value))
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(SelfTest, 4000, false);
	}

	protected PlayerBase m_Dummy;
	protected CarScript m_Car;
	protected float m_DummyHealth;
	protected float m_CarEngine;
	protected ZombieBase m_Zombie;

	//! -missiontest: kit out an identity-less dummy, drop a lit molotov on it and another
	//! on the car, check the fires and their damage, quit
	void SelfTest()
	{
		string probe;
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
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(CheckFires, 6000, false);
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

	void CheckFires()
	{
		array<Object> objs = new array<Object>;
		GetGame().GetObjectsAtPosition(Ground(SPOT), 40, objs, null);
		int fires;
		int molotovs;
		foreach (Object o : objs)
		{
			if (o.IsInherited(MolotovFire))
				fires++;
			else if (o.IsInherited(Molotov))
				molotovs++;
		}
		Print("[MolotovTest] fires burning: " + fires + " (want 3), molotovs left: " + molotovs + " (want 3, the kit)");
		Print("[MolotovTest] dummy health " + m_DummyHealth + " -> " + m_Dummy.GetHealth("", "Health"));
		if (m_Zombie)
			Print("[MolotovTest] infected health -> " + m_Zombie.GetHealth("", "Health") + " alive=" + m_Zombie.IsAlive());
		if (m_Car)
			Print("[MolotovTest] car engine " + m_CarEngine + " -> " + m_Car.GetHealth("Engine", "Health"));
		GetGame().RequestExit(0);
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

	void SpawnCar()
	{
		vector at = Ground(SPOT + ALONG * 10 + Side() * 4);
		CarScript car = CarScript.Cast(GetGame().CreateObjectEx("OffroadHatchback", at, ECE_PLACE_ON_SURFACE));
		if (!car)
		{
			Print("[Molotov] could not create the car");
			return;
		}
		car.SetDirection(ALONG);
		car.SetLifetimeMax(3888000);
		car.SetLifetime(3888000);
		car.OnDebugSpawn();   // wheels, doors, battery, fluids
		Print("[Molotov] car at " + at);
	}

	override void StartingEquipSetup(PlayerBase player, bool clothesChosen)
	{
		vector pos = Ground(SPOT);
		player.SetPosition(pos);
		player.SetDirection(ALONG);

		// The kit, in a row on the ground to the player's left
		for (int i = 0; i < KIT.Count(); i++)
		{
			for (int n = 0; n < 3; n++)
			{
				vector p = Ground(pos - Side() * (1.5 + n * 0.6) + ALONG * (i * 0.8 - 1));
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

		// Infected in a loose arc ahead
		for (int z = 0; z < INFECTED.Count(); z++)
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
