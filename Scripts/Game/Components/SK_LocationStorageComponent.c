[EntityEditorProps("SerialKillers", description: "Component that allows container to be used by locations.", color: "0 0 255 255")]
class SK_LocationStorageComponentClass : ScriptComponentClass
{
}

sealed class SK_LocationStorageComponent : ScriptComponent
{
	[Attribute(defvalue: "0", uiwidget: UIWidgets.CheckBox, desc: "Probability of spawn will be not checked and storage will be always filled with items.", category: "Escapists")]
	protected bool m_IsSpawnGuaranteed;

	protected int m_iStorageId;
	protected bool m_IsStatic = true;
	protected SCR_InventoryStorageManagerComponent m_InventoryStorageManager;
	protected SCR_UniversalInventoryStorageComponent m_InventoryStorage;
	protected SK_StorageArsenalComponent m_ArsenalComponent;

	int GetStorageId()
	{
		return m_iStorageId;
	}

	bool IsStatic()
	{
		return m_IsStatic;
	}

	void SetIsStatic(bool isStatic)
	{
		m_IsStatic = isStatic;
	}

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (SCR_Global.IsEditMode() || !Replication.IsServer()) return;

		SetEventMask(owner, EntityEvent.INIT);
	}

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		m_ArsenalComponent = SK_StorageArsenalComponent.Cast(owner.FindComponent(SK_StorageArsenalComponent));
		if (!m_ArsenalComponent)
			Print("SK_StorageArsenalComponent component could not be found on SK_StorageArsenalComponent entity!", LogLevel.ERROR);

		m_InventoryStorageManager = SCR_InventoryStorageManagerComponent.Cast(owner.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!m_InventoryStorageManager)
			Print("SCR_InventoryStorageManagerComponent component could not be found on SK_StorageArsenalComponent entity!", LogLevel.ERROR);

		m_InventoryStorage = SCR_UniversalInventoryStorageComponent.Cast(owner.FindComponent(SCR_UniversalInventoryStorageComponent));
		if (!m_InventoryStorage)
			Print("SCR_UniversalInventoryStorageComponent component could not be found on SK_StorageArsenalComponent entity!", LogLevel.ERROR);

		//m_iStorageId = ESCT_IdentifierGenerator.GenerateIntId();
		
		FirstSetup();
	}

	void ClearStorage()
	{
		if (!m_InventoryStorageManager)
			return;

		array<IEntity> items = {};
		m_InventoryStorageManager.GetItems(items);

		foreach (IEntity entity : items)
		{
			if (!entity)
				continue;

			SCR_EntityHelper.DeleteEntityAndChildren(entity);
		}
	}

	void FirstSetup()
	{
		if (!m_ArsenalComponent)
			return;

		array<SCR_ArsenalItem> items = {};
		if (!m_ArsenalComponent.AssignItems(items, m_IsSpawnGuaranteed))
			return;

		foreach (SCR_ArsenalItem item : items)
		{
			m_InventoryStorageManager.TrySpawnPrefabToStorage(item.GetItemResourceName());
		}
		
	}

	/*
	void Spawn()
	{
		if (!m_InventoryStorageManager || !m_InventoryStorage)
			return;

		//ESCT_Logger.InfoFormat("ID: %1", param1: m_iStorageId.ToString());

		map<int, ref ESCT_StorageState> stateMap = m_StorageManager.GetStorageStates();
		ESCT_StorageState state = stateMap.Get(m_iStorageId);
		if (!state)
			return;

		//ESCT_Logger.InfoFormat("Loading StorageState: %1, %2", param1: m_iStorageId.ToString(), param2: state.GetStoredItems().Count().ToString());

		foreach (ResourceName prefab : state.GetStoredItems())
		{
			m_InventoryStorageManager.TrySpawnPrefabToStorage(prefab, m_InventoryStorage);
		}
		
		array<IEntity> spawnedItems = {};
		m_InventoryStorage.GetAll(spawnedItems);
		
		foreach (IEntity spawnedEntity : spawnedItems)
		{
			ESCT_LocationItemComponent locationItem = ESCT_LocationItemComponent.Cast(spawnedEntity.FindComponent(ESCT_LocationItemComponent));
			if (locationItem)
				locationItem.AddOnParentSlotChanged(m_StorageManager, m_iStorageId);
		
			spawnedEntity.ClearFlags(EntityFlags.ACTIVE);

			InventoryItemComponent itemComp = InventoryItemComponent.Cast(spawnedEntity.FindComponent(InventoryItemComponent));
			if (itemComp)
				itemComp.DisablePhysics();

			spawnedEntity.Update();
		}
	}
	*/
}
