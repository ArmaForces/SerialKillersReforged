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