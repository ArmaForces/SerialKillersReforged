/**
 * SK_CivilianSpawnSystem
 * 
 * GameSystem responsible for frame-distributed spawning of civilians and vehicles.
 * Replaces CallLater patterns with proper frame-based batch spawning to avoid
 * frame rate spikes during initialization.
 * 
 * Usage:
 * 1. Queue spawn requests using QueueCivilianSpawn() and QueueVehicleSpawn()
 * 2. System processes spawns across multiple frames automatically
 * 3. Subscribe to events for spawn completion notifications
 */
class SK_CivilianSpawnSystem : GameSystem
{
	// Spawn rate limiting
	protected const int CIVILIANS_PER_FRAME = 2;
	protected const int VEHICLES_PER_FRAME = 1;
	protected const float UPDATE_INTERVAL = 0.1; // 100ms between batches
	
	protected float m_fUpdateTimer = 0;
	protected bool m_bIsSystemActive = false;
	
	// Pending spawn queues
	protected ref array<ref SK_CivilianSpawnData> m_aPendingCivilians = new array<ref SK_CivilianSpawnData>();
	protected ref array<ref SK_VehicleSpawnData> m_aPendingVehicles = new array<ref SK_VehicleSpawnData>();
	
	// Spawned entity tracking
	protected ref array<ref EntityID> m_aSpawnedCivilians = new array<ref EntityID>();
	protected ref array<ref EntityID> m_aSpawnedVehicles = new array<ref EntityID>();
	
	// Statistics
	protected int m_iTotalCiviliansQueued = 0;
	protected int m_iTotalVehiclesQueued = 0;
	protected int m_iCiviliansSpawned = 0;
	protected int m_iVehiclesSpawned = 0;
	
	// Event invokers
	protected ref ScriptInvoker m_OnCivilianSpawned;
	protected ref ScriptInvoker m_OnVehicleSpawned;
	protected ref ScriptInvoker m_OnAllCiviliansSpawned;
	protected ref ScriptInvoker m_OnAllVehiclesSpawned;
	protected ref ScriptInvoker m_OnAllSpawnsComplete;
	
	// Manager reference for spawning logic
	protected SK_CivilianManagerComponent m_CivilianManager;
	
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
	static SK_CivilianSpawnSystem GetInstance()
	{
		World world = GetGame().GetWorld();
		if (!world)
			return null;
		
		return SK_CivilianSpawnSystem.Cast(world.FindSystem(SK_CivilianSpawnSystem));
	}
	
	//------------------------------------------------------------------------------------------------
	override event bool ShouldBePaused()
	{
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	// Main update loop
	//------------------------------------------------------------------------------------------------
	override event protected void OnUpdatePoint(WorldUpdatePointArgs args)
	{
		if (!m_bIsSystemActive)
			return;
		
		// Check if there's anything to spawn
		if (m_aPendingCivilians.IsEmpty() && m_aPendingVehicles.IsEmpty())
		{
			OnAllSpawnsCompleted();
			return;
		}
		
		m_fUpdateTimer += args.GetTimeSliceSeconds();
		if (m_fUpdateTimer < UPDATE_INTERVAL)
			return;
		
		m_fUpdateTimer = 0;
		
		ProcessPendingSpawns();
	}
	
	//------------------------------------------------------------------------------------------------
	protected void ProcessPendingSpawns()
	{
		// Process civilians
		int civiliansToSpawn = Math.Min(CIVILIANS_PER_FRAME, m_aPendingCivilians.Count());
		for (int i = 0; i < civiliansToSpawn; i++)
		{
			SK_CivilianSpawnData data = m_aPendingCivilians[0];
			m_aPendingCivilians.Remove(0);
			
			EntityID spawnedId = SpawnCivilian(data);
			if (spawnedId != EntityID.INVALID)
			{
				m_aSpawnedCivilians.Insert(spawnedId);
				m_iCiviliansSpawned++;
				
				if (m_OnCivilianSpawned)
					m_OnCivilianSpawned.Invoke(spawnedId, m_iCiviliansSpawned, m_iTotalCiviliansQueued);
			}
		}
		
		// Check if all civilians are done
		if (m_aPendingCivilians.IsEmpty() && m_iCiviliansSpawned > 0)
		{
			if (m_OnAllCiviliansSpawned)
				m_OnAllCiviliansSpawned.Invoke(m_iCiviliansSpawned);
		}
		
		// Process vehicles
		int vehiclesToSpawn = Math.Min(VEHICLES_PER_FRAME, m_aPendingVehicles.Count());
		for (int i = 0; i < vehiclesToSpawn; i++)
		{
			SK_VehicleSpawnData data = m_aPendingVehicles[0];
			m_aPendingVehicles.Remove(0);
			
			EntityID spawnedId = SpawnVehicle(data);
			if (spawnedId != EntityID.INVALID)
			{
				m_aSpawnedVehicles.Insert(spawnedId);
				m_iVehiclesSpawned++;
				
				if (m_OnVehicleSpawned)
					m_OnVehicleSpawned.Invoke(spawnedId, m_iVehiclesSpawned, m_iTotalVehiclesQueued);
			}
		}
		
		// Check if all vehicles are done
		if (m_aPendingVehicles.IsEmpty() && m_iVehiclesSpawned > 0)
		{
			if (m_OnAllVehiclesSpawned)
				m_OnAllVehiclesSpawned.Invoke(m_iVehiclesSpawned);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected void OnAllSpawnsCompleted()
	{
		PrintFormat("SK_CivilianSpawnSystem: All spawns complete - %1 civilians, %2 vehicles", 
			m_iCiviliansSpawned, m_iVehiclesSpawned);
		
		m_bIsSystemActive = false;
		Enable(false);
		
		if (m_OnAllSpawnsComplete)
			m_OnAllSpawnsComplete.Invoke(m_iCiviliansSpawned, m_iVehiclesSpawned);
	}
	
	//------------------------------------------------------------------------------------------------
	// Spawn implementations
	//------------------------------------------------------------------------------------------------
	
	protected EntityID SpawnCivilian(SK_CivilianSpawnData data)
	{
		if (!m_CivilianManager)
		{
			m_CivilianManager = SK_CivilianManagerComponent.GetInstance();
			if (!m_CivilianManager)
			{
				Print("SK_CivilianSpawnSystem: CivilianManager not found!", LogLevel.ERROR);
				return EntityID.INVALID;
			}
		}
		
		vector spawnPosition = SK_Global.FindSafeSpawnPosition(data.m_vPosition);
		if (spawnPosition == vector.Zero)
		{
			Print("SK_CivilianSpawnSystem: Could not find safe spawn position", LogLevel.WARNING);
			return EntityID.INVALID;
		}
		
		IEntity civ = SK_Global.SpawnEntityPrefab(SK_Global.GetConfig().m_pCivilianPrefab, spawnPosition);
		if (!civ)
		{
			Print("SK_CivilianSpawnSystem: Failed to spawn civilian", LogLevel.ERROR);
			return EntityID.INVALID;
		}
		
		EntityID civId = civ.GetID();
		
		// Setup AI through manager - pass city ID for movement system registration
		m_CivilianManager.SetupCivilianAI(civ, data.m_vPosition, data.m_iRange, data.m_CityId);
		
		return civId;
	}
	
	//------------------------------------------------------------------------------------------------
	protected EntityID SpawnVehicle(SK_VehicleSpawnData data)
	{
		IEntity vehicle = SK_Global.SpawnEntityPrefab(
			data.m_sPrefab,
			data.m_vPosition, 
			false,
			data.m_vOrientation
		);
		
		if (!vehicle)
		{
			Print("SK_CivilianSpawnSystem: Failed to spawn vehicle", LogLevel.ERROR);
			return EntityID.INVALID;
		}
		
		return vehicle.GetID();
	}
	
	//------------------------------------------------------------------------------------------------
	// Public API - Queue Spawns
	//------------------------------------------------------------------------------------------------
	
	/**
	 * Queue a civilian for spawning
	 * @param position - Center position for spawn area
	 * @param range - Spawn range around position
	 * @param cityId - EntityID of the city this civilian belongs to
	 */
	void QueueCivilianSpawn(vector position, int range, EntityID cityId = EntityID.INVALID)
	{
		SK_CivilianSpawnData data = new SK_CivilianSpawnData();
		data.m_vPosition = position;
		data.m_iRange = range;
		data.m_CityId = cityId;
		
		m_aPendingCivilians.Insert(data);
		m_iTotalCiviliansQueued++;
		
		EnsureSystemActive();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Queue multiple civilians for spawning in a city
	 * @param cityPosition - City center position
	 * @param range - Spawn range
	 * @param count - Number of civilians to spawn
	 */
	void QueueCiviliansForCity(vector cityPosition, int range, int count)
	{
		for (int i = 0; i < count; i++)
		{
			vector spawnPos = SK_Global.GetRandomNonOceanPositionNear(cityPosition, range);
			QueueCivilianSpawn(spawnPos, range);
		}
		
		PrintFormat("SK_CivilianSpawnSystem: Queued %1 civilians near %2", count, cityPosition);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Queue a vehicle for spawning
	 * @param prefab - Vehicle prefab to spawn
	 * @param position - Spawn position
	 * @param orientation - Spawn orientation (angles)
	 */
	void QueueVehicleSpawn(ResourceName prefab, vector position, vector orientation = vector.Zero)
	{
		SK_VehicleSpawnData data = new SK_VehicleSpawnData();
		data.m_sPrefab = prefab;
		data.m_vPosition = position;
		data.m_vOrientation = orientation;
		
		m_aPendingVehicles.Insert(data);
		m_iTotalVehiclesQueued++;
		
		EnsureSystemActive();
	}
	
	//------------------------------------------------------------------------------------------------
	protected void EnsureSystemActive()
	{
		if (!m_bIsSystemActive)
		{
			m_bIsSystemActive = true;
			Enable(true);
			Print("SK_CivilianSpawnSystem: Activated");
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Public API - Control
	//------------------------------------------------------------------------------------------------
	
	void StopSystem()
	{
		m_bIsSystemActive = false;
		Enable(false);
		Print("SK_CivilianSpawnSystem: Stopped");
	}
	
	//------------------------------------------------------------------------------------------------
	void ClearQueues()
	{
		m_aPendingCivilians.Clear();
		m_aPendingVehicles.Clear();
		Print("SK_CivilianSpawnSystem: Queues cleared");
	}
	
	//------------------------------------------------------------------------------------------------
	// Public API - Getters
	//------------------------------------------------------------------------------------------------
	
	bool IsSystemActive()
	{
		return m_bIsSystemActive;
	}
	
	//------------------------------------------------------------------------------------------------
	int GetPendingCivilianCount()
	{
		return m_aPendingCivilians.Count();
	}
	
	//------------------------------------------------------------------------------------------------
	int GetPendingVehicleCount()
	{
		return m_aPendingVehicles.Count();
	}
	
	//------------------------------------------------------------------------------------------------
	int GetSpawnedCivilianCount()
	{
		return m_iCiviliansSpawned;
	}
	
	//------------------------------------------------------------------------------------------------
	int GetSpawnedVehicleCount()
	{
		return m_iVehiclesSpawned;
	}
	
	//------------------------------------------------------------------------------------------------
	float GetCivilianSpawnProgress()
	{
		if (m_iTotalCiviliansQueued == 0)
			return 1.0;
		return m_iCiviliansSpawned / m_iTotalCiviliansQueued;
	}
	
	//------------------------------------------------------------------------------------------------
	float GetVehicleSpawnProgress()
	{
		if (m_iTotalVehiclesQueued == 0)
			return 1.0;
		return m_iVehiclesSpawned / m_iTotalVehiclesQueued;
	}
	
	//------------------------------------------------------------------------------------------------
	array<ref EntityID> GetSpawnedCivilians()
	{
		return m_aSpawnedCivilians;
	}
	
	//------------------------------------------------------------------------------------------------
	array<ref EntityID> GetSpawnedVehicles()
	{
		return m_aSpawnedVehicles;
	}
	
	//------------------------------------------------------------------------------------------------
	// Event Invoker Getters
	//------------------------------------------------------------------------------------------------
	
	/**
	 * Invoked when a civilian is spawned
	 * Parameters: EntityID spawnedId, int currentCount, int totalCount
	 */
	ScriptInvoker GetOnCivilianSpawned()
	{
		if (!m_OnCivilianSpawned)
			m_OnCivilianSpawned = new ScriptInvoker();
		return m_OnCivilianSpawned;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Invoked when a vehicle is spawned
	 * Parameters: EntityID spawnedId, int currentCount, int totalCount
	 */
	ScriptInvoker GetOnVehicleSpawned()
	{
		if (!m_OnVehicleSpawned)
			m_OnVehicleSpawned = new ScriptInvoker();
		return m_OnVehicleSpawned;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Invoked when all queued civilians have been spawned
	 * Parameters: int totalSpawned
	 */
	ScriptInvoker GetOnAllCiviliansSpawned()
	{
		if (!m_OnAllCiviliansSpawned)
			m_OnAllCiviliansSpawned = new ScriptInvoker();
		return m_OnAllCiviliansSpawned;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Invoked when all queued vehicles have been spawned
	 * Parameters: int totalSpawned
	 */
	ScriptInvoker GetOnAllVehiclesSpawned()
	{
		if (!m_OnAllVehiclesSpawned)
			m_OnAllVehiclesSpawned = new ScriptInvoker();
		return m_OnAllVehiclesSpawned;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Invoked when all spawns (civilians and vehicles) are complete
	 * Parameters: int civiliansSpawned, int vehiclesSpawned
	 */
	ScriptInvoker GetOnAllSpawnsComplete()
	{
		if (!m_OnAllSpawnsComplete)
			m_OnAllSpawnsComplete = new ScriptInvoker();
		return m_OnAllSpawnsComplete;
	}
}

//------------------------------------------------------------------------------------------------
// Data classes for spawn queue
//------------------------------------------------------------------------------------------------

class SK_CivilianSpawnData
{
	vector m_vPosition;
	int m_iRange;
	EntityID m_CityId;
}

class SK_VehicleSpawnData
{
	ResourceName m_sPrefab;
	vector m_vPosition;
	vector m_vOrientation;
}
