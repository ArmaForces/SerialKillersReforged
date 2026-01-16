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
	
	[Attribute( defvalue: "125", desc: "Range to search settlements for houses")]
	int m_iSettlementRange;
	
	[Attribute(defvalue: "4", desc: "City weight in civilian distribution")]
	int m_iCityWieght;
	
	[Attribute(defvalue: "3", desc: "Town weight in civilian distribution")]
	int m_iTownWeight;
	
	[Attribute(defvalue: "2", desc: "Village weight in civilian distribution")]
	int m_iVillageWieght;
	
	[Attribute(defvalue: "2", desc: "Settlement weight in civilian distribution")]
	int m_iSettlementWieght;
	
	[Attribute( defvalue: "200", desc: "Target number of civilians to spawn")]
	int m_iTargetCivilianCount;
		
	[Attribute( defvalue: "0.05", desc: "Chance to spawn vehicle at a building in city")]
	float m_fCityVehicleSpawnChance;
	
	[Attribute( defvalue: "0.2", desc: "Chance to spawn vehicle at a building outside of cities")]
	float m_fVehicleSpawnChance;
	
	
	private static SK_CivilianManagerComponent s_Instance = null;
	private ref array<ref EntityID> cities = new array<ref EntityID>;
	private ref array<vector> housePositions = new array<vector>;
	private ref array<vector> cityPositions = new array<vector>;
	protected ref array<ref EntityID> m_aCivilians = new array<ref EntityID>;
	
	private int m_iCityCount = 0;
	private int m_iTownCount = 0;
	private int m_iVillageCount = 0;
	private int m_iSettlementCount = 0;
	
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
		
		// Scan for cities on the map
		GetGame().GetWorld().QueryEntitiesBySphere(
			"0 0 0",
			float.MAX,
			ProcessCities,
			FilterCityEntities,
			EQueryEntitiesFlags.STATIC
		);
		PrintFormat("Scanning map done, found %1 cities, %2 towns, %3 villages and %4 settlements", m_iCityCount, m_iTownCount, m_iVillageCount, m_iSettlementCount);
		
		// Calculate target civilians per weight unit
		int totalWeight = m_iCityWieght * m_iCityCount + m_iTownWeight * m_iTownCount + m_iVillageWieght * m_iVillageCount + m_iSettlementWieght * m_iSettlementCount;
		if (totalWeight == 0)
		{
			Print("SK_CivilianManagerComponent: No cities found on map!", LogLevel.WARNING);
			return;
		}
		
		int civTarget = Math.Ceil(m_iTargetCivilianCount / totalWeight);
		Print("Target civilian count per weight = " + civTarget);
		
		// Start the movement system before spawning civilians
		SK_CivilianMovementSystem movementSystem = SK_CivilianMovementSystem.GetInstance();
		if (movementSystem)
		{
			movementSystem.StartSystem();
		}
		else
		{
			Print("SK_CivilianManagerComponent: SK_CivilianMovementSystem not found! Civilians will be idle.", LogLevel.WARNING);
		}
		
		// Get spawn system and queue civilians
		SK_CivilianSpawnSystem spawnSystem = SK_CivilianSpawnSystem.GetInstance();
		if (!spawnSystem)
		{
			Print("SK_CivilianManagerComponent: SK_CivilianSpawnSystem not found! Ensure it's registered in WorldSystemInfo.", LogLevel.ERROR);
			return;
		}
		
		// Subscribe to spawn completion for vehicle spawning
		spawnSystem.GetOnAllCiviliansSpawned().Insert(OnCiviliansSpawnComplete);
		
		// Queue civilians for each city
		foreach (EntityID cityId : cities)
		{
			QueueCiviliansForCity(cityId, civTarget, spawnSystem);
		}
		
		PrintFormat("SK_CivilianManagerComponent: Queued civilians via spawn system");
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Queue civilians for a city using the spawn system
	 */
	protected void QueueCiviliansForCity(EntityID cityMarkerId, int civTargetCount, SK_CivilianSpawnSystem spawnSystem)
	{
		IEntity cityMarker = GetGame().GetWorld().FindEntityByID(cityMarkerId);
		if (!cityMarker)
			return;
		
		MapDescriptorComponent mapdesc = MapDescriptorComponent.Cast(cityMarker.FindComponent(MapDescriptorComponent));
		if (!mapdesc)
			return;
		
		int range = GetRangeForCityType(mapdesc.GetBaseType());
		int weight = GetWeightForCityType(mapdesc.GetBaseType());
		
		int civCount = civTargetCount * weight + s_AIRandomGenerator.RandInt(-weight, weight);
		civCount = Math.Max(1, civCount);
		
		PrintFormat("Queueing %1 civilians for %2", civCount, cityMarker.GetName());
		
		// Queue each civilian spawn with city ID
		vector cityPos = cityMarker.GetOrigin();
		for (int i = 0; i < civCount; i++)
		{
			vector spawnPos = SK_Global.GetRandomNonOceanPositionNear(cityPos, range);
			spawnSystem.QueueCivilianSpawn(spawnPos, range, cityMarkerId);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Called when all civilians have been spawned - now spawn vehicles
	 */
	protected void OnCiviliansSpawnComplete(int totalSpawned)
	{
		PrintFormat("SK_CivilianManagerComponent: All %1 civilians spawned, now spawning vehicles", totalSpawned);
		
		SK_CivilianSpawnSystem spawnSystem = SK_CivilianSpawnSystem.GetInstance();
		if (!spawnSystem)
		{
			Print("SK_CivilianManagerComponent: SK_CivilianSpawnSystem not found for vehicle spawning!", LogLevel.ERROR);
			return;
		}
		
		FindBuildingsAndQueueVehicles(spawnSystem);
		
		// Unsubscribe from event
		spawnSystem.GetOnAllCiviliansSpawned().Remove(OnCiviliansSpawnComplete);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Find buildings and queue vehicle spawns
	 */
	protected void FindBuildingsAndQueueVehicles(SK_CivilianSpawnSystem spawnSystem)
	{
		Print("Finding buildings for vehicle spawns");
		GetGame().GetWorld().QueryEntitiesBySphere(
			"0 0 0",
			float.MAX,
			ProcessBuilding,
			FilterBuildingEntities,
			EQueryEntitiesFlags.STATIC
		);
		
		QueueVehicleSpawns(spawnSystem);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Queue vehicle spawns using the spawn system
	 */
	protected void QueueVehicleSpawns(SK_CivilianSpawnSystem spawnSystem)
	{
		SCR_AIWorld aiWorld = SCR_AIWorld.Cast(GetGame().GetAIWorld());
		if (!aiWorld)
		{
			Print("SK_CivilianManagerComponent: AIWorld not found!", LogLevel.ERROR);
			return;
		}
		
		RoadNetworkManager roadNetworkManager = aiWorld.GetRoadNetworkManager();
		if (!roadNetworkManager)
		{
			Print("SK_CivilianManagerComponent: RoadNetworkManager not found!", LogLevel.ERROR);
			return;
		}
		
		array<ref ResourceName> vehiclePrefabs = SK_Global.GetConfig().m_pVehiclePrefabArray;
		if (!vehiclePrefabs || vehiclePrefabs.Count() == 0)
		{
			Print("SK_CivilianManagerComponent: No vehicle prefabs configured!", LogLevel.WARNING);
			return;
		}
		
		int vehiclesQueued = 0;
		foreach (vector pos : housePositions)
		{
			BaseRoad closestRoad;
			float distance = 0;
			
			int result = roadNetworkManager.GetClosestRoad(pos, closestRoad, distance);
			if (result > -1 && closestRoad)
			{
				array<vector> roadPoints = new array<vector>();
				result = closestRoad.GetPoints(roadPoints);
				if (result > 0 && roadPoints.Count() >= 2)
				{
					int randIndex = s_AIRandomGenerator.RandInt(0, Math.Max(0, roadPoints.Count() - 2));
					vector p1 = roadPoints[randIndex];
					vector p2 = roadPoints[randIndex + 1];
					
					vector dir = vector.Direction(p1, p2);
					float dirLen = dir.Length();
					if (dirLen < 0.001)
						continue;
					
					vector dirNormalized = dir / dirLen;
					vector perpendicular = dirNormalized * "0 1 0";
					float perpLen = perpendicular.Length();
					if (perpLen < 0.001)
						continue;
					
					vector perpendicularNormalized = perpendicular / perpLen;
					
					vector sidePoint;
					if (s_AIRandomGenerator.RandInt(0, 1) == 0)
						sidePoint = p1 + (closestRoad.GetWidth() / 2) * perpendicularNormalized;
					else
						sidePoint = p1 - (closestRoad.GetWidth() / 2) * perpendicularNormalized;
					
					float angleRad = Math.Atan2(dir[2], dir[0]);
					float angleDeg = angleRad * 180 / Math.PI;
					
					ResourceName vehiclePrefab = vehiclePrefabs.GetRandomElement();
					spawnSystem.QueueVehicleSpawn(vehiclePrefab, sidePoint, Vector(angleDeg, 0, 0));
					vehiclesQueued++;
				}
			}
		}
		
		PrintFormat("SK_CivilianManagerComponent: Queued %1 vehicles for spawning", vehiclesQueued);
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
	
	AIWaypoint SpawnGetOutWaypoint(vector pos)
	{
		AIWaypoint wp = AIWaypoint.Cast(SK_Global.SpawnEntityPrefab(SK_Global.GetConfig().m_pGetOutWaypointPrefab, pos));
		return wp;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Setup AI for a spawned civilian entity
	 * Registers the civilian with SK_CivilianMovementSystem for dynamic waypoint management
	 * @param civEntity - The spawned civilian entity
	 * @param originPos - Origin position (city center)
	 * @param range - Range for movement in the city
	 * @param cityId - EntityID of the city this civilian belongs to
	 */
	void SetupCivilianAI(IEntity civEntity, vector originPos, int range, EntityID cityId = EntityID.INVALID)
	{
		if (!civEntity)
			return;
		
		EntityID civId = civEntity.GetID();
		m_aCivilians.Insert(civId);
		
		SCR_AIGroup aigroup = SCR_AIGroup.Cast(civEntity);
		if (!aigroup)
		{
			Print("SK_CivilianManagerComponent: Entity is not an AI group!", LogLevel.ERROR);
			return;
		}
		
		// Prevent max LOD if configured
		if (!SK_Global.GetConfig().IsAIMaxLodAllowed())
			aigroup.PreventMaxLOD();
		
		// Register with movement system for dynamic waypoint management
		SK_CivilianMovementSystem movementSystem = SK_CivilianMovementSystem.GetInstance();
		if (movementSystem)
		{
			movementSystem.RegisterCivilian(civEntity, cityId, range);
		}
		else
		{
			Print("SK_CivilianManagerComponent: MovementSystem not found, civilian will be idle!", LogLevel.WARNING);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Get array of city entity IDs (for spawn system to use)
	 */
	array<ref EntityID> GetCities()
	{
		return cities;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Get city positions array
	 */
	array<vector> GetCityPositions()
	{
		return cityPositions;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Get range for a city type
	 */
	int GetRangeForCityType(EMapDescriptorType type)
	{
		switch (type)
		{
			case EMapDescriptorType.MDT_NAME_CITY:
				return m_iCityRange;
			case EMapDescriptorType.MDT_NAME_TOWN:
				return m_iTownRange;
			case EMapDescriptorType.MDT_NAME_VILLAGE:
				return m_iVillageRange;
			case EMapDescriptorType.MDT_NAME_SETTLEMENT:
				return m_iSettlementRange;
		}
		return m_iVillageRange;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Get weight for a city type
	 */
	int GetWeightForCityType(EMapDescriptorType type)
	{
		switch (type)
		{
			case EMapDescriptorType.MDT_NAME_CITY:
				return m_iCityWieght;
			case EMapDescriptorType.MDT_NAME_TOWN:
				return m_iTownWeight;
			case EMapDescriptorType.MDT_NAME_VILLAGE:
				return m_iVillageWieght;
			case EMapDescriptorType.MDT_NAME_SETTLEMENT:
				return m_iSettlementWieght;
		}
		return m_iVillageWieght;
	}
	
	protected bool ProcessCities(IEntity cityEntity)
	{
		cities.Insert(cityEntity.GetID());
		cityPositions.Insert(cityEntity.GetOrigin());
		return true;
	}
	
	protected bool FilterCityEntities(IEntity entity)
	{
		MapDescriptorComponent mapdesc = MapDescriptorComponent.Cast(entity.FindComponent(MapDescriptorComponent));
        if (mapdesc)
		{
			int baseType = mapdesc.GetBaseType();
			if (baseType == EMapDescriptorType.MDT_NAME_SETTLEMENT)
			{
				m_iSettlementCount += 1;
				return true;
			}
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
	
	protected bool ProcessBuilding(IEntity building)
	{
		float spawnChance = m_fVehicleSpawnChance;
		vector buildingPos = building.GetOrigin();
		
		foreach(vector pos: cityPositions)
		{
			if (vector.Distance(pos, buildingPos) < m_iCityRange)
			{
				spawnChance = m_fCityVehicleSpawnChance;
				break;
			}
		}
		
		if (s_AIRandomGenerator.RandFloat01() < spawnChance)
		{
			housePositions.Insert(building.GetOrigin());
		}
		return true;
	}
	
	protected bool FilterBuildingEntities(IEntity entity)
	{
		if (entity.Type() != SCR_DestructibleBuildingEntity)
			return false;
		
		VObject mesh = entity.GetVObject();
		if (!mesh)
			Print("No mesh found for " + entity.GetName());
		
		if(mesh){
			string res = mesh.GetResourceName();
			if(res.IndexOf("/Naval/") > -1) 
				return false;
			if(res.IndexOf("/Cemeteries/") > -1) 
				return false;
			//if(res.IndexOf("/Ruins/") > -1) return false;
			return true;
				
		}
		
		return false;
	}
	
}