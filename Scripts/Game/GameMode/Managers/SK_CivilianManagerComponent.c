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
	
	[Attribute(defvalue: "3", desc: "City weight in civilian distribution")]
	int m_iCityWieght;
	
	[Attribute(defvalue: "2", desc: "Town weight in civilian distribution")]
	int m_iTownWeight;
	
	[Attribute(defvalue: "1", desc: "Village weight in civilian distribution")]
	int m_iVillageWieght;
	
	[Attribute( defvalue: "200", desc: "Target number of civilians to spawn")]
	int m_iTargetCivilianCount;
	
	
	private static SK_CivilianManagerComponent s_Instance = null;
	private static int civCounter = 0;
	private ref array<ref EntityID> cities = new array<ref EntityID>;
	protected ref array<ref EntityID> m_aCivilians = new array<ref EntityID>;
	
	private int m_iCityCount = 0;
	private int m_iTownCount = 0;
	private int m_iVillageCount = 0;
	
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
		
		Print("Scanning map done, found " + m_iCityCount + " cities, " + m_iTownCount + " towns and " + m_iVillageCount + " villages", LogLevel.NORMAL);
		
		int civTarget = Math.Ceil(m_iTargetCivilianCount / 
			(m_iCityWieght * m_iCityCount +  m_iTownWeight * m_iTownCount + m_iVillageWieght * m_iVillageCount));
		
		Print("Target civilian count = " + civTarget);
		
		foreach(EntityID cityId: cities)
		{
			SpawnCivilians(cityId, civTarget);
		}
	}
	
	protected void SpawnCivilians(EntityID cityMarkerId, int civTargetCount) 
	{
		IEntity cityMarker = GetGame().GetWorld().FindEntityByID(cityMarkerId);
		MapDescriptorComponent mapdesc = MapDescriptorComponent.Cast(cityMarker.FindComponent(MapDescriptorComponent));
		int range = m_iVillageRange, weight = m_iVillageWieght;
		
		if (mapdesc.GetBaseType() == EMapDescriptorType.MDT_NAME_TOWN) 
		{
			range = m_iTownRange;
			weight = m_iTownWeight;
		}
		
		else if (mapdesc.GetBaseType() == EMapDescriptorType.MDT_NAME_CITY)
		{
			range = m_iCityRange;
			weight = m_iCityWieght;
		}
		
		for (int i = 0; i < civTargetCount * weight; i++) 
		{
			SpawnCivilian(cityMarker.GetOrigin(), range);
		}
		
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
	
	protected void SpawnCivilian(vector pos, int range)
	{
		Print("Spawning civilan at " + pos + "civ#: " + civCounter++, LogLevel.NORMAL);
		vector spawnPosition = SK_Global.GetRandomNonOceanPositionNear(pos, range);

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
		aigroup.AddWaypoint(cycle);
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
	
	protected bool ProcessCities(IEntity cityEntity)
	{
		cities.Insert(cityEntity.GetID());
		return true;
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
	*/
	
	protected bool FilterCityEntities(IEntity entity)
	{
		MapDescriptorComponent mapdesc = MapDescriptorComponent.Cast(entity.FindComponent(MapDescriptorComponent));
        if (mapdesc)
		{
			int baseType = mapdesc.GetBaseType();
			if (baseType ==  EMapDescriptorType.MDT_NAME_VILLAGE) 
			{
				m_iVillageCount += 1;
				return true;
			}
			if (baseType ==  EMapDescriptorType.MDT_NAME_TOWN) 
			{
				m_iTownCount += 1;
				return true;
			}
			if (baseType ==  EMapDescriptorType.MDT_NAME_CITY) 
			{
				m_iCityCount += 1;
				return true;
			}
		}

        return false;
	}
	
}