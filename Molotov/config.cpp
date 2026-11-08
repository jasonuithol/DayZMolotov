class CfgPatches
{
	class Molotov
	{
		units[] = {"Molotov", "MolotovFire", "MolotovInferno", "MolotovBlast", "MolotovFlames", "MolotovAshes", "MolotovSmoulder", "MolotovBoatHull", "MolotovBoatEngine", "MolotovDebris_HatchbackWheel", "MolotovDebris_HatchbackHood", "MolotovDebris_Hatchback_02_Wheel", "MolotovDebris_Hatchback_02_Hood", "MolotovDebris_CivSedanWheel", "MolotovDebris_CivSedanHood", "MolotovDebris_Sedan_02_Wheel", "MolotovDebris_Sedan_02_Hood", "MolotovDebris_Offroad_02_Wheel", "MolotovDebris_Offroad_02_Hood", "MolotovDebris_Truck_01_Wheel", "MolotovDebris_Truck_01_Hood"};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DZ_Data", "DZ_Scripts", "DZ_Gear_Drinks", "DZ_Gear_Cooking"};
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

	//! A big, long fire: a burning car wreck or fuel pump
	class MolotovInferno: MolotovFire {};

	//! A fuelled car going up: the fuel pump explosion, then an inferno
	class MolotovBlast: MolotovInferno {};

	//! Flames on top of a tall burning object (no damage of its own)
	class MolotovFlames: MolotovInferno {};

	//! A ruin cooling down: smoke and a faint glow (particles and a light), invisible itself
	class MolotovSmoulder: HouseNoDestruct
	{
		scope = 1;
		model = "\DZ\data\lightpoint.p3d";
	};

	//! What's left of a boat that's gone up, sinking to the bottom (MolotovSunk.c). Not
	//! stored: gone at restart
	class MolotovBoatHull: HouseNoDestruct
	{
		scope = 1;
		model = "\Molotov\ruins\boat_hull_burnt.p3d";
	};
	class MolotovBoatEngine: HouseNoDestruct
	{
		scope = 1;
		model = "\Molotov\ruins\boat_engine_burnt.p3d";
	};

	//! Bits blown off a car going up (MolotovWreck, MolotovSunk.c): its ruined wheel, and its
	//! hood with the paint burnt off. Not stored: gone at restart
	class MolotovDebris_HatchbackWheel: HouseNoDestruct
	{
		scope = 1;
		model = "\DZ\vehicles\wheeled\OffroadHatchback\proxy\nivaWheel_destroyed.p3d";
	};
	class MolotovDebris_HatchbackHood: HouseNoDestruct
	{
		scope = 1;
		model = "\DZ\vehicles\wheeled\OffroadHatchback\proxy\nivaDoors_Hood.p3d";
		hiddenSelections[] = {"dmgZone_doors"};
		hiddenSelectionsMaterials[] = {"dz\vehicles\wheeled\offroadhatchback\data\green\niva_hood_destruct.rvmat"};
	};
	class MolotovDebris_Hatchback_02_Wheel: HouseNoDestruct
	{
		scope = 1;
		model = "\DZ\vehicles\wheeled\Hatchback_02\proxy\Hatchback_02_Wheel_ruined.p3d";
	};
	class MolotovDebris_Hatchback_02_Hood: HouseNoDestruct
	{
		scope = 1;
		model = "\DZ\vehicles\wheeled\Hatchback_02\proxy\Hatchback_02_Hood.p3d";
		hiddenSelections[] = {"dmgZone_doors"};
		hiddenSelectionsMaterials[] = {"dz\vehicles\wheeled\hatchback_02\data\red\hatchback_02_hood_destruct.rvmat"};
	};
	class MolotovDebris_CivSedanWheel: HouseNoDestruct
	{
		scope = 1;
		model = "\DZ\vehicles\wheeled\civiliansedan\proxy\sedanwheel_destroyed.p3d";
	};
	class MolotovDebris_CivSedanHood: HouseNoDestruct
	{
		scope = 1;
		model = "\DZ\vehicles\wheeled\civilianSedan\proxy\sedanhood.p3d";
		hiddenSelections[] = {"dmgZone_doors"};
		hiddenSelectionsMaterials[] = {"dz\vehicles\wheeled\civiliansedan\data\white\civiliansedan_destruct.rvmat"};
	};
	class MolotovDebris_Sedan_02_Wheel: HouseNoDestruct
	{
		scope = 1;
		model = "\DZ\vehicles\wheeled\Sedan_02\proxy\Sedan_02_Wheel_ruined.p3d";
	};
	class MolotovDebris_Sedan_02_Hood: HouseNoDestruct
	{
		scope = 1;
		model = "\DZ\vehicles\wheeled\Sedan_02\proxy\Sedan_02_Hood.p3d";
		hiddenSelections[] = {"dmgZone_doors"};
		hiddenSelectionsMaterials[] = {"dz\vehicles\wheeled\sedan_02\data\yellow\sedan_02_hood_destruct.rvmat"};
	};
	class MolotovDebris_Offroad_02_Wheel: HouseNoDestruct
	{
		scope = 1;
		model = "\DZ\vehicles\wheeled\Offroad_02\proxy\Offroad_02_Wheel_destroyed.p3d";
	};
	class MolotovDebris_Offroad_02_Hood: HouseNoDestruct
	{
		scope = 1;
		model = "\DZ\vehicles\wheeled\Offroad_02\proxy\Offroad_02_Hood.p3d";
		hiddenSelections[] = {"dmgZone_doors"};
		hiddenSelectionsMaterials[] = {"dz\vehicles\wheeled\offroad_02\data\offroad_02_base_multi_destruct.rvmat"};
	};
	class MolotovDebris_Truck_01_Wheel: HouseNoDestruct
	{
		scope = 1;
		model = "\DZ\vehicles\wheeled\Truck_01\proxy\Truck_01_Wheel_ruined.p3d";
	};
	class MolotovDebris_Truck_01_Hood: HouseNoDestruct
	{
		scope = 1;
		model = "\DZ\vehicles\wheeled\Truck_01\proxy\Truck_01_Hood.p3d";
		hiddenSelections[] = {"dmgZone_doors", "camo_door"};
		hiddenSelectionsTextures[] = {"#(argb,8,8,3)color(0.08,0.07,0.06,1.0,co)", "#(argb,8,8,3)color(0.08,0.07,0.06,1.0,co)"};
		hiddenSelectionsMaterials[] = {"dz\vehicles\wheeled\Truck_01\data\Truck_01_hood_destruct.rvmat", "dz\vehicles\wheeled\Truck_01\data\Truck_01_hood_destruct.rvmat"};
	};

	//! A heap of ash (and some burnt logs) where something burned away: the vanilla
	//! fireplace model with only its ashes and burnt wood showing. Not stored, so it's gone
	//! at restart along with the burnt-out object coming back
	class MolotovAshes: HouseNoDestruct
	{
		scope = 1;
		model = "\dz\gear\cooking\Fireplace.p3d";
		hiddenSelections[] = {"ashes", "inventory"};
		hiddenSelectionsTextures[] = {"\dz\gear\cooking\data\stoneground_co.paa", ""};
		hiddenSelectionsMaterials[] = {"\dz\gear\cooking\data\stonegroundnoemit.rvmat", ""};
		class AnimationSources
		{
			class Ashes { source = "user"; animPeriod = 0.01; initPhase = 1; };
			class Kindling { source = "user"; animPeriod = 0.01; initPhase = 1; };
			class Sticks { source = "user"; animPeriod = 0.01; initPhase = 1; };
			class Wood { source = "user"; animPeriod = 0.01; initPhase = 1; };
			class BurntWood { source = "user"; animPeriod = 0.01; initPhase = 1; };
			class Stone { source = "user"; animPeriod = 0.01; initPhase = 1; };
			class StonesCircle { source = "user"; animPeriod = 0.01; initPhase = 1; };
			class Tripod { source = "user"; animPeriod = 0.01; initPhase = 1; };
			class Oven { source = "user"; animPeriod = 0.01; initPhase = 1; };
			class LidOn { source = "user"; animPeriod = 0.01; initPhase = 1; };
			class LidOff { source = "user"; animPeriod = 0.01; initPhase = 1; };
		};
	};
};
