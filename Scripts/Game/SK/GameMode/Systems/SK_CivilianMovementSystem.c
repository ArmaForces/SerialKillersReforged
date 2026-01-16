/**
 * SK_CivilianMovementSystem
 * 
 * GameSystem responsible for managing civilian AI movement using event-driven waypoint assignment.
 * 
 * Behavior:
 * - Each civilian is randomly assigned to either:
 *   1) Walk within their current city (Patrol waypoints)
 *   2) Travel to another city (GetIn -> Move -> GetOut -> Patrol sequence)
 * 
 * - When a waypoint sequence completes, the civilian is randomly assigned a new behavior
 * - Uses SCR_AIGroup.GetOnWaypointCompleted() for event-driven waypoint management
 */

// Movement behavior types
enum SK_CivilianBehavior
{
	IDLE,           // Initial state
	LOCAL_PATROL,   // Walking around current city
	TRAVEL          // Traveling to another city via vehicle
}

// Data class to track civilian state
class SK_CivilianMovementData
{
	EntityID m_CivilianId;
	EntityID m_CurrentCityId;
	SK_CivilianBehavior m_eBehavior;
	bool m_bIsInVehicle;
	int m_iCityRange;
	ref SK_CivilianWaypointHandler m_WaypointHandler; // Reference to handler for cleanup
	
	void SK_CivilianMovementData(EntityID civId, EntityID cityId, int range)
	{
		m_CivilianId = civId;
		m_CurrentCityId = cityId;
		m_eBehavior = SK_CivilianBehavior.IDLE;
		m_bIsInVehicle = false;
		m_iCityRange = range;
	}
}

// Handler class to capture civilian ID in waypoint completion callback
class SK_CivilianWaypointHandler
{
	protected EntityID m_CivilianId;
	protected SK_CivilianMovementSystem m_MovementSystem;
	
	void SK_CivilianWaypointHandler(EntityID civId, SK_CivilianMovementSystem system)
	{
		m_CivilianId = civId;
		m_MovementSystem = system;
	}
	
	void OnWaypointCompleted(AIWaypoint waypoint)
	{
		if (m_MovementSystem)
			m_MovementSystem.HandleWaypointCompleted(m_CivilianId, waypoint);
	}
	
	EntityID GetCivilianId()
	{
		return m_CivilianId;
	}
}

class SK_CivilianMovementSystem : GameSystem
{
	// Update frequency for cleanup and monitoring
	protected const float UPDATE_INTERVAL = 5.0;
	protected float m_fUpdateTimer = 0;
	protected bool m_bIsSystemActive = false;
	
	// Civilian tracking
	protected ref map<EntityID, ref SK_CivilianMovementData> m_mCivilianData = new map<EntityID, ref SK_CivilianMovementData>();
	
	// References
	protected SK_CivilianManagerComponent m_CivilianManager;
	protected ref array<ref EntityID> m_aCities;
	protected ref array<vector> m_aCityPositions;
	
	// Configuration
	protected const float LOCAL_PATROL_CHANCE = 0.6; // 60% chance to patrol locally, 40% to travel
	protected const float MIN_WAIT_TIME = 10.0;
	protected const float MAX_WAIT_TIME = 60.0;
	
	// Random generator
	protected static ref RandomGenerator s_RandomGenerator = new RandomGenerator();
	
	//------------------------------------------------------------------------------------------------
	// System initialization
	//------------------------------------------------------------------------------------------------
	override static void InitInfo(WorldSystemInfo outInfo)
	{
		outInfo
			.SetAbstract(false)
			.SetUnique(true)
			.SetLocation(ESystemLocation.Server)
			.AddPoint(ESystemPoint.FixedFrame);
	}
	
	//------------------------------------------------------------------------------------------------
	static SK_CivilianMovementSystem GetInstance()
	{
		World world = GetGame().GetWorld();
		if (!world)
			return null;
		
		return SK_CivilianMovementSystem.Cast(world.FindSystem(SK_CivilianMovementSystem));
	}
	
	//------------------------------------------------------------------------------------------------
	override event bool ShouldBePaused()
	{
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	// Main update loop - used for cleanup and monitoring
	//------------------------------------------------------------------------------------------------
	override event protected void OnUpdatePoint(WorldUpdatePointArgs args)
	{
		if (!m_bIsSystemActive)
			return;
		
		m_fUpdateTimer += args.GetTimeSliceSeconds();
		if (m_fUpdateTimer < UPDATE_INTERVAL)
			return;
		
		m_fUpdateTimer = 0;
		CleanupDeadCivilians();
	}
	
	//------------------------------------------------------------------------------------------------
	protected void CleanupDeadCivilians()
	{
		array<EntityID> toRemove = new array<EntityID>();
		
		foreach (EntityID civId, SK_CivilianMovementData data : m_mCivilianData)
		{
			IEntity civ = GetGame().GetWorld().FindEntityByID(civId);
			if (!civ)
			{
				toRemove.Insert(civId);
			}
		}
		
		foreach (EntityID id : toRemove)
		{
			m_mCivilianData.Remove(id);
		}
		
		if (toRemove.Count() > 0)
			PrintFormat("SK_CivilianMovementSystem: Cleaned up %1 dead civilians, %2 remaining", toRemove.Count(), m_mCivilianData.Count());
	}
	
	//------------------------------------------------------------------------------------------------
	// Public API
	//------------------------------------------------------------------------------------------------
	
	/**
	 * Start the movement system
	 */
	void StartSystem()
	{
		m_CivilianManager = SK_CivilianManagerComponent.GetInstance();
		if (!m_CivilianManager)
		{
			Print("SK_CivilianMovementSystem: CivilianManager not found!", LogLevel.ERROR);
			return;
		}
		
		// Cache city data
		m_aCities = m_CivilianManager.GetCities();
		m_aCityPositions = m_CivilianManager.GetCityPositions();
		
		if (!m_aCities || m_aCities.Count() == 0)
		{
			Print("SK_CivilianMovementSystem: No cities found!", LogLevel.WARNING);
		}
		
		m_bIsSystemActive = true;
		Enable(true);
		
		Print("SK_CivilianMovementSystem: Started");
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Stop the movement system
	 */
	void StopSystem()
	{
		m_bIsSystemActive = false;
		Enable(false);
		
		// Unsubscribe from all civilian events
		foreach (EntityID civId, SK_CivilianMovementData data : m_mCivilianData)
		{
			UnsubscribeFromCivilian(civId);
		}
		
		m_mCivilianData.Clear();
		Print("SK_CivilianMovementSystem: Stopped");
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Register a civilian for movement management
	 * Called by SK_CivilianSpawnSystem after spawning
	 * @param civEntity - The spawned civilian entity (SCR_AIGroup)
	 * @param cityId - The city where civilian spawned
	 * @param range - Range for movement in the city
	 */
	void RegisterCivilian(IEntity civEntity, EntityID cityId, int range)
	{
		if (!civEntity)
			return;
		
		EntityID civId = civEntity.GetID();
		
		SCR_AIGroup aiGroup = SCR_AIGroup.Cast(civEntity);
		if (!aiGroup)
		{
			Print("SK_CivilianMovementSystem: Entity is not SCR_AIGroup!", LogLevel.ERROR);
			return;
		}
		
		// Create tracking data
		SK_CivilianMovementData data = new SK_CivilianMovementData(civId, cityId, range);
		
		// Create handler that captures civilian ID for callback
		SK_CivilianWaypointHandler handler = new SK_CivilianWaypointHandler(civId, this);
		data.m_WaypointHandler = handler;
		
		m_mCivilianData.Set(civId, data);
		
		// Subscribe to waypoint completion using the handler
		aiGroup.GetOnWaypointCompleted().Insert(handler.OnWaypointCompleted);
		
		// Assign initial behavior
		AssignNewBehavior(civId);
	}
	
	//------------------------------------------------------------------------------------------------
	protected void UnsubscribeFromCivilian(EntityID civId)
	{
		SK_CivilianMovementData data = m_mCivilianData.Get(civId);
		if (!data || !data.m_WaypointHandler)
			return;
		
		IEntity civ = GetGame().GetWorld().FindEntityByID(civId);
		if (!civ)
			return;
		
		SCR_AIGroup aiGroup = SCR_AIGroup.Cast(civ);
		if (aiGroup)
		{
			aiGroup.GetOnWaypointCompleted().Remove(data.m_WaypointHandler.OnWaypointCompleted);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Waypoint completion handler (called by SK_CivilianWaypointHandler)
	//------------------------------------------------------------------------------------------------
	void HandleWaypointCompleted(EntityID civId, AIWaypoint waypoint)
	{
		SK_CivilianMovementData data = m_mCivilianData.Get(civId);
		if (!data)
			return;
		
		// Check if this is the last waypoint in the current sequence
		IEntity civ = GetGame().GetWorld().FindEntityByID(civId);
		if (!civ)
		{
			m_mCivilianData.Remove(civId);
			return;
		}
		
		SCR_AIGroup aiGroup = SCR_AIGroup.Cast(civ);
		if (!aiGroup)
			return;
		
		// Get remaining waypoints
		array<AIWaypoint> remainingWaypoints = new array<AIWaypoint>();
		aiGroup.GetWaypoints(remainingWaypoints);
		
		// If no more waypoints, assign new behavior
		if (remainingWaypoints.IsEmpty())
		{
			AssignNewBehavior(civId);
		}
		// Otherwise let the AI continue to next waypoint
	}
	
	//------------------------------------------------------------------------------------------------
	// Behavior assignment
	//------------------------------------------------------------------------------------------------
	protected void AssignNewBehavior(EntityID civId)
	{
		SK_CivilianMovementData data = m_mCivilianData.Get(civId);
		if (!data)
			return;
		
		IEntity civ = GetGame().GetWorld().FindEntityByID(civId);
		if (!civ)
		{
			m_mCivilianData.Remove(civId);
			return;
		}
		
		SCR_AIGroup aiGroup = SCR_AIGroup.Cast(civ);
		if (!aiGroup)
			return;
		
		// Clear existing waypoints
		ClearWaypoints(aiGroup);
		
		// Randomly choose behavior
		float roll = s_RandomGenerator.RandFloat01();
		
		if (roll < LOCAL_PATROL_CHANCE || !m_aCities || m_aCities.Count() < 2)
		{
			// Local patrol in current city
			AssignLocalPatrol(aiGroup, data);
		}
		else
		{
			// Travel to another city
			AssignTravelBehavior(aiGroup, data);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected void ClearWaypoints(SCR_AIGroup aiGroup)
	{
		// Remove all current waypoints
		array<AIWaypoint> waypoints = new array<AIWaypoint>();
		aiGroup.GetWaypoints(waypoints);
		
		foreach (AIWaypoint wp : waypoints)
		{
			aiGroup.RemoveWaypoint(wp);
			delete wp;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Assign local patrol behavior - walk to random position in current city
	 */
	protected void AssignLocalPatrol(SCR_AIGroup aiGroup, SK_CivilianMovementData data)
	{
		data.m_eBehavior = SK_CivilianBehavior.LOCAL_PATROL;
		
		// Get current city position
		IEntity cityMarker = GetGame().GetWorld().FindEntityByID(data.m_CurrentCityId);
		vector cityPos;
		
		if (cityMarker)
		{
			cityPos = cityMarker.GetOrigin();
		}
		else
		{
			// Fallback to civilian's current position
			cityPos = aiGroup.GetOrigin();
		}
		
		// Get random target within city
		vector targetPos = SK_Global.GetRandomNonOceanPositionNear(cityPos, data.m_iCityRange);
		
		// Create patrol waypoint
		AIWaypoint patrolWp = SpawnPatrolWaypoint(targetPos);
		if (patrolWp)
		{
			aiGroup.AddWaypoint(patrolWp);
		}
		
		// Add wait waypoint after patrol
		float waitTime = s_RandomGenerator.RandFloatXY(MIN_WAIT_TIME, MAX_WAIT_TIME);
		SCR_TimedWaypoint waitWp = SpawnWaitWaypoint(targetPos, waitTime);
		if (waitWp)
		{
			aiGroup.AddWaypoint(waitWp);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Assign travel behavior - get in vehicle, drive to another city, get out, patrol
	 */
	protected void AssignTravelBehavior(SCR_AIGroup aiGroup, SK_CivilianMovementData data)
	{
		data.m_eBehavior = SK_CivilianBehavior.TRAVEL;
		
		// Pick a different city
		EntityID targetCityId = GetRandomDifferentCity(data.m_CurrentCityId);
		if (targetCityId == EntityID.INVALID)
		{
			// Fallback to local patrol
			AssignLocalPatrol(aiGroup, data);
			return;
		}
		
		IEntity targetCity = GetGame().GetWorld().FindEntityByID(targetCityId);
		if (!targetCity)
		{
			AssignLocalPatrol(aiGroup, data);
			return;
		}
		
		vector currentPos = aiGroup.GetOrigin();
		vector targetCityPos = targetCity.GetOrigin();
		
		// 1. Get In waypoint - find nearest vehicle
		AIWaypoint getInWp = SpawnGetInWaypoint(currentPos);
		if (getInWp)
		{
			aiGroup.AddWaypoint(getInWp);
			data.m_bIsInVehicle = true;
		}
		
		// 2. Move waypoint to target city
		AIWaypoint moveWp = SpawnPatrolWaypoint(targetCityPos);
		if (moveWp)
		{
			aiGroup.AddWaypoint(moveWp);
		}
		
		// 3. Get Out waypoint
		AIWaypoint getOutWp = SpawnGetOutWaypoint(targetCityPos);
		if (getOutWp)
		{
			aiGroup.AddWaypoint(getOutWp);
		}
		
		// 4. Patrol in new city
		int targetRange = GetCityRange(targetCityId);
		vector patrolTarget = SK_Global.GetRandomNonOceanPositionNear(targetCityPos, targetRange);
		
		AIWaypoint patrolWp = SpawnPatrolWaypoint(patrolTarget);
		if (patrolWp)
		{
			aiGroup.AddWaypoint(patrolWp);
		}
		
		// Update current city
		data.m_CurrentCityId = targetCityId;
		data.m_iCityRange = targetRange;
		data.m_bIsInVehicle = false;
	}
	
	//------------------------------------------------------------------------------------------------
	// Helper methods
	//------------------------------------------------------------------------------------------------
	
	protected EntityID GetRandomDifferentCity(EntityID excludeCityId)
	{
		if (!m_aCities || m_aCities.Count() < 2)
			return EntityID.INVALID;
		
		array<ref EntityID> availableCities = new array<ref EntityID>();
		
		foreach (EntityID cityId : m_aCities)
		{
			if (cityId != excludeCityId)
				availableCities.Insert(cityId);
		}
		
		if (availableCities.IsEmpty())
			return EntityID.INVALID;
		
		int index = s_RandomGenerator.RandInt(0, availableCities.Count() - 1);
		return availableCities[index];
	}
	
	//------------------------------------------------------------------------------------------------
	protected int GetCityRange(EntityID cityId)
	{
		if (!m_CivilianManager)
			return 500; // Default
		
		IEntity cityMarker = GetGame().GetWorld().FindEntityByID(cityId);
		if (!cityMarker)
			return 500;
		
		MapDescriptorComponent mapdesc = MapDescriptorComponent.Cast(cityMarker.FindComponent(MapDescriptorComponent));
		if (!mapdesc)
			return 500;
		
		return m_CivilianManager.GetRangeForCityType(mapdesc.GetBaseType());
	}
	
	//------------------------------------------------------------------------------------------------
	// Waypoint spawning helpers
	//------------------------------------------------------------------------------------------------
	
	protected AIWaypoint SpawnPatrolWaypoint(vector pos)
	{
		ResourceName prefab = SK_Global.GetConfig().m_pPatrolWaypointPrefab;
		if (prefab.IsEmpty())
			return null;
		
		return AIWaypoint.Cast(SK_Global.SpawnEntityPrefab(prefab, pos));
	}
	
	//------------------------------------------------------------------------------------------------
	protected SCR_TimedWaypoint SpawnWaitWaypoint(vector pos, float time)
	{
		ResourceName prefab = SK_Global.GetConfig().m_pWaitWaypointPrefab;
		if (prefab.IsEmpty())
			return null;
		
		SCR_TimedWaypoint wp = SCR_TimedWaypoint.Cast(SK_Global.SpawnEntityPrefab(prefab, pos));
		if (wp)
			wp.SetHoldingTime(time);
		
		return wp;
	}
	
	//------------------------------------------------------------------------------------------------
	protected AIWaypoint SpawnGetInWaypoint(vector pos)
	{
		ResourceName prefab = SK_Global.GetConfig().m_pGetInWaypointPrefab;
		if (prefab.IsEmpty())
			return null;
		
		return AIWaypoint.Cast(SK_Global.SpawnEntityPrefab(prefab, pos));
	}
	
	//------------------------------------------------------------------------------------------------
	protected AIWaypoint SpawnGetOutWaypoint(vector pos)
	{
		ResourceName prefab = SK_Global.GetConfig().m_pGetOutWaypointPrefab;
		if (prefab.IsEmpty())
			return null;
		
		return AIWaypoint.Cast(SK_Global.SpawnEntityPrefab(prefab, pos));
	}
	
	//------------------------------------------------------------------------------------------------
	// Getters
	//------------------------------------------------------------------------------------------------
	
	bool IsSystemActive()
	{
		return m_bIsSystemActive;
	}
	
	//------------------------------------------------------------------------------------------------
	int GetManagedCivilianCount()
	{
		return m_mCivilianData.Count();
	}
}
