// Molotov - Expansion Vehicles: kept so servers that run it keep working. What it did - the
// Expansion vehicles' wrecks and the burnt bits flung off them - is now in the Better
// Explosions - Expansion Vehicles add-on (BetterExplosions_Expansion), which this requires.
// Nothing of its own: with BetterExplosions_Expansion loaded you can drop this from -mod.
class CfgPatches
{
	class Molotov_Expansion
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"Molotov", "BetterExplosions_Expansion"};
	};
};
class CfgMods
{
	class Molotov_Expansion
	{
		dir = "Molotov_Expansion";
		name = "Molotov - Expansion vehicles";
		type = "mod";
	};
};
