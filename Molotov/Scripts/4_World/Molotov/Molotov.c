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

	// Fuel going up: a car with fuel in the tank, or a working fuel pump, caught in a fire.
	// The explosion itself - and the wreck, the sinking boat, the flying bits - is
	// BetterExplosions'; what's ours is the fire it leaves behind
	static const int COOKOFF_MS = 3000;          //!< from catching fire to the explosion
	static const int INFERNO_MS = 60000;         //!< how long a wreck or pump burns afterwards
	static const float INFERNO_RADIUS = 3.5;
	static const float VEHICLE_DAMAGE_PER_S = 0.05;   //!< an empty boat or bike in the fire: engine (or whole) health per second

	// Fire spreading from one burning thing to whatever is close by (a molotov's own pool
	// always lights what it reaches). One roll per neighbour per fire.
	static const float SPREAD_CHANCE = 0.35;     //!< dry weather, at the edge of the fire's reach
	static const float SPREAD_CHANCE_CLOSE = 0.8; //!< dry weather, touching the flames
	static const float RAIN_LIGHT = 0.05;        //!< rain from here: half the chance
	static const float RAIN_HEAVY = 0.5;         //!< rain from here: none
	static const int MAX_SPREADING = 24;         //!< things lit by spreading, burning at once (a forest fire creeps, it doesn't explode)
	static const int SPREAD_DELAY_MIN_MS = 3000; //!< a neighbour takes this long, up to the max, to catch (or not):
	static const int SPREAD_DELAY_MAX_MS = 7000; //!< the fire creeps across rather than jumping
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

	//! not by a hand drill: it takes an ember and tinder to get a flame, not something you
	//! hold to a rag soaked in petrol
	override bool CanBeIgnitedBy(EntityAI igniter = null)
	{
		if (igniter && igniter.IsInherited(HandDrillKit))
			return false;
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
			BurstOn(hit);
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
		BurstOn(null);
	}

	//! ... and whatever it broke on is in the fire, however big it is or high it stands
	//! (a boat up on a jetty, the roof of a shed)
	void BurstOn(Object hit)
	{
		if (!GetGame().IsServer() || m_Burst)
			return;
		m_Burst = true;
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).Remove(Burst);

		vector pos = GetPosition();
		EntityAI root = GetHierarchyRoot();
		if (root && root != this)
			pos = root.GetPosition();
		pos = MolotovIgnition.Ground(pos);
		MolotovFire fire = MolotovFire.Cast(GetGame().CreateObjectEx("MolotovFire", pos, ECE_CREATEPHYSICS | ECE_UPDATEPATHGRAPH));
		if (fire && hit && !hit.IsInherited(DayZCreature) && !hit.IsMan())
			MolotovIgnition.Reach(hit, fire, true);
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
//! What a molotov fire sets alight, and how
class MolotovIgnition
{
	//! Map objects that burn, by model file: every hay and straw mesh, the stacked-log
	//! woodpiles, pallets, the wooden sheds, and the hunters' feeding rack, feed shack and
	//! deer stands (when the fire's out they're gone until restart - see MolotovGone)
	static const ref array<string> BURNING_MODELS = {
		"farm_strawstack.p3d", "misc_haybale.p3d", "misc_haybale_decayed.p3d", "misc_haybale_packed.p3d", "misc_haybale_packed_stack.p3d", "misc_haystack.p3d",
		"misc_woodpile1.p3d", "misc_woodpile2.p3d", "misc_woodpile_forest1.p3d", "misc_woodpile_forest2.p3d", "misc_woodpile_forest3.p3d",
		"misc_pallet.p3d", "misc_pallets1.p3d", "misc_pallets2.p3d", "misc_pallets3.p3d",
		"shed_w1.p3d", "shed_w2.p3d", "shed_w3.p3d", "shed_w4.p3d", "shed_w5.p3d", "shed_w6.p3d",
		"misc_feedrack.p3d", "misc_feedshack.p3d", "misc_deerstand1.p3d", "misc_deerstand2.p3d"
	};
	//! Classes that burn: the wooden sheds, the outhouse, the wooden guard shed, and the
	//! lumber pile (a CE event, so it burns away - see BURNS_AWAY)
	static const ref array<string> BURNING_TYPES = {
		"Land_Shed_W1", "Land_Shed_W2", "Land_Shed_W3", "Land_Shed_W4", "Land_Shed_W5", "Land_Shed_W6",
		"Land_Misc_Toilet_Dry", "Land_Mil_GuardShed", "PileOfWoodenPlanks"
	};
	//! Networked things that burn away (deleted the usual way); everything else that burns
	//! is a map object, gone until restart through MolotovGone
	static const ref array<string> BURNS_AWAY = {"PileOfWoodenPlanks"};
	static const float INFERNO_SPACING = 7;   //!< a big object (a 20 m straw stack) burns as a row of fires this far apart

	//! Burning or burnt out (until restart, so neighbours can't keep relighting each other),
	//! and vehicles/pumps counting down to going up
	protected static ref array<Object> s_Alight = new array<Object>;
	//! The same for map objects without a config class (hay, woodpiles): the engine hands
	//! out a new script object for those each time, so they go by model and position
	protected static ref array<string> s_AlightStatic = new array<string>;

	static string Key(Object o)
	{
		return o.GetShapeName() + o.GetPosition().ToString();
	}

	static bool IsAlight(Object o)
	{
		if (EntityAI.Cast(o))
			return s_Alight.Find(o) >= 0;
		return s_AlightStatic.Find(Key(o)) >= 0;
	}

	//! The chance of a fire spreading to something close by, given the rain (0..1) and how
	//! close it is (0: touching the flames .. 1: at the edge of their reach): from
	//! SPREAD_CHANCE at the edge to SPREAD_CHANCE_CLOSE touching
	static float s_ChanceOverride = -1;   //!< the self-test pins it
	static float SpreadChance(float rain, float farness = 1)
	{
		if (s_ChanceOverride >= 0)
			return s_ChanceOverride;
		farness = Math.Clamp(farness, 0, 1);
		float chance = MolotovSettings.SPREAD_CHANCE_CLOSE + (MolotovSettings.SPREAD_CHANCE - MolotovSettings.SPREAD_CHANCE_CLOSE) * farness;
		if (rain >= MolotovSettings.RAIN_HEAVY)
			return 0;
		if (rain >= MolotovSettings.RAIN_LIGHT)
			return chance * 0.5;
		return chance;
	}

	protected static int s_Spreading;   //!< things lit by spreading, still burning
	static void SpreadOut()
	{
		s_Spreading--;
	}

	//! Marks o as alight; false if it already was
	static bool Catch(Object o)
	{
		if (EntityAI.Cast(o))
		{
			if (s_Alight.Find(o) >= 0)
				return false;
			s_Alight.Insert(o);
			return true;
		}
		string key = Key(o);
		if (s_AlightStatic.Find(key) >= 0)
			return false;
		s_AlightStatic.Insert(key);
		return true;
	}

	//! Ground level - or the water's surface (sea with its tide and waves, or a pond), so a
	//! fire on a boat floats
	static vector Ground(vector p)
	{
		p[1] = GetGame().SurfaceY(p[0], p[2]);
		float depth = GetGame().GetWaterDepth(p);   // how far that point is under a pond
		if (depth > 0)
			p[1] = p[1] + depth;
		float sea = SeaLevel(p);                    // (GetWaterDepth doesn't see the sea)
		if (sea > p[1])
			p[1] = sea;
		return p;
	}

	//! The sea's surface over p (it rises and falls), or far below if p isn't over the sea
	static float SeaLevel(vector p)
	{
		if (GetGame().SurfaceIsSea(p[0], p[2]))
			return GetGame().SurfaceGetSeaLevel();
		return -1000;
	}

	//! Is the fire at c within reach of o? Measured to o's collision box, not its centre:
	//! a straw stack is 20 m long, a car 4 m
	static bool InReach(Object o, vector c, float reach)
	{
		return Gap(o, c) <= reach;
	}

	//! How far c is from o's nearest side (its collision box; else its bounding sphere)
	static float Gap(Object o, vector c)
	{
		vector box[2];
		if (o.GetCollisionBox(box))
		{
			vector lo = box[0];
			vector hi = box[1];
			vector lp = o.WorldToModel(c);
			for (int i = 0; i < 3; i++)
				lp[i] = Math.Clamp(lp[i], lo[i], hi[i]);
			return vector.Distance(o.ModelToWorld(lp), c);
		}
		vector clip[2];
		float radius = o.ClippingInfo(clip);
		return Math.Max(0, vector.Distance(o.GetPosition(), c) - radius);
	}

	static bool IsFlammable(Object o)
	{
		if (o.IsTree() || o.IsBush())
			return !o.IsDamageDestroyed();   // a burnt (or felled) one has gone: it reports a new position, so it'd catch again
		if (BURNING_TYPES.Find(o.GetType()) >= 0)
			return true;
		string shape = o.GetShapeName();
		if (shape == "")
			return false;
		shape.ToLower();
		foreach (string model : BURNING_MODELS)
		{
			if (shape.Contains("\\" + model))
				return true;
		}
		return Wreck(shape) != "";
	}

	//! Our own burnt-out remains (blender/build_ruins.py), for the big things with no ruin
	//! of their own in the game: Molotov\ruins\<model>_burnt.p3d
	static const ref array<string> OWN_RUINS = {
		"shed_w1", "shed_w2", "shed_w3", "shed_w5", "shed_w6", "misc_feedshack", "farm_strawstack", "misc_deerstand1", "misc_deerstand2"
	};

	//! What a burnt-out object leaves standing: the wooden houses (house_1w01-1w13,
	//! house_2w01-2w05 and their colour variants) the matching Livonia ruin, the big sheds,
	//! hunter's shack, straw stack and deer stands our own; "" for anything else (it leaves ash)
	static string Wreck(string shape)
	{
		shape.ToLower();
		foreach (string own : OWN_RUINS)
		{
			if (shape.Contains("\\" + own + ".p3d"))
				return "Molotov\\ruins\\" + own + "_burnt.p3d";
		}
		int at = shape.IndexOf("\\house_");
		if (at < 0 || shape.Length() < at + 11)
			return "";
		string tag = shape.Substring(at + 7, 4);   // 1w01
		string storeys = tag.Substring(0, 2);
		if (storeys != "1w" && storeys != "2w")
			return "";
		return "dz\\structures_bliss\\ruins\\ruin_house_" + tag + ".p3d";
	}

	static bool HasFuel(Transport t)
	{
		return BetterExplosions.HasFuel(t);
	}

	//! Server: one tick of a fire reaching o
	static void Reach(Object o, MolotovFire fire, bool slowTick)
	{
		// from a burning thing (not a molotov's pool), catching is a roll of the dice
		MolotovInferno from = MolotovInferno.Cast(fire);
		Land_FuelStation_Feed pump = Land_FuelStation_Feed.Cast(o);
		if (pump)
		{
			if (!pump.IsRuined() && !IsAlight(pump) && (!from || from.MaySpreadTo(pump)))
				CookOff(pump);
			return;
		}
		Transport t = Transport.Cast(o);
		if (t)
		{
			if (HasFuel(t))
			{
				if (!IsAlight(t) && (!from || from.MaySpreadTo(t)))
					CookOff(t);
			}
			else if (slowTick)
			{
				SlowBurn(t);
			}
			return;
		}
		FireplaceBase fp = FireplaceBase.Cast(o);
		if (fp)
		{
			if (fp.MolotovCanLight() && (!from || from.MaySpreadTo(fp)))
				fp.MolotovLight(fire);
			return;
		}
		if (IsFlammable(o) && !IsAlight(o) && (!from || from.MaySpreadTo(o)))
			Kindle(o);
	}

	//! Server: something caught from a neighbour - it counts against MAX_SPREADING while it burns
	static bool StartSpreading()
	{
		if (s_Spreading >= MolotovSettings.MAX_SPREADING)
			return false;
		s_Spreading++;
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(MolotovIgnition.SpreadOut, MolotovSettings.INFERNO_MS + MolotovSettings.COOKOFF_MS, false);
		return true;
	}

	//! An empty vehicle in the fire: cars lose engine and fuel tank, others engine (or all)
	static void SlowBurn(Transport t)
	{
		if (t.IsRuined())
			return;
		if (CarScript.Cast(t))
		{
			Print("[Molotov] burning " + t.GetType() + ": engine " + t.GetHealth("Engine", "Health") + ", fuel tank " + t.GetHealth("FuelTank", "Health"));
			t.AddHealth("Engine", "Health", -t.GetMaxHealth("Engine", "Health") * MolotovSettings.CAR_DAMAGE_PER_S);
			t.AddHealth("FuelTank", "Health", -t.GetMaxHealth("FuelTank", "Health") * MolotovSettings.CAR_DAMAGE_PER_S);
		}
		else if (t.GetMaxHealth("Engine", "Health") > 0)
		{
			t.AddHealth("Engine", "Health", -t.GetMaxHealth("Engine", "Health") * MolotovSettings.VEHICLE_DAMAGE_PER_S);
		}
		else
		{
			t.AddHealth("", "Health", -t.GetMaxHealth("", "Health") * MolotovSettings.VEHICLE_DAMAGE_PER_S);
		}
	}

	//! Hay, a woodpile, a wooden shed or a lumber pile catches: a big fire, no explosion
	static void Kindle(Object o)
	{
		if (!Catch(o))
			return;
		// a row of fires along the object's long side, one per INFERNO_SPACING metres
		vector box[2];
		vector centre = "0 0 0";
		vector axis = "0 0 0";
		int count = 1;
		if (o.GetCollisionBox(box))
		{
			vector lo = box[0];
			vector hi = box[1];
			centre = (lo + hi) * 0.5;
			float lx = hi[0] - lo[0];
			float lz = hi[2] - lo[2];
			if (lx >= lz)
				axis = Vector(lx, 0, 0);
			else
				axis = Vector(0, 0, lz);
			count = Math.Max(1, Math.Round(Math.Max(lx, lz) / INFERNO_SPACING));
		}
		Print("[Molotov] " + o.GetType() + o.GetShapeName() + " caught fire at " + o.GetPosition() + ": " + count + " fires");
		// each fire reaches across the whole width and up over the top, so there's nowhere
		// on or beside a burning stack or shed to stand; flames show on top of tall ones
		float halfWidth = 0;
		float top = 0;
		if (o.GetCollisionBox(box))
		{
			vector blo = box[0];
			vector bhi = box[1];
			halfWidth = Math.Min(bhi[0] - blo[0], bhi[2] - blo[2]) * 0.5;
			top = bhi[1];
		}
		float radius = Math.Max(MolotovSettings.INFERNO_RADIUS, halfWidth + 1.5);
		for (int i = 0; i < count; i++)
		{
			vector lp = centre + axis * ((i + 0.5) / count - 0.5);
			vector pos = Ground(o.ModelToWorld(lp));
			MolotovInferno inferno = MolotovInferno.Cast(GetGame().CreateObjectEx("MolotovInferno", pos, ECE_CREATEPHYSICS | ECE_UPDATEPATHGRAPH));
			if (!inferno)
				continue;
			vector topAt = o.ModelToWorld(Vector(lp[0], top, lp[2]));
			float tall = topAt[1] - pos[1];
			inferno.SetSize(radius, Math.Max(MolotovSettings.FIRE_HEIGHT, tall + 1.5));
			if (i == 0 && BURNS_AWAY.Find(o.GetType()) >= 0)
				inferno.SetBurnsAway(o);
			else if (i == 0)
				inferno.SetMapObjectGone(o.GetShapeName(), o.GetPosition());
			if (tall > 1.5)
				GetGame().CreateObjectEx("MolotovFlames", topAt - "0 0.3 0", ECE_CREATEPHYSICS);
		}
	}

	//! A fuelled vehicle or a working fuel pump has caught fire: it goes up shortly
	static void CookOff(Object o)
	{
		if (!Catch(o))
			return;
		Print("[Molotov] " + o.GetType() + " caught fire at " + o.GetPosition());
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(MolotovIgnition.GoUp, MolotovSettings.COOKOFF_MS, false, o);
	}

	//! A vehicle or pump that caught fire goes up (BetterExplosions), and burns where it was
	static void GoUp(Object o)
	{
		s_Alight.RemoveItem(o);   // a ruined pump or vehicle never goes up again anyway
		if (!o)
			return;
		vector pos = Ground(o.GetPosition());
		bool vehicle = Transport.Cast(o) != null;
		if (!BetterExplosions.Explode(o))
			return;
		if (vehicle)
			GetGame().CreateObjectEx("MolotovBlast", pos, ECE_CREATEPHYSICS | ECE_UPDATEPATHGRAPH);
		else
			GetGame().CreateObjectEx("MolotovInferno", pos, ECE_CREATEPHYSICS | ECE_UPDATEPATHGRAPH);
	}
}

//! The pool of burning fuel: hurts whoever stands in it, sets off what burns, then goes out
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
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(GoOut, Duration(), false);
		Print("[Molotov] " + GetType() + " at " + GetPosition());
	}

	float Radius()
	{
		return MolotovSettings.FIRE_RADIUS;
	}

	//! How high above the fire it still burns (a man standing in it, not one on a roof)
	float Height()
	{
		return MolotovSettings.FIRE_HEIGHT;
	}

	int Duration()
	{
		return MolotovSettings.FIRE_MS;
	}

	//! Every tick: burn players, infected and animals standing in the fire, and set alight
	//! whatever burns in reach (see MolotovIgnition.Reach); empty vehicles every second
	void Burn()
	{
		m_Ticks++;
		bool slowTick = m_Ticks % 2 == 0;
		vector c = GetPosition();
		float r = Radius();
		array<Object> near = new array<Object>;
		GetGame().GetObjectsAtPosition(c, r + 12, near, null);   // wide: big objects reach in from further
		foreach (Object o : near)
		{
			if (o == this || o.IsInherited(MolotovFire))
				continue;
			EntityAI e = EntityAI.Cast(o);
			if (!e || !(e.IsInherited(PlayerBase) || e.IsInherited(ZombieBase) || e.IsInherited(AnimalBase)))
			{
				if (MolotovIgnition.InReach(o, c, r + 1))
					MolotovIgnition.Reach(o, this, slowTick);
				continue;
			}
			if (!e.IsAlive())
				continue;
			vector p = e.GetPosition();
			if (vector.DistanceSq(Vector(p[0], 0, p[2]), Vector(c[0], 0, c[2])) > r * r)
				continue;
			if (p[1] < c[1] - 0.5 || p[1] > c[1] + Height())
				continue;

			PlayerBase pb = PlayerBase.Cast(e);
			if (pb)
			{
				MolotovPain.Burn(pb, this, MolotovSettings.PLAYER_DPS * TICK_S, MolotovSettings.PLAYER_SHOCK_PS * TICK_S);
			}
			else
			{
				e.AddHealth("", "Health", -MolotovSettings.CREATURE_DPS * TICK_S);
				// a vanilla heat hit: what makes them flinch (its own damage is a token 1)
				if (e.IsAlive())
					e.ProcessDirectDamage(DamageType.CUSTOM, this, "", "HeatDamage", "0.5 0.5 0.5", 1);
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
		float r = Radius() * 0.7;
		for (int i = 0; i < 6; i++)
		{
			float a = i * Math.PI2 / 6 + Math.RandomFloat(-0.3, 0.3);
			float d = r * Math.RandomFloat(0.6, 1.1);
			vector p = c + Vector(Math.Cos(a) * d, 0, Math.Sin(a) * d);
			p = MolotovIgnition.Ground(p);
			m_Flames.Insert(ParticleManager.GetInstance().PlayInWorld(ParticleList.CAMP_SMALL_FIRE, p));
		}

		m_Light = ScriptedLightBase.CreateLight(FireplaceLight, c + "0 0.6 0");
		PlaySoundSet(m_Smash, "offroad_hit_window_small_SoundSet", 0, 0);   // glass breaking (there is no bottle-smash sound)
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

//! A big fire that burns for a minute: a burning wreck, fuel pump, hay, shed or woodpile
class MolotovInferno: MolotovFire
{
	protected Object m_BurnsAway;   //!< deleted when the fire goes out (a lumber pile)
	protected string m_GoneModel;   //!< a map object (shed, hay, pallets): gone when the fire goes out, until restart
	protected vector m_GoneAt;
	protected float m_Radius;       //!< 0: INFERNO_RADIUS; wider for a wide burning object (synced, for the flames)
	protected float m_Height;       //!< server: 0: FIRE_HEIGHT; up over the top of a burning object
	protected bool m_Widened;

	void MolotovInferno()
	{
		RegisterNetSyncVariableFloat("m_Radius", 0, 50, 2);
	}

	void SetBurnsAway(Object o)
	{
		m_BurnsAway = o;
	}

	protected ref map<string, bool> m_Rolled = new map<string, bool>;   //!< neighbours already rolled for
	protected ref map<string, int> m_RollAt = new map<string, int>;     //!< neighbours waiting: when they roll

	//! Server: does this fire spread to o, close by? A few seconds after it first reaches
	//! it (SPREAD_DELAY_*), one roll: the chance halved in light rain and none in heavy
	//! (MolotovSettings.SPREAD_CHANCE)
	bool MaySpreadTo(Object o)
	{
		string key = MolotovIgnition.Key(o);
		if (m_Rolled.Contains(key))
			return false;   // rolled already: a hit has caught by now
		int now = GetGame().GetTime();
		if (!m_RollAt.Contains(key))
		{
			m_RollAt.Set(key, now + Math.RandomIntInclusive(MolotovSettings.SPREAD_DELAY_MIN_MS, MolotovSettings.SPREAD_DELAY_MAX_MS));
			return false;
		}
		if (now < m_RollAt.Get(key))
			return false;   // still taking hold
		float rain = GetGame().GetWeather().GetRain().GetActual();
		float reach = Radius() + 1;
		float gap = MolotovIgnition.Gap(o, GetPosition());
		float chance = MolotovIgnition.SpreadChance(rain, gap / reach);
		bool caught = Math.RandomFloat01() < chance;
		if (caught && !MolotovIgnition.StartSpreading())
		{
			// too much burning already: try again on a later tick
			return false;
		}
		m_Rolled.Set(key, caught);
		Print("[Molotov] fire at " + GetPosition() + " spreads to " + o.GetShapeName() + " at " + o.GetPosition() + ": " + caught + " (chance " + chance + ", " + gap + " m off, rain " + rain + ")");
		return caught;
	}

	//! by model and position: map objects keep no stable script object to hold on to
	void SetMapObjectGone(string model, vector at)
	{
		m_GoneModel = model;
		m_GoneAt = at;
	}

	//! Server: cover a burning object - reach out to radius, and up to height above the ground
	void SetSize(float radius, float height)
	{
		m_Radius = radius;
		m_Height = height;
		SetSynchDirty();
	}

	override float Radius()
	{
		if (m_Radius > 0)
			return m_Radius;
		return MolotovSettings.INFERNO_RADIUS;
	}

	override float Height()
	{
		if (m_Height > 0)
			return m_Height;
		return MolotovSettings.FIRE_HEIGHT;
	}

	//! Client: the width arrives after the flames started - spread a ring out to it
	override void OnVariablesSynchronized()
	{
		super.OnVariablesSynchronized();
		if (m_Widened || !m_Flames || m_Radius <= MolotovSettings.INFERNO_RADIUS)
			return;
		m_Widened = true;
		vector c = GetPosition();
		for (int i = 0; i < 10; i++)
		{
			float a = i * Math.PI2 / 10 + Math.RandomFloat(-0.2, 0.2);
			float d = m_Radius * Math.RandomFloat(0.6, 0.9);
			vector p = MolotovIgnition.Ground(c + Vector(Math.Cos(a) * d, 0, Math.Sin(a) * d));
			m_Flames.Insert(ParticleManager.GetInstance().PlayInWorld(ParticleList.CAMP_NORMAL_FIRE, p));
		}
	}

	override int Duration()
	{
		return MolotovSettings.INFERNO_MS;
	}

	override void GoOut()
	{
		if (m_BurnsAway)
		{
			Print("[Molotov] " + m_BurnsAway.GetType() + " burned away at " + m_BurnsAway.GetPosition());
			MolotovAshes.Spread(m_BurnsAway);
			GetGame().ObjectDelete(m_BurnsAway);
		}
		if (m_GoneModel != "")
			MolotovGone.BurnAway(m_GoneModel, m_GoneAt);
		super.GoOut();
	}

	override void StartEffects()
	{
		vector c = GetPosition();
		m_Flames = new array<Particle>;
		m_Flames.Insert(ParticleManager.GetInstance().PlayInWorld(ParticleList.BONFIRE_FIRE, c));
		m_Flames.Insert(ParticleManager.GetInstance().PlayInWorld(ParticleList.BONFIRE_SMOKE, c + "0 1 0"));
		m_Flames.Insert(ParticleManager.GetInstance().PlayInWorld(ParticleList.SMOKE_GENERIC_WRECK, c + "0 0.5 0"));
		float r = Radius() * 0.6;
		for (int i = 0; i < 8; i++)
		{
			float a = i * Math.PI2 / 8 + Math.RandomFloat(-0.25, 0.25);
			float d = r * Math.RandomFloat(0.5, 1.1);
			vector p = c + Vector(Math.Cos(a) * d, 0, Math.Sin(a) * d);
			p = MolotovIgnition.Ground(p);
			m_Flames.Insert(ParticleManager.GetInstance().PlayInWorld(ParticleList.CAMP_NORMAL_FIRE, p));
		}

		m_Light = ScriptedLightBase.CreateLight(FireplaceLight, c + "0 1.5 0");
		m_Light.SetRadiusTo(25);
		m_Light.SetBrightnessTo(2);
		PlaySoundSetLoop(m_Sound, "HeavyFire_SoundSet", 0.2, 1.0);
	}
}

//! The fire a fuelled vehicle leaves when it goes up (the explosion and fireball are
//! BetterExplosions')
class MolotovBlast: MolotovInferno {}

//! Flames only, on top of a tall burning object (the inferno below it does the burning)
class MolotovFlames: MolotovInferno
{
	override void StartServer()
	{
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(GoOut, Duration(), false);
	}

	override void StartEffects()
	{
		vector c = GetPosition();
		m_Flames = new array<Particle>;
		m_Flames.Insert(ParticleManager.GetInstance().PlayInWorld(ParticleList.BONFIRE_FIRE, c));
		for (int i = 0; i < 3; i++)
		{
			float a = i * Math.PI2 / 3 + Math.RandomFloat(-0.4, 0.4);
			m_Flames.Insert(ParticleManager.GetInstance().PlayInWorld(ParticleList.CAMP_NORMAL_FIRE, c + Vector(Math.Cos(a) * 1.5, 0, Math.Sin(a) * 1.5)));
		}
	}
}
