class SK_SerialKillersConfigComponentClass: ScriptComponentClass
{
}

class SK_SerialKillersConfigComponent: ScriptComponent
{
	[Attribute(uiwidget: UIWidgets.ResourceNamePicker, desc: "Civilian Prefab", params: "et", category: "Prefabs")]
	ResourceName m_pCivilianPrefab;
	
	[Attribute(uiwidget: UIWidgets.ResourceNamePicker, desc: "Patrol Waypoint Prefab", params: "et", category: "Waypoints")]
	ResourceName m_pPatrolWaypointPrefab;

	[Attribute(uiwidget: UIWidgets.ResourceNamePicker, desc: "Wait Waypoint Prefab", params: "et", category: "Waypoints")]
	ResourceName m_pWaitWaypointPrefab;
	
	[Attribute(uiwidget: UIWidgets.ResourceNamePicker, desc: "Cycle Waypoint Prefab", params: "et", category: "Waypoints")]
	ResourceName m_pCycleWaypointPrefab;
	
	[Attribute(uiwidget: UIWidgets.ResourceNamePicker, desc: "GetIn Waypoint Prefab", params: "et", category: "Waypoints")]
	ResourceName m_pGetInWaypointPrefab;
	
	[Attribute(uiwidget: UIWidgets.ResourceAssignArray, desc: "Vehicle to spawn", params: "et", category: "PrefabList")]
	ref array<ref ResourceName> m_pVehiclePrefabArray;
	
	private static SK_SerialKillersConfigComponent s_Instance = null;
	
	static SK_SerialKillersConfigComponent GetInstance()
	{
		if (!s_Instance) 
		{
			BaseGameMode pGameMode = GetGame().GetGameMode();
			if (pGameMode) 
				s_Instance = SK_SerialKillersConfigComponent.Cast(
					pGameMode.FindComponent(SK_SerialKillersConfigComponent)
				);
		}
		
		return s_Instance;
	}
}

/*
Case: 

	1) Redfor zabija cywila: 
		- Punkt dla redforu
		- informacja dla graczy (czat + mapa)
		- punkty dla bluforu(?)
	2) Redfor zabija blufor:
		- 2x punkt dla redforu
		- informacja dla graczy (czat + mapa)
		- punkty dla bluforu(?)
	3) Redfor zabija redfor:
		- nic
	4) Blufor zabija redfor:
		- nic
	5) Blufor zabija blufor:
		- 2x punkt dla redforu
		- informacja dla graczy (czat + mapa)
		- -2x punkty dla bluforu
	6) Blufor zabija cywila:
		- punkt dla redforu
		- informacja dla graczy (czat + mapa)
		- -punkt dla bluforu
	7) Cywil zabija redfor:
		- nic
	8) Cywil zabija blufor:
		- 2xpunkt dla redforu
		- informacja dla graczy (czat + mapa)
		- TBD - - punkt dla bluforu? 
	9) Cywil zabija cywila:		
		- Punkt dla redforu
		- informacja dla graczy (czat + mapa)
		- punkty dla bluforu(?)

odwrocone

	1) Ginie redfor:
		- nic
	2) Ginie blufor:
		-informacja dla graczy (czat + mapa)
		- 

*/