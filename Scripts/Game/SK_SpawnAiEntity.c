[EntityEditorProps(category: "Tutorial Entities", description: "Spawns AI Character")]
class SK_SpawnAIEntityClass : GenericEntityClass
{}

class SK_SpawnAIEntity : GenericEntity
{
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

	protected ref array<ref EntityID> m_aCivilians;
	
	protected void SpawnCivilian(vector pos)
	{
		Print("Spawning civilan at " + pos, LogLevel.NORMAL);
		vector spawnPosition = SK_Global.GetRandomNonOceanPositionNear(pos, 10);

		vector targetPos = SK_Global.GetRandomNonOceanPositionNear(pos, 500);

		BaseWorld world = GetGame().GetWorld();

		spawnPosition = SK_Global.FindSafeSpawnPosition(spawnPosition);
		IEntity civ = SK_Global.SpawnEntityPrefab(SK_Global.GetConfig().m_pCivilianPrefab, spawnPosition);

		EntityID civId = civ.GetID();

		m_aCivilians.Insert(civId);

		SCR_AIGroup aigroup = SCR_AIGroup.Cast(civ);
		
		//TODO! Dress civs
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

	
	void SK_SpawnAIEntity(IEntitySource src, IEntity parent) 
	{
		SetFlags(EntityFlags.ACTIVE | EntityFlags.NO_LINK, false);
		SetEventMask(EntityEvent.FRAME);
		m_aCivilians = new array<ref EntityID>;
	}
}