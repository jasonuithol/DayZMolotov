// Optional add-on: DayZ Expansion's vehicles burn down to the matching vanilla wreck when a
// molotov sets them off, like the vanilla cars do, and fling a burnt wheel (and the hood)
// off. Server and client (-mod), after Molotov and DayZ-Expansion-Vehicles: the flying
// pieces are our own classes, built from Expansion's (and vanilla's) part models.
class CfgPatches
{
	class Molotov_Expansion
	{
		units[] = {"MolotovDebris_ExpTractorWheel", "MolotovDebris_ExpTractorFrontWheel", "MolotovDebris_ExpTractorDoor", "MolotovDebris_ExpBusWheel", "MolotovDebris_ExpBusRearWheel", "MolotovDebris_ExpBusDoorsLeft", "MolotovDebris_ExpBusDoorsRight", "MolotovDebris_ExpBusEngineCover", "MolotovDebris_ExpUAZWheel", "MolotovDebris_ExpUAZHood", "MolotovDebris_ExpUAZDoor", "MolotovDebris_ExpLandroverWheel", "MolotovDebris_ExpLandroverHood", "MolotovDebris_ExpLandroverDoor", "MolotovDebris_ExpUh1hDoor", "MolotovDebris_ExpUh1hDoor2", "MolotovDebris_RotorBlade"};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"Molotov"};
	};
};

class CfgMods
{
	class Molotov_Expansion
	{
		dir = "Molotov_Expansion";
		name = "Molotov - Expansion vehicles";
		type = "mod";
		dependencies[] = {"World", "Mission"};

		class defs
		{
			class worldScriptModule
			{
				value = "";
				files[] = {"Molotov_Expansion/Scripts/4_World"};
			};
			class missionScriptModule
			{
				value = "";
				files[] = {"Molotov_Expansion/Scripts/5_Mission"};
			};
		};
	};
};

// Flung off a vehicle that goes up (MolotovSunk, from Molotov): models only, no physics of
// their own. Gone at restart. Expansion's part models, the bus's from vanilla's unreleased
// Ikarus (the same bus), and Molotov's own burnt rotor blade.
class CfgVehicles
{
	class HouseNoDestruct;
	class MolotovDebris_ExpTractorWheel: HouseNoDestruct
	{
		scope = 1;
		model = "\DayZExpansion\Vehicles\Ground\Tractor\proxy\tractor2_back_wheel_destroyed.p3d";
	};
	class MolotovDebris_ExpTractorFrontWheel: HouseNoDestruct
	{
		scope = 1;
		model = "\DayZExpansion\Vehicles\Ground\Tractor\proxy\tractor2_front_wheel_destroyed.p3d";
	};
	class MolotovDebris_ExpTractorDoor: HouseNoDestruct
	{
		scope = 1;
		model = "\DayZExpansion\Vehicles\Ground\Tractor\proxy\ExpansionTractorDoorsDriver.p3d";
	};
	class MolotovDebris_ExpBusWheel: HouseNoDestruct
	{
		scope = 1;
		model = "\DZ\vehicles\wheeled\TransitBus\proxy\ikarusWheel_destroyed.p3d";
	};
	class MolotovDebris_ExpBusRearWheel: HouseNoDestruct
	{
		scope = 1;
		model = "\DZ\vehicles\wheeled\TransitBus\proxy\ikarusWheel_rear_destroyed.p3d";
	};
	class MolotovDebris_ExpBusDoorsLeft: HouseNoDestruct
	{
		scope = 1;
		model = "\DZ\vehicles\wheeled\TransitBus\proxy\IkarusDoors_Left.p3d";
	};
	class MolotovDebris_ExpBusDoorsRight: HouseNoDestruct
	{
		scope = 1;
		model = "\DZ\vehicles\wheeled\TransitBus\proxy\IkarusDoors_Right.p3d";
	};
	class MolotovDebris_ExpBusEngineCover: HouseNoDestruct
	{
		scope = 1;
		model = "\DZ\vehicles\wheeled\TransitBus\proxy\ikarusHood.p3d";
	};
	class MolotovDebris_ExpUAZWheel: HouseNoDestruct
	{
		scope = 1;
		model = "\DayZExpansion\Vehicles\Ground\UAZ\proxy\UAZWheel_Destroyed.p3d";
	};
	class MolotovDebris_ExpUAZHood: HouseNoDestruct
	{
		scope = 1;
		model = "\DayZExpansion\Vehicles\Ground\UAZ\proxy\UAZHoodDoor_Wreck.p3d";
	};
	class MolotovDebris_ExpUAZDoor: HouseNoDestruct
	{
		scope = 1;
		model = "\DayZExpansion\Vehicles\Ground\UAZ\proxy\uazdriverdoor.p3d";
	};
	class MolotovDebris_ExpLandroverWheel: HouseNoDestruct
	{
		scope = 1;
		model = "\DayZExpansion\Vehicles\Ground\LandRover\proxy\landrover_wheel_destroyed.p3d";
	};
	class MolotovDebris_ExpLandroverHood: HouseNoDestruct
	{
		scope = 1;
		model = "\DayZExpansion\Vehicles\Ground\LandRover\proxy\expansion_landrover_hood.p3d";
		hiddenSelections[] = {"body_outdoor"};
		hiddenSelectionsMaterials[] = {"DayZExpansion\Vehicles\Ground\LandRover\data\damage\lr_body_outdoor_destruct.rvmat"};
	};
	class MolotovDebris_ExpLandroverDoor: HouseNoDestruct
	{
		scope = 1;
		model = "\DayZExpansion\Vehicles\Ground\LandRover\proxy\expansion_landrover_driverdoor.p3d";
	};
	class MolotovDebris_ExpUh1hDoor: HouseNoDestruct
	{
		scope = 1;
		model = "\DayZExpansion\Vehicles\Air\Uh1h\proxy\uh1hdoor_1_1.p3d";
	};
	class MolotovDebris_ExpUh1hDoor2: HouseNoDestruct
	{
		scope = 1;
		model = "\DayZExpansion\Vehicles\Air\Uh1h\proxy\uh1hdoor_2_1.p3d";
	};
	class MolotovDebris_RotorBlade: HouseNoDestruct
	{
		scope = 1;
		model = "\Molotov\ruins\rotor_blade_burnt.p3d";
	};
};
