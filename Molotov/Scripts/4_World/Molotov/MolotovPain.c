//! A player in the flames: the damage (straight to health and shock - clothing soaks up
//! nearly all of a hit), a vanilla heat hit that makes them flinch, a cry of pain now and
//! then, the red flash on their screen, and the health display kept up to date. A burn that
//! would kill holds them at the last sliver of health a moment first, so the display has
//! drained to the bottom before the screen goes black.
class MolotovPain
{
	static const int RPC_BURNT = 0x4D4F4C02;   //!< "MOL" 2: server -> the burning player: flash red
	static const int CRY_MS = 1500;            //!< at most one cry of pain this often
	static const int LAST_BREATH_MS = 700;     //!< held at the last sliver of health before dying
	static const float LAST_SLIVER = 0.5;

	protected static ref map<int, int> s_LastCry = new map<int, int>;
	protected static ref array<PlayerBase> s_Dying = new array<PlayerBase>;

	//! Server: one tick of burning
	static void Burn(PlayerBase p, EntityAI fire, float health, float shock)
	{
		if (!p || !p.IsAlive() || s_Dying.Find(p) >= 0)
			return;
		bool dies = p.GetHealth("", "Health") - health <= LAST_SLIVER;
		if (dies)
			p.SetHealth("", "Health", LAST_SLIVER);
		else
			p.AddHealth("", "Health", -health);
		p.AddHealth("", "Shock", -shock);
		// the flinch (its own damage is a token 1)
		p.ProcessDirectDamage(DamageType.CUSTOM, fire, "", "HeatDamage", "0.5 0.5 0.5", 1);
		int now = GetGame().GetTime();
		int key = p.GetID();
		if (!s_LastCry.Contains(key) || now - s_LastCry.Get(key) >= CRY_MS)
		{
			s_LastCry.Set(key, now);
			p.RequestSoundEvent(EPlayerSoundEventID.TAKING_DMG_HEAVY);
		}
		if (p.GetIdentity())
			GetGame().RPCSingleParam(p, RPC_BURNT, null, true, p.GetIdentity());
		if (p.m_NotifiersManager)
			p.m_NotifiersManager.RequestImmediateUpdate(eNotifiers.NTF_HEALTHY);
		if (dies)
		{
			s_Dying.Insert(p);
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(MolotovPain.Die, LAST_BREATH_MS, false, p);
		}
	}

	static void Die(PlayerBase p)
	{
		s_Dying.RemoveItem(p);
		if (p && p.IsAlive())
		{
			p.SetHealth("", "Health", 0);
			Print("[Molotov] " + p.GetType() + " burned to death");
		}
	}

	//! Client: the server says we're burning - the red flash a hit gives
	static void OnGameRPC(PlayerIdentity sender, Object target, int rpc_type, ParamsReadContext ctx)
	{
		if (rpc_type != RPC_BURNT || GetGame().IsDedicatedServer())
			return;
		PlayerBase p = PlayerBase.Cast(target);
		if (p && p == GetGame().GetPlayer())
			p.SpawnDamageDealtEffect();
	}
}
