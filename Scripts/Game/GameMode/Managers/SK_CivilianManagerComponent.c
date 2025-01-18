class SK_CivilianManagerComponentClass: ScriptComponentClass
{
}


class SK_CivilianManagerComponent: ScriptComponent
{
	[Attribute( defvalue: "1200", desc: "Range to search cities for houses")]
	int m_iCityRange;
	
	[Attribute( defvalue: "600", desc: "Range to search towns for houses")]
	int m_iTownRange;
	
	[Attribute( defvalue: "250", desc: "Range to search villages for houses")]
	int m_iVillageRange;
	
	[Attribute( defvalue: "2", desc: "Default occupants per house")]
	int m_iDefaultHouseOccupants;
	
	private static SK_CivilianManagerComponent s_Instance = null;
	private static int civCounter = 0;
	private ref array<ref EntityID> cities = new array<ref EntityID>;
	protected ref array<ref EntityID> m_aCivilians = new array<ref EntityID>;
	
	protected SCR_MapMarkerManagerComponent m_mapMarkerManager;
	
	static SK_CivilianManagerComponent GetInstance() 
	{
		if (!s_Instance)
		{
			BaseGameMode pGameMode = GetGame().GetGameMode();
			if (pGameMode)
				s_Instance = SK_CivilianManagerComponent.Cast(
					pGameMode.FindComponent(SK_CivilianManagerComponent)
				);
		}
		
		return s_Instance;
	}
	
	void Init(IEntity owner)
	{
		Print("Setting up civilians", LogLevel.DEBUG);

		m_mapMarkerManager = SCR_MapMarkerManagerComponent.GetInstance();
		
		GetGame().GetWorld().QueryEntitiesBySphere(
			"0 0 0",
			float.MAX,
			ProcessCities,
			FilterCityEntities,
			EQueryEntitiesFlags.STATIC
		);
	}
	
	AIWaypoint SpawnPatrolWaypoint(vector pos)
	{
		AIWaypoint wp = SpawnWaypoint(SK_Global.GetConfig().m_pPatrolWaypointPrefab, pos);
		return wp;
	}
	
	SCR_TimedWaypoint SpawnWaitWaypoint(vector pos, float time)
	{
		SCR_TimedWaypoint wp = SCR_TimedWaypoint.Cast(SK_Global.SpawnEntityPrefab(SK_Global.GetConfig().m_pWaitWaypointPrefab, pos));
		wp.SetHoldingTime(time);
		return wp;
	}
	
	AIWaypoint SpawnWaypoint(ResourceName res, vector pos)
	{
		AIWaypoint wp = AIWaypoint.Cast(SK_Global.SpawnEntityPrefab(res, pos));
		return wp;
	}
	
	AIWaypoint SpawnGetInWaypoint(vector pos) 
	{
		AIWaypoint wp = AIWaypoint.Cast(SK_Global.SpawnEntityPrefab(SK_Global.GetConfig().m_pGetInWaypointPrefab, pos));
		return wp;
	}
	
	protected void SpawnCivilian(vector pos)
	{
		Print("Spawning civilan at " + pos + "civ#: " + civCounter++, LogLevel.NORMAL);
		vector spawnPosition = SK_Global.GetRandomNonOceanPositionNear(pos, 50);

		vector targetPos = SK_Global.GetRandomNonOceanPositionNear(pos, 5000);
		vector carPosition = SK_Global.GetRandomNonOceanPositionNear(pos, 50);

		BaseWorld world = GetGame().GetWorld();

		spawnPosition = SK_Global.FindSafeSpawnPosition(spawnPosition);
		IEntity civ = SK_Global.SpawnEntityPrefab(SK_Global.GetConfig().m_pCivilianPrefab, spawnPosition);

		EntityID civId = civ.GetID();

		m_aCivilians.Insert(civId);

		SCR_AIGroup aigroup = SCR_AIGroup.Cast(civ);
		
		array<AIWaypoint> queueOfWaypoints = new array<AIWaypoint>();
		
		if (s_AIRandomGenerator.RandFloat01() < 0.5) 
		{
			SpawnVehicle(carPosition);
			if (s_AIRandomGenerator.RandFloat01() < 0.5) 
				queueOfWaypoints.Insert(SpawnGetInWaypoint(carPosition));
		}
		
		queueOfWaypoints.Insert(SpawnPatrolWaypoint(targetPos));
		queueOfWaypoints.Insert(SpawnWaitWaypoint(targetPos, s_AIRandomGenerator.RandFloatXY(15, 50)));

		queueOfWaypoints.Insert(SpawnPatrolWaypoint(spawnPosition));
		queueOfWaypoints.Insert(SpawnWaitWaypoint(spawnPosition, s_AIRandomGenerator.RandFloatXY(15, 50)));

		AIWaypointCycle cycle = AIWaypointCycle.Cast(SpawnWaypoint(SK_Global.GetConfig().m_pCycleWaypointPrefab, targetPos));
		cycle.SetWaypoints(queueOfWaypoints);
		//cycle.SetRerunCounter(1);
		aigroup.AddWaypoint(cycle);

		Print("Done spawning civilian", LogLevel.DEBUG);
	}
	
	protected void SpawnVehicle(vector pos)
	{
		Print("Spawning vehicle at " + pos, LogLevel.NORMAL);
		vector spawnPosition = SK_Global.GetRandomNonOceanPositionNear(pos, 50);
		BaseWorld world = GetGame().GetWorld();
		
		spawnPosition = SK_Global.FindSafeSpawnPosition(spawnPosition);
		IEntity vehicle = SK_Global.SpawnEntityPrefab(
			SK_Global.GetConfig().m_pVehiclePrefabArray.GetRandomElement(),
			spawnPosition, 
			true
		);
		
	}
	
	protected bool CheckAndProcessBuilding(IEntity entity)
	{	
		createDebugMarker(entity, 2);
		for (int i = 0; i < m_iDefaultHouseOccupants; i++)
			SpawnCivilian(entity.GetOrigin());
		return true;
	}
	
	protected bool ProcessCities(IEntity cityEntity)
	{
		vector cityPos[4];
		cityEntity.GetWorldTransform(cityPos);
		cities.Insert(cityEntity.GetID());
		
		createDebugMarker(cityEntity, 6);
		GetGame().GetWorld().QueryEntitiesBySphere(
			cityEntity.GetOrigin(),
			m_iCityRange,
			CheckAndProcessBuilding,
			FilterBuildingEntities,
			EQueryEntitiesFlags.STATIC
		);
		return true;
	}
	
	private void createDebugMarker(IEntity entity, int color)
	{	
		vector pos[4];
		entity.GetWorldTransform(pos);
		SCR_MapMarkerBase marker = new SCR_MapMarkerBase();
		marker.SetType(SCR_EMapMarkerType.PLACED_CUSTOM);
		//marker.SetCustomText(entity.ToString());
		marker.SetColorEntry(color);
		marker.SetWorldPos(pos[3][0], pos[3][2]);
		m_mapMarkerManager.InsertStaticMarker(marker, false, true);
	}
	
	/*
		Overview of civilian lifespan algo:
			1. Spawn in city/town
				* Query map globally for city enities to find city positions
				* Query map around city entites for building entites
				* Spawn 2-3 civilians per building around town
			2. Find PoI waypoint 
				* Check distance and only use vehicle if PoI is far away? 
			3. After reaching PoI find random town
			4. After reaching town, go back to point 2.
	
	{9506C81D8DA79BAB}Assets/Structures/Houses/Village/House_Village_E_1I05/House_Village_E_1I05s.xob
	*/
	
	protected bool FilterBuildingEntities(IEntity entity) 
    {
		if (entity.ClassName() == "SCR_DestructibleBuildingEntity")
		{
			VObject mesh = entity.GetVObject();
			if(mesh){
				
				string res = mesh.GetResourceName();
				if(res.IndexOf("/Military/") > -1) return false;
				if(res.IndexOf("/Industrial/") > -1) return false;
				if(res.IndexOf("/Recreation/") > -1) return false;
				
				if(res.IndexOf("/Houses/") > -1 || res.IndexOf("/Commercial/") > -1){
					if(res.IndexOf("_ruin") > -1) return false;
					if(res.IndexOf("/Shed/") > -1) return false;
					if(res.IndexOf("/Garage/") > -1) return false;
					if(res.IndexOf("/HouseAddon/") > -1) return false;
					return true;
				}
					
			}
		}
		
		return false;
    }
	
	protected bool FilterCityEntities(IEntity entity)
	{
		
		
		MapDescriptorComponent mapdesc = MapDescriptorComponent.Cast(entity.FindComponent(MapDescriptorComponent));
        if (mapdesc)
			return mapdesc.GetBaseType() == EMapDescriptorType.MDT_NAME_CITY ||
				   mapdesc.GetBaseType() == EMapDescriptorType.MDT_NAME_TOWN;

        return false;
	}
	
	protected bool FilterMapPoIEntities(IEntity entity)
	{
		MapDescriptorComponent mapdesc = MapDescriptorComponent.Cast(entity.FindComponent(MapDescriptorComponent));
        if (mapdesc) {
			int type = mapdesc.GetBaseType();
			
			if (type == EMapDescriptorType.MDT_NAME_LOCAL) return true;
			if (type == EMapDescriptorType.MDT_HILL) return true;
			if (type == EMapDescriptorType.MDT_NAME_RIDGE) return true;
		}

        return false;
	}
	
}