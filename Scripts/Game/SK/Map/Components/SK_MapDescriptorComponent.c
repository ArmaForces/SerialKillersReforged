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
		PrintFormat("MapMarker for %1 at %2", m_Owner, m_Owner.GetOrigin());
		SCR_MapMarkerManagerComponent mapMarkerManager = SCR_MapMarkerManagerComponent.GetInstance();
		
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
		{
			Print("Missing SCR_ChimeraCharacter", LogLevel.WARNING);
			return;
		}
		
		Faction faction = character.GetFaction();
		if (!faction)
		{
			Print("Mising faction", LogLevel.WARNING);
			return;
		}
		
		if (faction.GetFactionKey() != "CIV")
			marker.SetFaction(faction);
		
	}
	
	/*
	protected void UpdateMapMarker()
	{
		if (!m_Owner)
			return;
		
		vector newPos = m_Owner.GetOrigin();
		if (vector.Distance(m_lastPos, newPos) < 1)
			return;
		m_lastPos = newPos;
		
	}
	*/
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