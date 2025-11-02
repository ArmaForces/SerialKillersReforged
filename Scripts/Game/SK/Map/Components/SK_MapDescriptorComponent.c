class SK_MapMarkerComponentClass : ScriptComponentClass
{
}

class SK_MapMarkerComponent: ScriptComponent
{
	[Attribute("5", desc: "Refresh rate in seconds")]
	int m_iRefreshRateSeconds;
	
	protected IEntity m_Owner;
	
	
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
			SCR_EMapMarkerType.SK_UNIT,
			m_Owner
		);
		
		if (!marker)
		{
			Print("Marker was not created!", LogLevel.ERROR);
			return;
		}		
		
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(m_Owner);
		if (!character)
			return;
		
		Faction faction = character.GetFaction();
		if (!faction)
			return;
		
		if (faction.GetFactionKey() != "CIV")
			marker.SetFaction(faction);
		
	}
}

modded enum SCR_EMapMarkerType
{
	SK_UNIT
}

[BaseContainerProps(), SCR_MapMarkerTitle()]
class SK_MapMarkerEntryUnit : SCR_MapMarkerEntryDynamic
{
	//------------------------------------------------------------------------------------------------
	override SCR_EMapMarkerType GetMarkerType()
	{
	 	return SCR_EMapMarkerType.SK_UNIT;
	}
}