//! What's left of a boat that's gone up: its outboard motor, blown off at 45 degrees, and
//! the burnt, burst hull, both sinking to the bottom (Molotov\ruins\boat_*_burnt.p3d).
//! The server puts each straight where it ends up; the clients that see it happen play
//! the flight and the sinking themselves, from where it started. Anyone who turns up later
//! just sees it lying on the bottom. Not stored: gone at restart.
class MolotovSunk: House
{
	static const float GRAVITY = 9.81;
	static const float ENGINE_SPEED = 11;   //!< m/s, launched at 45 degrees: lands ~13 m off
	static const float ENGINE_SINK = 1.5;   //!< m/s through the water
	static const float HULL_SINK = 0.35;
	static const int SETTLE_MS = 20000;     //!< by then it's on the bottom wherever it was seen from

	protected float m_FromX;
	protected float m_FromY;
	protected float m_FromZ;
	protected float m_Speed;                //!< > 0: flies off at 45 degrees; 0: just sinks
	protected bool m_Settled;

	protected bool m_Started;
	protected bool m_InWater;
	protected float m_Age;
	protected vector m_Pos;
	protected vector m_Vel;
	protected vector m_Ori;
	protected vector m_Spin;
	protected vector m_RestPos;
	protected vector m_RestOri;

	void MolotovSunk()
	{
		RegisterNetSyncVariableFloat("m_FromX", 0, 0, 2);
		RegisterNetSyncVariableFloat("m_FromY", 0, 0, 2);
		RegisterNetSyncVariableFloat("m_FromZ", 0, 0, 2);
		RegisterNetSyncVariableFloat("m_Speed", 0, 0, 2);
		RegisterNetSyncVariableBool("m_Settled");
	}

	override void EEInit()
	{
		super.EEInit();
		if (!GetGame().IsDedicatedServer())
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(Begin, 100, false);
	}

	override void EEDelete(EntityAI parent)
	{
		GetGame().GetUpdateQueue(CALL_CATEGORY_GAMEPLAY).Remove(Tick);
		super.EEDelete(parent);
	}

	override void OnVariablesSynchronized()
	{
		super.OnVariablesSynchronized();
		Begin();
	}

	//! Server: it starts at from (flying off at speed, or just sinking) and settles where it is now
	void Launch(vector from, float speed)
	{
		m_FromX = from[0];
		m_FromY = from[1];
		m_FromZ = from[2];
		m_Speed = speed;
		SetSynchDirty();
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(Settle, SETTLE_MS, false);
	}

	protected void Settle()
	{
		m_Settled = true;
		SetSynchDirty();
	}

	//! Client: play it from the start, unless it's long over
	protected void Begin()
	{
		if (m_Started || m_FromX == 0 || GetGame().IsDedicatedServer())
			return;
		m_Started = true;
		if (m_Settled)
			return;
		m_RestPos = GetPosition();
		m_RestOri = GetOrientation();
		m_Pos = Vector(m_FromX, m_FromY, m_FromZ);
		m_Ori = m_RestOri;
		if (m_Speed > 0)
		{
			vector dir = m_RestPos - m_Pos;
			dir[1] = 0;
			dir.Normalize();
			float c = m_Speed * 0.7071;
			m_Vel = dir * c + Vector(0, c, 0);
			m_Spin = Vector(Math.RandomFloat(-300, 300), Math.RandomFloat(250, 450), Math.RandomFloat(-200, 200));
			m_Ori = m_RestOri + Vector(0, -60, -80);   // starts upright on the transom
		}
		else
		{
			m_Vel = vector.Zero;
			m_InWater = true;
			m_Ori = m_RestOri + Vector(0, Math.RandomFloat(-3, 3), Math.RandomFloat(-3, 3));
		}
		Place();
		GetGame().GetUpdateQueue(CALL_CATEGORY_GAMEPLAY).Insert(Tick);
	}

	protected void Tick(float dt)
	{
		m_Age += dt;
		if (!m_InWater)
		{
			m_Vel[1] = m_Vel[1] - GRAVITY * dt;
			m_Pos = m_Pos + m_Vel * dt;
			m_Ori = m_Ori + m_Spin * dt;
			float ground = GetGame().SurfaceY(m_Pos[0], m_Pos[2]);
			float sea = MolotovIgnition.SeaLevel(m_Pos);
			if (m_Vel[1] < 0 && m_Pos[1] <= sea && sea > ground)
			{
				m_InWater = true;
				vector splash = m_Pos;
				splash[1] = sea;
				ParticleManager.GetInstance().PlayInWorld(ParticleList.IMPACT_WATER_LARGE_ENTER, splash);
				SEffectManager.PlaySound("Fishing_splash_SoundSet", splash).SetAutodestroy(true);
				m_Vel = m_Vel * 0.2;
				m_Spin = m_Spin * 0.15;
			}
			else if (m_Vel[1] < 0 && m_Pos[1] <= m_RestPos[1])
			{
				Finish();
				return;
			}
		}
		else
		{
			// through the water: slows to a steady sink, drifts to where it comes to rest,
			// turning to lie the way it ends up
			float sink = ENGINE_SINK;
			if (m_Speed <= 0)
				sink = HULL_SINK;
			float k = Math.Min(1, dt * 2);
			m_Vel[1] = m_Vel[1] + (-sink - m_Vel[1]) * k;
			m_Pos[1] = m_Pos[1] + m_Vel[1] * dt;
			m_Pos[0] = m_Pos[0] + (m_RestPos[0] - m_Pos[0]) * k;
			m_Pos[2] = m_Pos[2] + (m_RestPos[2] - m_Pos[2]) * k;
			for (int i = 0; i < 3; i++)
			{
				float d = m_RestOri[i] - m_Ori[i];
				while (d > 180)
					d -= 360;
				while (d < -180)
					d += 360;
				m_Ori[i] = m_Ori[i] + d * Math.Min(1, dt * 1.2);
			}
			if (m_Pos[1] <= m_RestPos[1])
			{
				Finish();
				return;
			}
		}
		if (m_Age > SETTLE_MS / 1000)
		{
			Finish();
			return;
		}
		Place();
	}

	protected void Place()
	{
		SetPosition(m_Pos);
		SetOrientation(m_Ori);
		Update();
	}

	protected void Finish()
	{
		GetGame().GetUpdateQueue(CALL_CATEGORY_GAMEPLAY).Remove(Tick);
		m_Pos = m_RestPos;
		m_Ori = m_RestOri;
		Place();
	}

	//! Server: the boat's gone up - swap it for its engine and hull, its cargo dropped in the water
	static void Sink(BoatScript boat)
	{
		if (!boat)
			return;
		vector pos = boat.GetPosition();
		vector ori = boat.GetOrientation();
		float sea = MolotovIgnition.SeaLevel(pos);

		// the hull: from where it floated, straight down
		vector hullOri = Vector(ori[0], 0, 0);
		MolotovSunk hull = MolotovSunk.Cast(GetGame().CreateObjectEx("MolotovBoatHull", pos, ECE_CREATEPHYSICS));
		if (hull)
		{
			hull.SetOrientation(hullOri + Vector(0, Math.RandomFloat(-4, 4), Math.RandomFloat(-4, 4)));
			vector rest = hull.Rest(pos);
			vector from = rest;
			from[1] = Math.Max(rest[1], sea - 0.15);
			hull.Launch(from, 0);
		}

		// the engine: off the stern at 45 degrees, out to sea if it can
		vector back = -boat.GetDirection();
		back[1] = 0;
		back.Normalize();
		vector start = pos + back * 1.9 + "0 1 0";
		vector landing = vector.Zero;
		vector dir = back;
		array<float> turns = {0, 40, -40, 80, -80, 120, -120, 180};
		foreach (float turn : turns)
		{
			vector d = vector.RotateAroundZeroDeg(back, vector.Up, turn);
			vector at = Landing(start, d, ENGINE_SPEED);
			if (MolotovIgnition.SeaLevel(at) > GetGame().SurfaceY(at[0], at[2]) + 0.3)
			{
				dir = d;
				landing = at;
				break;
			}
		}
		if (landing == vector.Zero)
			landing = Landing(start, dir, ENGINE_SPEED);
		// on its side on the bottom
		MolotovSunk engine = Throw("MolotovBoatEngine", start, dir, ENGINE_SPEED, Vector(Math.RandomFloat(-180, 180), Math.RandomFloat(-10, 10), 85));

		// whatever was in it goes in the water (not its own parts)
		array<string> slots = new array<string>;
		boat.ConfigGetTextArray("attachments", slots);
		foreach (int si, string sl : slots)
		{
			sl.ToLower();
			slots[si] = sl;
		}
		CargoBase cargo = boat.GetInventory().GetCargo();
		int dropped;
		if (cargo)
		{
			array<EntityAI> items = new array<EntityAI>;
			for (int c = 0; c < cargo.GetItemCount(); c++)
			{
				if (!MolotovWreck.IsPart(cargo.GetItem(c), slots))
					items.Insert(cargo.GetItem(c));
			}
			vector m[3];
			float quat[4];
			Math3D.YawPitchRollMatrix(ori, m);
			Math3D.MatrixToQuat(m, quat);
			foreach (EntityAI item : items)
			{
				vector p = pos + Vector(Math.RandomFloat(-1, 1), 0, Math.RandomFloat(-1, 1));
				p[1] = Math.Max(GetGame().SurfaceY(p[0], p[2]), sea) + 0.2;
				MiscGameplayFunctions.ThrowEntityFromInventory(item, p, quat, vector.Zero, ThrowEntityFlags.NONE);
				dropped++;
			}
		}
		string said = "[Molotov] " + boat.GetType() + " sank:";
		if (hull)
			said += " hull at " + hull.GetPosition();
		if (engine)
			said += ", engine thrown " + vector.Distance(start, landing) + " m to " + engine.GetPosition();
		Print(said + ", " + dropped + " things in the water");
		GetGame().ObjectDelete(boat);
	}

	//! Server: a motorbike that's gone up flies apart - its wheels, fenders, exhaust and
	//! shields every which way - leaving nothing but the fire
	static void Scatter(MotorbikeScript bike)
	{
		if (!bike)
			return;
		string model = "Motorbike_01";
		array<string> parts = {"Fender", "Exhaust"};
		if (bike.IsKindOf("Motorbike_02_ColorBase"))
		{
			model = "Motorbike_02";
			parts = {"FenderFront", "FenderRear", "Exhaust"};
		}
		// the fitted ones only: wheels and shields come off
		array<string> fitted = {"Wheel_1", "Wheel_2", "ShieldFront", "ShieldLeft", "ShieldRight"};
		foreach (string f : fitted)
		{
			if (bike.FindAttachmentBySlotName(model + "_" + f))
			{
				if (f == "ShieldFront")
					f = "Windshield";
				parts.Insert(f);
			}
		}
		vector pos = bike.GetPosition();
		vector fwd = bike.GetDirection();
		fwd[1] = 0;
		fwd.Normalize();
		vector centre = pos + "0 0.7 0";
		float spin = Math.RandomFloat(0, 360);
		string flung = "";
		for (int i = 0; i < parts.Count(); i++)
		{
			vector dir = vector.RotateAroundZeroDeg(fwd, vector.Up, spin + 360 * i / parts.Count() + Math.RandomFloat(-20, 20));
			vector restOri = Vector(Math.RandomFloat(-180, 180), Math.RandomFloat(-6, 6), Math.RandomFloat(-6, 6));
			if (parts[i].IndexOf("Wheel") == 0)
				restOri[2] = 90;   // a wheel lies flat
			if (Throw("MolotovDebris_" + model + "_" + parts[i], centre + dir * 0.3, dir, Math.RandomFloat(7, 11), restOri))
				flung += " " + parts[i];
		}
		Print("[Molotov] " + bike.GetType() + " flew apart:" + flung);
		GetGame().ObjectDelete(bike);
	}

	//! Server: a type blown off from start along dir at 45 degrees and speed, coming to rest
	//! (oriented restOri) where it lands - on the ground, or the bottom if that's water
	static MolotovSunk Throw(string type, vector start, vector dir, float speed, vector restOri)
	{
		vector landing = Landing(start, dir, speed);
		MolotovSunk thing = MolotovSunk.Cast(GetGame().CreateObjectEx(type, landing, ECE_CREATEPHYSICS));
		if (!thing)
		{
			Print("[Molotov] could not throw a " + type);
			return null;
		}
		thing.SetOrientation(restOri);
		thing.Rest(landing);
		thing.Launch(start, speed);
		return thing;
	}

	//! Where something launched from start along dir at 45 degrees comes down: on the
	//! water, or the ground if it gets there first
	static vector Landing(vector start, vector dir, float speed)
	{
		float c = speed * 0.7071;
		vector p = start;
		vector v = dir * c + Vector(0, c, 0);
		for (int i = 0; i < 500; i++)
		{
			v[1] = v[1] - GRAVITY * 0.02;
			p = p + v * 0.02;
			float floor = Math.Max(GetGame().SurfaceY(p[0], p[2]), MolotovIgnition.SeaLevel(p));
			if (v[1] < 0 && p[1] <= floor)
				break;
		}
		return p;
	}

	//! Server: settle on the bottom at xz, the collision box's floor on the ground
	vector Rest(vector at)
	{
		vector box[2];
		float lift = 0;
		if (GetCollisionBox(box))
		{
			vector lo = box[0];
			vector hi = box[1];
			// lying on its side, the box's x is what's up
			vector o = GetOrientation();
			if (o[2] > 45 || o[2] < -45)
				lift = (hi[0] - lo[0]) * 0.5;
			else
				lift = -lo[1];
		}
		at[1] = GetGame().SurfaceY(at[0], at[2]) + lift - 0.03;
		SetPosition(at);
		Update();
		return at;
	}
}

class MolotovBoatHull: MolotovSunk {}
class MolotovBoatEngine: MolotovSunk {}

//! Bits blown off a car going up (MolotovWreck): a burnt wheel and the hood
class MolotovDebris_HatchbackWheel: MolotovSunk {}
class MolotovDebris_HatchbackHood: MolotovSunk {}
class MolotovDebris_Hatchback_02_Wheel: MolotovSunk {}
class MolotovDebris_Hatchback_02_Hood: MolotovSunk {}
class MolotovDebris_CivSedanWheel: MolotovSunk {}
class MolotovDebris_CivSedanHood: MolotovSunk {}
class MolotovDebris_Sedan_02_Wheel: MolotovSunk {}
class MolotovDebris_Sedan_02_Hood: MolotovSunk {}
class MolotovDebris_Offroad_02_Wheel: MolotovSunk {}
class MolotovDebris_Offroad_02_Hood: MolotovSunk {}
class MolotovDebris_Truck_01_Wheel: MolotovSunk {}
class MolotovDebris_Truck_01_Hood: MolotovSunk {}

//! A motorbike flying apart (MolotovSunk.Scatter)
class MolotovDebris_Motorbike_01_Wheel_1: MolotovSunk {}
class MolotovDebris_Motorbike_01_Wheel_2: MolotovSunk {}
class MolotovDebris_Motorbike_01_Fender: MolotovSunk {}
class MolotovDebris_Motorbike_01_Exhaust: MolotovSunk {}
class MolotovDebris_Motorbike_02_Wheel_1: MolotovSunk {}
class MolotovDebris_Motorbike_02_Wheel_2: MolotovSunk {}
class MolotovDebris_Motorbike_02_FenderFront: MolotovSunk {}
class MolotovDebris_Motorbike_02_FenderRear: MolotovSunk {}
class MolotovDebris_Motorbike_02_Exhaust: MolotovSunk {}
class MolotovDebris_Motorbike_02_Windshield: MolotovSunk {}
class MolotovDebris_Motorbike_02_ShieldLeft: MolotovSunk {}
class MolotovDebris_Motorbike_02_ShieldRight: MolotovSunk {}
