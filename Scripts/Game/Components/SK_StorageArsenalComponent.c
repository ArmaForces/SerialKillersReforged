[EntityEditorProps("SerialKillers", description: "Inventory storage arsenal component.")]
class SK_StorageArsenalComponentClass : ScriptComponentClass
{
}

class SK_StorageArsenalComponent : ScriptComponent
{
	[Attribute("0", uiwidget: UIWidgets.Slider, desc: "How many items will be stored in the.", params: "0 50 1")]
	protected int m_iItemQuantity;

	[Attribute("", desc: "Toggle supported SCR_EArsenalItemType by this arsenal, items are gathered from SCR_Faction or from the overwrite config", uiwidget: UIWidgets.Flags, enums: ParamEnumArray.FromEnum(SCR_EArsenalItemType))]
	protected SCR_EArsenalItemType m_eSupportedArsenalItemTypes;

	[Attribute("", desc: "Toggle supported SCR_EArsenalItemMode by this arsenal, items are gathered from SCR_Faction or from the overwrite config", uiwidget: UIWidgets.Flags, enums: ParamEnumArray.FromEnum(SCR_EArsenalItemMode))]
	protected SCR_EArsenalItemMode m_eSupportedArsenalItemModes;

	protected bool m_bAreItemsAssigned = false;

	bool AssignItems(out array<SCR_ArsenalItem> items, bool isSpawnGuaranateed)
	{
		if (m_bAreItemsAssigned)
			return false;

		if (!isSpawnGuaranateed && IsSlotInvalid())
			return false;
		

		array<SCR_ArsenalItem> filteredArsenalItems = {};
		if (!GetFiliteredItems(filteredArsenalItems))
			return false;
		
		//ESCT_Logger.InfoFormat("[ESCT_StorageArsenalComponent] Storage on %1 position - %2 available item count.", GetOwner().GetOrigin().ToString(), filteredArsenalItems.Count().ToString()); 

		int availableItemCount = filteredArsenalItems.Count();
		int itemsToFill = GetItemCount();
		
		//ESCT_Logger.InfoFormat("[ESCT_StorageArsenalComponent] About to fill storage on %1 position with %2 items.", GetOwner().GetOrigin().ToString(), itemsToFill.ToString()); 
		
		if (itemsToFill == 0)
			return false;

		for (int i = 0; i < itemsToFill; i++)
		{
			float randomValue = Math.RandomFloat01();

			for (int j = 0; j < availableItemCount; j++)
			{
				if (randomValue <= 0.5)
				{
					if (SCR_Enum.HasPartialFlag(filteredArsenalItems[j].GetItemMode(), SCR_EArsenalItemMode.AMMUNITION) 
						&& !SCR_Enum.HasPartialFlag(filteredArsenalItems[j].GetItemType(), SCR_EArsenalItemType.EQUIPMENT))
					{
						int insertCount = Math.RandomInt(2, 5);
						for (int q = 1; q <= insertCount; q++)
						{
							items.Insert(filteredArsenalItems[j]);
						}
					}
					else 
					{
						items.Insert(filteredArsenalItems[j]);
					}
					
					break;
				}
			}
		}

		m_bAreItemsAssigned = true;

		return !items.IsEmpty();
	}

	protected bool IsSlotInvalid()
	{
		/*
		float spawnChance = ESCT_EscapistsConfigComponent.GetInstance().GetItemSpawnChance();
		if (ESCT_EscapistsGameMode.GetGameMode().GetRandomGenerator().RandFloat01() > spawnChance)
		{
			return true;
		}*/

		return false;
	}

	protected bool GetFiliteredItems(out notnull array<SCR_ArsenalItem> filteredArsenalItems)
	{
		//~ Cannot get items if no entity catalog manager
		SCR_EntityCatalogManagerComponent catalogManager = SCR_EntityCatalogManagerComponent.GetInstance();
		if (!catalogManager)
			return false;

		//~ Get filtered list from faction (if any) or default
		SCR_Faction redfor = SCR_Faction.Cast(GetGame().GetFactionManager().GetFactionByKey("USSR"));
		filteredArsenalItems = catalogManager.GetFilteredArsenalItems(m_eSupportedArsenalItemTypes, m_eSupportedArsenalItemModes, -1, redfor);

		return !filteredArsenalItems.IsEmpty();
	}

	protected int GetItemCount()
	{
		return m_iItemQuantity;
	}
}
