class CfgPatches
{
	class Molotov
	{
		units[] = {"Molotov", "MolotovFire"};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DZ_Data", "DZ_Scripts", "DZ_Gear_Drinks"};
	};
};

class CfgMods
{
	class Molotov
	{
		dir = "Molotov";
		name = "Molotov";
		type = "mod";
		dependencies[] = {"World"};

		class defs
		{
			class worldScriptModule
			{
				value = "";
				files[] = {"Molotov/Scripts/4_World"};
			};
			class missionScriptModule
			{
				value = "";
				files[] = {"Molotov/Scripts/5_Mission"};
			};
		};
	};
};

class CfgVehicles
{
	class Inventory_Base;
	class HouseNoDestruct;

	//! A vodka bottle of petrol with a rag in the neck (our own model: vanilla bottle texture, a petrol-soaked rag, fuel inside). Not a liquid container, so it
	//! offers none of the drink/pour/wash actions.
	class Molotov: Inventory_Base
	{
		scope = 2;
		displayName = "Molotov Cocktail";
		descriptionShort = "A glass bottle of fuel with a rag stuffed in the neck. Light the rag, then throw it - quickly.";
		model = "\Molotov\molotov.p3d";
		weight = 900;
		itemSize[] = {1, 3};
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints = 50;
					healthLevels[] = {{1.0, {"DZ\gear\drinks\data\VodkaBottles.rvmat", "Molotov\data\molotov_rag.rvmat"}}, {0.7, {"DZ\gear\drinks\data\VodkaBottles.rvmat", "Molotov\data\molotov_rag.rvmat"}}, {0.5, {"DZ\gear\drinks\data\VodkaBottles_damage.rvmat", "DZ\gear\consumables\data\rags_bandages_damage.rvmat"}}, {0.3, {"DZ\gear\drinks\data\VodkaBottles_damage.rvmat", "DZ\gear\consumables\data\rags_bandages_damage.rvmat"}}, {0.0, {"DZ\gear\drinks\data\VodkaBottles_destruct.rvmat", "DZ\gear\consumables\data\rags_bandages_destruct.rvmat"}}};
				};
			};
		};
	};

	//! The pool of burning fuel a molotov leaves; invisible, the flames are particles
	class MolotovFire: HouseNoDestruct
	{
		scope = 1;
		model = "\DZ\data\lightpoint.p3d";
	};
};
