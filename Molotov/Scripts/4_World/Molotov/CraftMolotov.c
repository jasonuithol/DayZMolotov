//! Glass bottle with at least MIN_FUEL_ML of petrol or diesel + a rag -> molotov.
//! The bottle (and its fuel) becomes the molotov; one rag from the stack goes in the neck.
class CraftMolotov: RecipeBase
{
	override void Init()
	{
		m_Name = "Make a molotov";
		m_IsInstaRecipe = false;
		m_AnimationLength = 1.0;
		m_Specialty = 0.02;

		m_MinDamageIngredient[0] = -1;
		m_MaxDamageIngredient[0] = 3;
		m_MinQuantityIngredient[0] = MolotovSettings.MIN_FUEL_ML;
		m_MaxQuantityIngredient[0] = -1;

		m_MinDamageIngredient[1] = -1;
		m_MaxDamageIngredient[1] = 3;
		m_MinQuantityIngredient[1] = 1;
		m_MaxQuantityIngredient[1] = -1;

		InsertIngredient(0, "GlassBottle");
		m_IngredientAddHealth[0] = 0;
		m_IngredientSetHealth[0] = -1;
		m_IngredientAddQuantity[0] = 0;
		m_IngredientDestroy[0] = true;
		m_IngredientUseSoftSkills[0] = false;

		InsertIngredient(1, "Rag");
		m_IngredientAddHealth[1] = 0;
		m_IngredientSetHealth[1] = -1;
		m_IngredientAddQuantity[1] = -1;
		m_IngredientDestroy[1] = false;
		m_IngredientUseSoftSkills[1] = false;

		AddResult("Molotov");
		m_ResultSetFullQuantity[0] = false;
		m_ResultSetQuantity[0] = -1;
		m_ResultSetHealth[0] = -1;
		m_ResultInheritsHealth[0] = 0;
		m_ResultInheritsColor[0] = -1;
		m_ResultToInventory[0] = -1;
		m_ResultUseSoftSkills[0] = false;
		m_ResultReplacesIngredient[0] = -1;
	}

	override bool CanDo(ItemBase ingredients[], PlayerBase player)
	{
		ItemBase bottle = ingredients[0];
		if (!bottle || bottle.GetQuantity() < MolotovSettings.MIN_FUEL_ML)
			return false;
		int liquid = bottle.GetLiquidType();
		return liquid == LIQUID_GASOLINE || liquid == LIQUID_DIESEL;
	}
}

modded class PluginRecipesManagerBase
{
	override void RegisterRecipies()
	{
		super.RegisterRecipies();
		RegisterRecipe(new CraftMolotov);
	}
}

modded class ActionConstructor
{
	override void RegisterActions(TTypenameArray actions)
	{
		super.RegisterActions(actions);
		actions.Insert(ActionLightMolotov);
	}
}
