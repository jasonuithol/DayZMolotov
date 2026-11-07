//! Light the rag of the molotov in your hands with a lighter or matches (or anything else
//! that can light it - not a hand drill) from your inventory. The igniter uses a match / some fuel as usual.
class ActionLightMolotov: ActionSingleUseBase
{
	void ActionLightMolotov()
	{
		m_Text = "Light the rag";
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_LIGHTFLARE;
		m_CommandUIDProne = DayZPlayerConstants.CMD_ACTIONFB_LITCHEMLIGHT;
	}

	override void CreateConditionComponents()
	{
		m_ConditionItem = new CCINonRuined();
		m_ConditionTarget = new CCTNone();
	}

	override bool HasTarget()
	{
		return false;
	}

	override bool HasProneException()
	{
		return true;
	}

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		Molotov molotov = Molotov.Cast(item);
		return molotov && !molotov.IsLit() && FindIgniter(player, molotov);
	}

	override void OnExecuteServer(ActionData action_data)
	{
		Molotov molotov = Molotov.Cast(action_data.m_MainItem);
		if (!molotov || molotov.IsLit())
			return;
		ItemBase igniter = FindIgniter(action_data.m_Player, molotov);
		if (!igniter)
			return;
		igniter.OnIgnitedTarget(molotov);
		molotov.OnIgnitedThis(igniter);
	}

	static ItemBase FindIgniter(PlayerBase player, Molotov molotov)
	{
		array<EntityAI> items = new array<EntityAI>;
		player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);
		foreach (EntityAI e : items)
		{
			ItemBase ib = ItemBase.Cast(e);
			if (ib && ib != molotov && !ib.IsRuined() && ib.CanIgniteItem(molotov) && molotov.CanBeIgnitedBy(ib))
				return ib;
		}
		return null;
	}
}
