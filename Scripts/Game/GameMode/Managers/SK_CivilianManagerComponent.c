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
		
		GetGame().GetWorld().QueryEntitiesBySphere(
			"0 0 0",
			float.MAX,
			CheckAndProcessBuilding,
			FilterBuildingEntities,
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

	protected ref array<ref EntityID> m_aCivilians = new array<ref EntityID>;
	
	
	protected void SpawnCivilian(vector pos)
	{
		Print("Spawning civilan at " + pos + "civ#: " + civCounter++, LogLevel.NORMAL);
		vector spawnPosition = SK_Global.GetRandomNonOceanPositionNear(pos, 50);

		vector targetPos = SK_Global.GetRandomNonOceanPositionNear(pos, 5000);

		BaseWorld world = GetGame().GetWorld();

		spawnPosition = SK_Global.FindSafeSpawnPosition(spawnPosition);
		IEntity civ = SK_Global.SpawnEntityPrefab(SK_Global.GetConfig().m_pCivilianPrefab, spawnPosition);

		EntityID civId = civ.GetID();

		m_aCivilians.Insert(civId);

		SCR_AIGroup aigroup = SCR_AIGroup.Cast(civ);
		
		//TODO: Dress civs
		//aigroup.GetOnAgentAdded().Insert(RandomizeCivilianClothes);
		
		
		array<AIWaypoint> queueOfWaypoints = new array<AIWaypoint>();

		queueOfWaypoints.Insert(SpawnPatrolWaypoint(targetPos));
		queueOfWaypoints.Insert(SpawnWaitWaypoint(targetPos, s_AIRandomGenerator.RandFloatXY(15, 50)));

		queueOfWaypoints.Insert(SpawnPatrolWaypoint(spawnPosition));
		queueOfWaypoints.Insert(SpawnWaitWaypoint(spawnPosition, s_AIRandomGenerator.RandFloatXY(15, 50)));

		AIWaypointCycle cycle = AIWaypointCycle.Cast(SpawnWaypoint(SK_Global.GetConfig().m_pCycleWaypointPrefab, targetPos));
		cycle.SetWaypoints(queueOfWaypoints);
		cycle.SetRerunCounter(-1);
		aigroup.AddWaypoint(cycle);
		Print("Done spawning civilian", LogLevel.DEBUG);
	}
	
	protected bool CheckAndProcessBuilding(IEntity entity)
	{	
		MapDescriptorComponent mapdesc = MapDescriptorComponent.Cast(entity.FindComponent(MapDescriptorComponent));
		if (mapdesc){
			ProcessBuilding(entity, mapdesc);
		}
		return true;
	}
	
	protected void ProcessBuilding(IEntity entity, MapDescriptorComponent mapdesc)
	{
		Print("Building found at " + entity.GetOrigin(), LogLevel.DEBUG);
		for (int i = 0; i < m_iDefaultHouseOccupants; i++)
			SpawnCivilian(entity.GetOrigin());
	}
	
	protected bool FilterBuildingEntities(IEntity entity) 
	{
		return true;
		if(entity.ClassName() == "SCR_DestructibleBuildingEntity"){
			VObject mesh = entity.GetVObject();
			
			if(mesh){
				//TODO: Filter map entities
				string res = mesh.GetResourceName();
				//if(res.IndexOf("/Military/") > -1) return false;
				//if(res.IndexOf("/Industrial/") > -1) return false;
				//if(res.IndexOf("/Recreation/") > -1) return false;
				
				if(res.IndexOf("/Houses/") > -1){
					//if(res.IndexOf("_ruin") > -1) return false;
					//if(res.IndexOf("/Shed/") > -1) return false;
					//if(res.IndexOf("/Garage/") > -1) return false;
					//if(res.IndexOf("/HouseAddon/") > -1) return false;
					return true;
				}
					
			}
		}
		return false;	
	}
	
	void OnAIKilled(IEntity ai, IEntity instigator)
	{
		Print("AI " + ai.GetID() + " was killed by " + instigator.GetID() + " !", LogLevel.WARNING);
	}
}