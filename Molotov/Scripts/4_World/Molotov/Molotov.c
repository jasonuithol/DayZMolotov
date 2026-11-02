//! Tunables
class MolotovSettings
{
	static const int FUSE_MS = 30000;            //!< a lit molotov bursts on its own after this
	static const int FIRE_MS = 20000;            //!< how long the pool of fire burns
	static const float FIRE_RADIUS = 1.75;       //!< half-width of the burning area, metres
	static const float CAR_DAMAGE_PER_S = 0.05;  //!< fraction of a car's engine/fuel tank health per second
	// Burn damage per second to whoever stands in the fire. Applied straight to health,
	// not as a hit: clothing soaked up nearly all of a melee-type "fire" hit.
	static const float PLAYER_DPS = 10;          //!< players have 100 health
	static const float PLAYER_SHOCK_PS = 10;
	static const float CREATURE_DPS = 25;        //!< infected and animals
	static const float FIRE_HEIGHT = 1.5;        //!< standing on a roof above it doesn't burn
	static const float MIN_FUEL_ML = 200;        //!< fuel a bottle needs to become a molotov
}

class Molotov: Inventory_Base
{
	protected bool m_Lit;
	protected bool m_Burst;

	protected Particle m_Flame;
	protected ScriptedLightBase m_Light;
	protected EffectSound m_Sound;

	void Molotov()
	{
		RegisterNetSyncVariableBool("m_Lit");
		SetEventMask(EntityEvent.CONTACT);
	}

	override void SetActions()
	{
		super.SetActions();
		AddAction(ActionLightMolotov);
		AddAction(ActionLightItemOnFire);   // molotov in hands, lighter on the ground
	}

	bool IsLit()
	{
		return m_Lit;
	}

	// --- ignition (vanilla ActionLightItemOnFire calls these) ---------------------------

	override bool HasFlammableMaterial()
	{
		return true;
	}

	override bool CanBeIgnitedBy(EntityAI igniter = null)
	{
		return !m_Lit && !IsRuined();
	}

	override bool IsIgnited()
	{
		return m_Lit;
	}

	override void OnIgnitedThis(EntityAI fire_source)
	{
		Light();
	}

	void Light()
	{
		if (!GetGame().IsServer() || m_Lit)
			return;
		m_Lit = true;
		SetSynchDirty();
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(Burst, MolotovSettings.FUSE_MS, false);
	}

	// --- a lit one can't be stowed --------------------------------------------------------

	override bool CanPutInCargo(EntityAI parent)
	{
		return !m_Lit && super.CanPutInCargo(parent);
	}

	override bool CanPutAsAttachment(EntityAI parent)
	{
		return !m_Lit && super.CanPutAsAttachment(parent);
	}

	// --- bursting -------------------------------------------------------------------------

	override void EOnContact(IEntity other, Contact extra)
	{
		if (GetGame().IsServer() && m_Lit && !GetHierarchyParent() && !m_Burst)
		{
			Object hit = Object.Cast(other);
			string what = "the ground";
			if (hit)
				what = hit.GetType();
			Print("[Molotov] burst on " + what + " at " + GetPosition());
			Burst();
		}
	}

	override void EEHealthLevelChanged(int oldLevel, int newLevel, string zone)
	{
		super.EEHealthLevelChanged(oldLevel, newLevel, zone);
		if (GetGame().IsServer() && m_Lit && newLevel == GameConstants.STATE_RUINED)
			Burst();
	}

	//! Server: leave a pool of fire where the bottle is (at the feet of whoever holds it)
	void Burst()
	{
		if (!GetGame().IsServer() || m_Burst)
			return;
		m_Burst = true;
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).Remove(Burst);

		vector pos = GetPosition();
		EntityAI root = GetHierarchyRoot();
		if (root && root != this)
			pos = root.GetPosition();
		pos[1] = GetGame().SurfaceY(pos[0], pos[2]);
		GetGame().CreateObjectEx("MolotovFire", pos, ECE_CREATEPHYSICS | ECE_UPDATEPATHGRAPH);
		DeleteSafe();
	}

	// --- client effects -------------------------------------------------------------------

	override void OnVariablesSynchronized()
	{
		super.OnVariablesSynchronized();
		if (m_Lit && !m_Flame)
			StartFlame();
	}

	void StartFlame()
	{
		if (GetGame().IsDedicatedServer())
			return;
		vector tip = GetMemoryPointPos("rag_tip");   // top of the rag plug
		m_Flame = ParticleManager.GetInstance().PlayOnObject(ParticleList.TORCH_T1, this, tip);
		m_Light = ScriptedLightBase.CreateLight(TorchLight, "0 0 0");
		m_Light.SetRadiusTo(6);
		m_Light.AttachOnObject(this, tip + "0 0.05 0");
		PlaySoundSetLoop(m_Sound, "LightFire_SoundSet", 0.3, 0.3);
	}

	void StopFlame()
	{
		if (m_Flame)
			m_Flame.Stop();
		m_Flame = null;
		if (m_Light)
			m_Light.FadeOut(0);
		m_Light = null;
		if (m_Sound)
			StopSoundSet(m_Sound);
	}

	override void EEDelete(EntityAI parent)
	{
		super.EEDelete(parent);
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).Remove(Burst);
		StopFlame();
	}
}

//! The pool of burning fuel: hurts whoever stands in it, damages cars, then goes out
class MolotovFire: House
{
	static const float TICK_S = 0.5;
	protected int m_Ticks;
	protected ref array<Particle> m_Flames;
	protected ScriptedLightBase m_Light;
	protected EffectSound m_Sound;
	protected EffectSound m_Smash;

	override void EEInit()
	{
		super.EEInit();
		if (GetGame().IsServer())
			StartServer();
		if (!GetGame().IsDedicatedServer())
			StartEffects();
	}

	void StartServer()
	{
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(Burn, TICK_S * 1000, true);
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(GoOut, MolotovSettings.FIRE_MS, false);
		Print("[Molotov] fire at " + GetPosition());
	}

	//! Every tick: burn players, infected and animals standing in the fire; every second
	//! also the engine and fuel tank of any car parked in or over it
	void Burn()
	{
		m_Ticks++;
		bool carTick = m_Ticks % 2 == 0;
		vector c = GetPosition();
		float r = MolotovSettings.FIRE_RADIUS;
		array<Object> near = new array<Object>;
		GetGame().GetObjectsAtPosition(c, r + 2, near, null);
		foreach (Object o : near)
		{
			CarScript car = CarScript.Cast(o);
			if (car)
			{
				if (carTick)
				{
					Print("[Molotov] burning " + car.GetType() + ": engine " + car.GetHealth("Engine", "Health") + ", fuel tank " + car.GetHealth("FuelTank", "Health"));
					car.AddHealth("Engine", "Health", -car.GetMaxHealth("Engine", "Health") * MolotovSettings.CAR_DAMAGE_PER_S);
					car.AddHealth("FuelTank", "Health", -car.GetMaxHealth("FuelTank", "Health") * MolotovSettings.CAR_DAMAGE_PER_S);
				}
				continue;
			}

			EntityAI e = EntityAI.Cast(o);
			if (!e || !e.IsAlive() || !(e.IsInherited(PlayerBase) || e.IsInherited(ZombieBase) || e.IsInherited(AnimalBase)))
				continue;
			vector p = e.GetPosition();
			if (vector.DistanceSq(Vector(p[0], 0, p[2]), Vector(c[0], 0, c[2])) > r * r)
				continue;
			if (p[1] < c[1] - 0.5 || p[1] > c[1] + MolotovSettings.FIRE_HEIGHT)
				continue;

			if (e.IsInherited(PlayerBase))
			{
				e.AddHealth("", "Health", -MolotovSettings.PLAYER_DPS * TICK_S);
				e.AddHealth("", "Shock", -MolotovSettings.PLAYER_SHOCK_PS * TICK_S);
			}
			else
			{
				e.AddHealth("", "Health", -MolotovSettings.CREATURE_DPS * TICK_S);
			}
			if (!e.IsAlive())
				Print("[Molotov] " + e.GetType() + " burned to death");
		}
	}

	void GoOut()
	{
		GetGame().ObjectDelete(this);
	}

	void StartEffects()
	{
		vector c = GetPosition();
		m_Flames = new array<Particle>;
		m_Flames.Insert(ParticleManager.GetInstance().PlayInWorld(ParticleList.CAMP_NORMAL_FIRE, c));
		m_Flames.Insert(ParticleManager.GetInstance().PlayInWorld(ParticleList.CAMP_NORMAL_SMOKE, c + "0 0.5 0"));
		float r = MolotovSettings.FIRE_RADIUS * 0.7;
		for (int i = 0; i < 6; i++)
		{
			float a = i * Math.PI2 / 6 + Math.RandomFloat(-0.3, 0.3);
			float d = r * Math.RandomFloat(0.6, 1.1);
			vector p = c + Vector(Math.Cos(a) * d, 0, Math.Sin(a) * d);
			p[1] = GetGame().SurfaceY(p[0], p[2]);
			m_Flames.Insert(ParticleManager.GetInstance().PlayInWorld(ParticleList.CAMP_SMALL_FIRE, p));
		}

		m_Light = ScriptedLightBase.CreateLight(FireplaceLight, c + "0 0.6 0");
		PlaySoundSet(m_Smash, "Impact_glass_ground_SoundSet", 0, 0);
		PlaySoundSetLoop(m_Sound, "HeavyFire_SoundSet", 0.2, 1.0);
	}

	override void EEDelete(EntityAI parent)
	{
		super.EEDelete(parent);
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).Remove(Burn);
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).Remove(GoOut);
		if (m_Flames)
		{
			foreach (Particle f : m_Flames)
			{
				if (f)
					f.Stop();
			}
		}
		if (m_Light)
			m_Light.FadeOut(1);
		if (m_Sound)
			StopSoundSet(m_Sound);
	}
}
