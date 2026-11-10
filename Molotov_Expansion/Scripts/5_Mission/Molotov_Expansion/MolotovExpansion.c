// The Expansion vehicles that have a vanilla wreck of the same model. The vehicles are named
// as strings, so nothing here needs Expansion's scripts: without Expansion they never match.
// turn is how far the wreck's model is turned from the vehicle's (the vanilla wreck props
// all face backwards: 180, checked in game). Then the burnt bits flung off, in order: off one
// side, the front, the other side, the back, anywhere (true: a wheel or door, standing on
// edge in its model, so it comes down on its side).
modded class MissionServer
{
	override void OnInit()
	{
		super.OnInit();
		MolotovWreck.Add("ExpansionTractor", "Land_Wreck_Tractor", 180);
		MolotovWreck.Fling("ExpansionTractor", "MolotovDebris_ExpTractorWheel", true);
		MolotovWreck.Fling("ExpansionTractor", "MolotovDebris_ExpTractorFrontWheel", true);
		MolotovWreck.Fling("ExpansionTractor", "MolotovDebris_ExpTractorDoor", true);

		MolotovWreck.Add("ExpansionBus", "Land_Wreck_Ikarus", 180);
		MolotovWreck.Fling("ExpansionBus", "MolotovDebris_ExpBusWheel", true);
		MolotovWreck.Fling("ExpansionBus", "MolotovDebris_ExpBusDoorsLeft", true);
		MolotovWreck.Fling("ExpansionBus", "MolotovDebris_ExpBusRearWheel", true);
		MolotovWreck.Fling("ExpansionBus", "MolotovDebris_ExpBusEngineCover");
		MolotovWreck.Fling("ExpansionBus", "MolotovDebris_ExpBusDoorsRight", true);
		MolotovWreck.Fling("ExpansionBus", "MolotovDebris_ExpBusWheel", true);

		MolotovWreck.Add("ExpansionUAZBase", "Land_Wreck_Uaz", 180);
		MolotovWreck.Fling("ExpansionUAZBase", "MolotovDebris_ExpUAZWheel", true);
		MolotovWreck.Fling("ExpansionUAZBase", "MolotovDebris_ExpUAZHood");
		MolotovWreck.Fling("ExpansionUAZBase", "MolotovDebris_ExpUAZWheel", true);
		MolotovWreck.Fling("ExpansionUAZBase", "MolotovDebris_ExpUAZDoor", true);

		MolotovWreck.Add("Expansion_Landrover_Base", "Land_Wreck_LR_TK", 180);
		MolotovWreck.Fling("Expansion_Landrover_Base", "MolotovDebris_ExpLandroverWheel", true);
		MolotovWreck.Fling("Expansion_Landrover_Base", "MolotovDebris_ExpLandroverHood");
		MolotovWreck.Fling("Expansion_Landrover_Base", "MolotovDebris_ExpLandroverWheel", true);
		MolotovWreck.Fling("Expansion_Landrover_Base", "MolotovDebris_ExpLandroverDoor", true);

		MolotovWreck.Add("ExpansionUh1h", "Wreck_UH1Y", 180);
		MolotovWreck.Fling("ExpansionUh1h", "MolotovDebris_ExpUh1hDoor", true);
		MolotovWreck.Fling("ExpansionUh1h", "MolotovDebris_RotorBlade");
		MolotovWreck.Fling("ExpansionUh1h", "MolotovDebris_ExpUh1hDoor2", true);
		MolotovWreck.Fling("ExpansionUh1h", "MolotovDebris_RotorBlade");
	}
}
