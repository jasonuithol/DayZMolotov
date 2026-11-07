//! A lit molotov that hits (or bursts beside) an unlit fireplace is both the lighter and the
//! kindling: its rag goes in as kindling if there isn't any (a fireplace always holds
//! something - an empty one is gone), then it lights and burns the normal way: wind, rain
//! and wet wood still put it out. No action - you light fires with what lit the molotov.
modded class FireplaceBase
{
	bool MolotovCanLight()
	{
		return !IsBurning() && !GetHierarchyParent() && IsOpen() && !IsOnWaterSurface();
	}

	void MolotovLight(EntityAI source)
	{
		if (!GetGame().IsServer() || !MolotovCanLight())
			return;
		if (!HasAnyKindling())
			GetInventory().CreateAttachment("Rag");
		Print("[Molotov] lighting " + GetType() + " at " + GetPosition() + ", kindling " + HasAnyKindling());
		OnIgnitedThis(source);
	}
}
