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
	private static int civCounter = 0;
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
		
		GetGame().GetWorld().QueryEntitiesBySphere(
			"0 0 0",
			float.MAX,
			ProcessCities,
			FilterCityEntities,
			EQueryEntitiesFlags.STATIC
		);
		PrintFormat("Scanning map done, found %1 cities, %2 towns, %3 villages and %4 settlements", m_iCityCount, m_iTownCount, m_iVillageCount, m_iSettlementCount);
		
		int civTarget = Math.Ceil(m_iTargetCivilianCount / 
			(m_iCityWieght * m_iCityCount +  m_iTownWeight * m_iTownCount + m_iVillageWieght * m_iVillageCount + m_iSettlementWieght * m_iSettlementCount));
		
		Print("Target civilian count = " + civTarget);
		
		foreach(EntityID cityId: cities)
		{
			SpawnCivilians(cityId, civTarget);
		}
		
		Print("Finding buildings");
		GetGame().GetWorld().QueryEntitiesBySphere(
			"0 0 0",
			float.MAX,
			ProcessBuilding,
			FilterBuildingEntities,
			EQueryEntitiesFlags.STATIC
		);
		
		GetGame().GetCallqueue().CallLater(SpawnVehicles, 1500);
	}
	
	protected void SpawnVehicles()
	{
		SCR_AIWorld aiWorld = SCR_AIWorld.Cast(GetGame().GetAIWorld());
		RoadNetworkManager roadNetworkManager = aiWorld.GetRoadNetworkManager();
		foreach(vector pos: housePositions)
		{
			BaseRoad closestRoad;
			float distance = 0;
			
			int result = roadNetworkManager.GetClosestRoad(pos, closestRoad, distance);
			if (result > -1)
			{
				array<vector> roadPoints = new array<vector>;
				result = closestRoad.GetPoints(roadPoints);
				if (result > 0)
				{	
					int randIndex = s_AIRandomGenerator.RandInt(0, roadPoints.Count() - 2);
					vector p1 = roadPoints[randIndex];
					vector p2 = roadPoints[randIndex+1];
					
					vector dir = vector.Direction(p1, p2);
					vector dirNormalized = dir / dir.Length();
					vector perpendicular = dirNormalized * "0 1 0";
					vector perpendicualrNormalized = perpendicular / perpendicular.Length();
					
					vector sidePoint;
					if (s_AIRandomGenerator.RandInt(0,1) == 0)
						sidePoint = p1 + (closestRoad.GetWidth() / 2) * perpendicualrNormalized;
					else
						sidePoint = p1 - (closestRoad.GetWidth() / 2) * perpendicualrNormalized;
					
					float angleRad = Math.Atan2(dir[2], dir[0]);
					float angleDeg = angleRad*180/Math.PI;
					
					PrintFormat("Spawning vehicle at %1", sidePoint);
					SK_Global.SpawnEntityPrefab(
						SK_Global.GetConfig().m_pVehiclePrefabArray.GetRandomElement(),
						sidePoint, 
						true,
						{angleDeg, 0, 0}
					);
				}
			}
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
		else if (mapdesc.GetBaseType() == EMapDescriptorType.MDT_NAME_SETTLEMENT)
		{
			range = m_iSettlementRange;
			weight = m_iSettlementWieght;
		}
		
		int civCount = civTargetCount * weight + s_AIRandomGenerator.RandInt(-weight, weight);
		
		PrintFormat("Will spawn %1 civilians in %2", civCount, cityMarker.GetName());
		for (int i = 0; i < civCount; i++) 
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
		vector carPosition = SK_Global.GetRandomNonOceanPositionNear(spawnPosition, 50);

		BaseWorld world = GetGame().GetWorld();

		spawnPosition = SK_Global.FindSafeSpawnPosition(spawnPosition);
		IEntity civ = SK_Global.SpawnEntityPrefab(SK_Global.GetConfig().m_pCivilianPrefab, spawnPosition);
		
		EntityID civId = civ.GetID();

		m_aCivilians.Insert(civId);

		SCR_AIGroup aigroup = SCR_AIGroup.Cast(civ);
		
		array<AIWaypoint> queueOfWaypoints = new array<AIWaypoint>();
		
		if (s_AIRandomGenerator.RandFloat01() < 0.5) 
		{
			housePositions.Insert(carPosition);
			queueOfWaypoints.Insert(SpawnGetInWaypoint(carPosition));
			for (int i = 0; i < s_AIRandomGenerator.RandInt(1, 10); i++)
			{
				EntityID randCity = cities.GetRandomElement();
				IEntity cityMarker = GetGame().GetWorld().FindEntityByID(randCity);
				targetPos = SK_Global.GetRandomNonOceanPositionNear(cityMarker.GetOrigin(), 500);
				queueOfWaypoints.Insert(SpawnPatrolWaypoint(targetPos));
				queueOfWaypoints.Insert(SpawnWaitWaypoint(targetPos, s_AIRandomGenerator.RandFloatXY(15, 50)));
				PrintFormat("Civilian is going to %1", cityMarker.GetName()); 
			}
		}
		else 
		{
			queueOfWaypoints.Insert(SpawnPatrolWaypoint(targetPos));
			queueOfWaypoints.Insert(SpawnWaitWaypoint(targetPos, s_AIRandomGenerator.RandFloatXY(15, 50)));
		}

		queueOfWaypoints.Insert(SpawnPatrolWaypoint(spawnPosition));
		queueOfWaypoints.Insert(SpawnWaitWaypoint(spawnPosition, s_AIRandomGenerator.RandFloatXY(15, 50)));

		AIWaypointCycle cycle = AIWaypointCycle.Cast(SpawnWaypoint(SK_Global.GetConfig().m_pCycleWaypointPrefab, targetPos));
		cycle.SetWaypoints(queueOfWaypoints);
		aigroup.AddWaypoint(cycle);
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
		if(entity.Type() == SCR_DestructibleBuildingEntity){
			VObject mesh = entity.GetVObject();
			
			if(mesh){
				string res = mesh.GetResourceName();
				if(res.IndexOf("/Naval/") > -1) return false;
				if(res.IndexOf("/Cemeteries/") > -1) return false;
				if(res.IndexOf("/Ruins/") > -1) return false;
				return true;
					
			}
		}
		return false;
	}
	
}