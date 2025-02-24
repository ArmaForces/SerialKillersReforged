class SK_StorageCacheComponentClass : ScriptComponentClass
{
}

class SK_StorageCacheComponent : ScriptComponent
{
	protected SCR_InventoryStorageManagerComponent m_InventoryStorageManager;
	protected SCR_UniversalInventoryStorageComponent m_InventoryStorage;
	
	[Attribute("0", uiwidget: UIWidgets.Slider, desc: "How many items will be stored in the.", params: "0 50 1")]
	protected int m_iItemQuantity;
	
	[Attribute("USSR", desc:"Which faction should be used as a source")]
	protected string m_sFactionItemKey;
	
	[Attribute("", desc: "Toggle supported SCR_EArsenalItemType by this arsenal, items are gathered from SCR_Faction or from the overwrite config", uiwidget: UIWidgets.Flags, enums: ParamEnumArray.FromEnum(SCR_EArsenalItemType))]
	protected SCR_EArsenalItemType m_eSupportedArsenalItemTypes;

	[Attribute("", desc: "Toggle supported SCR_EArsenalItemMode by this arsenal, items are gathered from SCR_Faction or from the overwrite config", uiwidget: UIWidgets.Flags, enums: ParamEnumArray.FromEnum(SCR_EArsenalItemMode))]
	protected SCR_EArsenalItemMode m_eSupportedArsenalItemModes;
	
	
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (SCR_Global.IsEditMode() || !Replication.IsServer()) return;

		SetEventMask(owner, EntityEvent.INIT);
	}
	

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		m_InventoryStorageManager = SCR_InventoryStorageManagerComponent.Cast(owner.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!m_InventoryStorageManager)
			Print("SCR_InventoryStorageManagerComponent component could not be found on SK_StorageCacheComponent entity!", LogLevel.ERROR);

		m_InventoryStorage = SCR_UniversalInventoryStorageComponent.Cast(owner.FindComponent(SCR_UniversalInventoryStorageComponent));
		if (!m_InventoryStorage)
			Print("SCR_UniversalInventoryStorageComponent component could not be found on SK_StorageCacheComponent entity!", LogLevel.ERROR);
		
		GetGame().GetCallqueue().CallLater(InitializeCache, Math.RandomInt(1,10) * 1000);
	}

	void InitializeCache()
	{
		array<SCR_ArsenalItem> items = {};
		if (!GetItems(items))
		{
			Print("Failed to get random item for supply cache!", LogLevel.ERROR);
			return;
		}

		PrintFormat("Spawning %1 items in supply cache", items.Count(), level: LogLevel.WARNING);
		foreach (SCR_ArsenalItem item : items)
		{
			ResourceName itemName = item.GetItemResourceName();
			bool result = m_InventoryStorageManager.TrySpawnPrefabToStorage(itemName, m_InventoryStorage);
			if (!result) 
			{
				PrintFormat("Unable to spawn resource %1. Error code %2", itemName, m_InventoryStorageManager.GetReturnCode(), level: LogLevel.WARNING);
			}
		}
		CreateMapMarker();
	}
	
	protected bool GetItems(out notnull array<SCR_ArsenalItem> items)
	{
		array<SCR_ArsenalItem> filteredArsenalItems = {};
		SCR_EntityCatalogManagerComponent catalogManager = SCR_EntityCatalogManagerComponent.GetInstance();
		if (!catalogManager)
			return false;

		SCR_Faction faction = SCR_Faction.Cast(GetGame().GetFactionManager().GetFactionByKey(m_sFactionItemKey));
		filteredArsenalItems = catalogManager.GetFilteredArsenalItems(m_eSupportedArsenalItemTypes, m_eSupportedArsenalItemModes, -1, faction);
		
		int availableItemCount = filteredArsenalItems.Count();
		int itemsToFill = GetItemCount();
		
		if (itemsToFill == 0)
			return false;

		//TODO! Refactor me
		for (int i = 0; i < itemsToFill; i++)
		{
			int j = filteredArsenalItems.GetRandomIndex();
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
			
		}

		return !items.IsEmpty();
	}

	protected int GetItemCount()
	{
		return m_iItemQuantity;
	}
	
	protected void CreateMapMarker()
	{
		SCR_MapMarkerManagerComponent mapMarkerMgr = SCR_MapMarkerManagerComponent.Cast(GetGame().GetGameMode().FindComponent(SCR_MapMarkerManagerComponent));
		if (!mapMarkerMgr)
		{
			Print("MamMarkerMgr not found", LogLevel.ERROR);
			return;
		}
		
		SCR_MapMarkerBase m_MapMarker = new SCR_MapMarkerBase();
		m_MapMarker = mapMarkerMgr.PrepareMilitaryMarker(EMilitarySymbolIdentity.OPFOR, EMilitarySymbolDimension.LAND, EMilitarySymbolIcon.SUPPLY);
		vector worldPos = GetOwner().GetOrigin();
		m_MapMarker.SetWorldPos(worldPos[0], worldPos[2]);
		m_MapMarker.SetCustomText("Supply cache");
		
		FactionManager factionManager = GetGame().GetFactionManager();
		
		if (factionManager)
		{
			Faction faction = factionManager.GetFactionByKey(m_sFactionItemKey);
			if (faction) {
				m_MapMarker.AddMarkerFactionFlags(factionManager.GetFactionIndex(faction));
				mapMarkerMgr.InsertStaticMarker(m_MapMarker, false);
			}
			else 
			{
				Print("No faction found!", LogLevel.ERROR);
			}
		}
		else 
		{
			Print("No faction manager found!", LogLevel.ERROR);
		}
	}
}
