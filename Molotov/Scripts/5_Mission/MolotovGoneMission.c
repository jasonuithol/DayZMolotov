//! Players joining after something burnt away get told it's gone, and clients act on
//! the server telling them (see MolotovGone)
modded class MissionServer
{
	//! Whatever goes up burns with our fire, not Better Explosions' own
	override void OnInit()
	{
		super.OnInit();
		BetterExplosions.s_OwnFire = false;
		BetterExplosions.Event_OnExploded.Insert(MolotovIgnition.OnExploded);
	}

	override void InvokeOnConnect(PlayerBase player, PlayerIdentity identity)
	{
		super.InvokeOnConnect(player, identity);
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(MolotovGone.SendAll, 3000, false, identity);
	}
}

modded class MissionGameplay
{
	void MissionGameplay()
	{
		DayZGame.Event_OnRPC.Insert(MolotovGone.OnGameRPC);
		DayZGame.Event_OnRPC.Insert(MolotovPain.OnGameRPC);
	}
}
