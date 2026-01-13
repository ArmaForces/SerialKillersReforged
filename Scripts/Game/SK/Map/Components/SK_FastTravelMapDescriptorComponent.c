class SK_FastTravelMapDescriptorComponentClass : ScriptComponentClass
{
}

class SK_FastTravelMapDescriptorComponent: ScriptComponent
{
	protected IEntity m_Owner;
	protected SCR_EMapMarkerType m_eMarkerType = SCR_EMapMarkerType.SK_UNIT;
	
	
	protected override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		m_Owner = owner;
		
		GetGame().GetCallqueue().CallLater(CreateMapMarker, s_AIRandomGenerator.RandInt(10, 30) * 500);
	}
	
	protected void CreateMapMarker()
	{
		SCR_MapMarkerManagerComponent mapMarkerManager = SCR_MapMarkerManagerComponent.GetInstance();
		if (!mapMarkerManager)
			return;
		
		SCR_MapMarkerEntity marker = mapMarkerManager.InsertDynamicMarker(
			SCR_EMapMarkerType.SK_FAST_TRAVEL,
			m_Owner
		);
		
		if (!marker)
		{
			Print("Marker was not created!", LogLevel.ERROR);
			return;
		}		
		
		SCR_SpawnPoint sp = SCR_SpawnPoint.Cast(m_Owner);
		if (!sp)
			return;
		
		FactionKey fk = sp.GetFactionKey();
		Faction faction = GetGame().GetFactionManager().GetFactionByKey(fk);
		if (!faction)
			return;
		
		if (faction.GetFactionKey() != "CIV")
			marker.SetFaction(faction);
		
	}
}


modded enum SCR_EMapMarkerType
{
	SK_FAST_TRAVEL
}


[BaseContainerProps(), SCR_MapMarkerTitle()]
class SK_MapMarkerEntryFastTravel : SCR_MapMarkerEntryDynamic
{
	//------------------------------------------------------------------------------------------------
	override SCR_EMapMarkerType GetMarkerType()
	{
	 	return SCR_EMapMarkerType.SK_FAST_TRAVEL;
	}
}