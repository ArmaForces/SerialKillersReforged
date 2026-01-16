/**
 * SK_GameStateSystem
 * 
 * GameSystem responsible for managing all time-based game logic in Serial Killers.
 * 
 * Manages:
 * - Game start countdown
 * - Victory timeout timer
 * - Redfor inactivity penalty clock
 */
class SK_GameStateSystem : GameSystem
{
	// Update frequency
	protected const float UPDATE_INTERVAL = 1.0;
	protected float m_fUpdateTimer = 0;
	
	// Game mode reference
	protected SK_SerialKillersGameMode m_GameMode;
	
	// Timer states
	protected bool m_bIsSystemActive = false;
	protected bool m_bGameStartPending = false;
	protected bool m_bVictoryTimerActive = false;
	protected bool m_bPenaltyClockActive = false;
	
	// Countdown timers (in seconds)
	protected float m_fGameStartCountdown = 0;
	protected float m_fVictoryCountdown = 0;
	protected float m_fPenaltyCountdown = 0;
	
	// Configuration (set from game mode)
	protected float m_fPenaltyInterval = 900; // 15 minutes default
	protected int m_iMaxPenaltyStrikes = 2;
	protected int m_iPenaltyStrikeCount = 0;
	
	// Event invokers
	protected ref ScriptInvoker m_OnGameStart;
	protected ref ScriptInvoker m_OnVictoryTimeout;
	protected ref ScriptInvoker m_OnPenaltyStrike;
	protected ref ScriptInvoker m_OnMaxPenaltiesReached;
	
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
	static SK_GameStateSystem GetInstance()
	{
		World world = GetGame().GetWorld();
		if (!world)
			return null;
		
		return SK_GameStateSystem.Cast(world.FindSystem(SK_GameStateSystem));
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
		
		m_fUpdateTimer += args.GetTimeSliceSeconds();
		if (m_fUpdateTimer < UPDATE_INTERVAL)
			return;
		
		float deltaTime = m_fUpdateTimer;
		m_fUpdateTimer = 0;
		
		ProcessTimers(deltaTime);
	}
	
	//------------------------------------------------------------------------------------------------
	protected void ProcessTimers(float deltaTime)
	{
		// Game start countdown
		if (m_bGameStartPending)
		{
			m_fGameStartCountdown -= deltaTime;
			if (m_fGameStartCountdown <= 0)
			{
				m_bGameStartPending = false;
				OnGameStartTriggered();
			}
		}
		
		// Victory timeout countdown
		if (m_bVictoryTimerActive)
		{
			m_fVictoryCountdown -= deltaTime;
			if (m_fVictoryCountdown <= 0)
			{
				m_bVictoryTimerActive = false;
				OnVictoryTimeoutTriggered();
			}
		}
		
		// Penalty clock
		if (m_bPenaltyClockActive)
		{
			m_fPenaltyCountdown -= deltaTime;
			if (m_fPenaltyCountdown <= 0)
			{
				OnPenaltyTriggered();
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Public API - Start/Stop System
	//------------------------------------------------------------------------------------------------
	
	/**
	 * Initialize and start the game state system
	 * @param gameStartDelay - Seconds before game starts
	 * @param victoryTimeout - Seconds until Blufor wins by timeout
	 * @param penaltyInterval - Seconds of inactivity before penalty
	 * @param maxPenalties - Maximum penalty strikes before Blufor wins
	 */
	void StartSystem(float gameStartDelay, float victoryTimeout, float penaltyInterval, int maxPenalties)
	{
		m_GameMode = SK_SerialKillersGameMode.Cast(GetGame().GetGameMode());
		if (!m_GameMode)
		{
			Print("SK_GameStateSystem: Failed to get game mode!", LogLevel.ERROR);
			return;
		}
		
		m_fPenaltyInterval = penaltyInterval;
		m_iMaxPenaltyStrikes = maxPenalties;
		m_iPenaltyStrikeCount = 0;
		
		// Setup game start countdown
		m_fGameStartCountdown = gameStartDelay;
		m_bGameStartPending = true;
		
		// Setup victory timeout (starts after game start delay)
		m_fVictoryCountdown = gameStartDelay + victoryTimeout;
		m_bVictoryTimerActive = true;
		
		// Penalty clock starts later, after first activity period
		m_bPenaltyClockActive = false;
		
		m_bIsSystemActive = true;
		Enable(true);
		
		PrintFormat("SK_GameStateSystem: Started - Game in %1s, Victory timeout in %2s", 
			gameStartDelay, gameStartDelay + victoryTimeout);
	}
	
	//------------------------------------------------------------------------------------------------
	void StopSystem()
	{
		m_bIsSystemActive = false;
		m_bGameStartPending = false;
		m_bVictoryTimerActive = false;
		m_bPenaltyClockActive = false;
		Enable(false);
		
		Print("SK_GameStateSystem: Stopped");
	}
	
	//------------------------------------------------------------------------------------------------
	// Public API - Penalty Clock Control
	//------------------------------------------------------------------------------------------------
	
	/**
	 * Start the penalty clock (called after first activity period)
	 */
	void StartPenaltyClock()
	{
		if (!m_bIsSystemActive)
			return;
		
		m_fPenaltyCountdown = m_fPenaltyInterval;
		m_bPenaltyClockActive = true;
		
		PrintFormat("SK_GameStateSystem: Penalty clock started - %1s until first check", m_fPenaltyInterval);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Reset the penalty clock (called when a kill occurs)
	 */
	void ResetPenaltyClock()
	{
		if (!m_bPenaltyClockActive)
			return;
		
		m_fPenaltyCountdown = m_fPenaltyInterval;
		Print("SK_GameStateSystem: Penalty clock reset");
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Stop the penalty clock
	 */
	void StopPenaltyClock()
	{
		m_bPenaltyClockActive = false;
		Print("SK_GameStateSystem: Penalty clock stopped");
	}
	
	//------------------------------------------------------------------------------------------------
	// Event handlers (internal)
	//------------------------------------------------------------------------------------------------
	
	protected void OnGameStartTriggered()
	{
		Print("SK_GameStateSystem: Game starting!");
		
		if (m_OnGameStart)
			m_OnGameStart.Invoke();
		
		// Schedule penalty clock to start after initial grace period
		m_fPenaltyCountdown = m_fPenaltyInterval;
		// Don't activate yet - wait for StartPenaltyClock() to be called
	}
	
	//------------------------------------------------------------------------------------------------
	protected void OnVictoryTimeoutTriggered()
	{
		Print("SK_GameStateSystem: Victory timeout reached - Blufor wins!");
		
		if (m_OnVictoryTimeout)
			m_OnVictoryTimeout.Invoke();
	}
	
	//------------------------------------------------------------------------------------------------
	protected void OnPenaltyTriggered()
	{
		m_iPenaltyStrikeCount++;
		
		PrintFormat("SK_GameStateSystem: Penalty strike %1/%2", m_iPenaltyStrikeCount, m_iMaxPenaltyStrikes);
		
		if (m_OnPenaltyStrike)
			m_OnPenaltyStrike.Invoke(m_iPenaltyStrikeCount);
		
		// Check if max penalties reached
		if (m_iPenaltyStrikeCount >= m_iMaxPenaltyStrikes)
		{
			Print("SK_GameStateSystem: Maximum penalties reached - Blufor wins!");
			m_bPenaltyClockActive = false;
			
			if (m_OnMaxPenaltiesReached)
				m_OnMaxPenaltiesReached.Invoke();
		}
		else
		{
			// Reset for next penalty period
			m_fPenaltyCountdown = m_fPenaltyInterval;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Public API - Getters
	//------------------------------------------------------------------------------------------------
	
	bool IsSystemActive()
	{
		return m_bIsSystemActive;
	}
	
	//------------------------------------------------------------------------------------------------
	bool IsGameStartPending()
	{
		return m_bGameStartPending;
	}
	
	//------------------------------------------------------------------------------------------------
	bool IsPenaltyClockActive()
	{
		return m_bPenaltyClockActive;
	}
	
	//------------------------------------------------------------------------------------------------
	float GetGameStartCountdown()
	{
		return Math.Max(0, m_fGameStartCountdown);
	}
	
	//------------------------------------------------------------------------------------------------
	float GetVictoryCountdown()
	{
		return Math.Max(0, m_fVictoryCountdown);
	}
	
	//------------------------------------------------------------------------------------------------
	float GetPenaltyCountdown()
	{
		return Math.Max(0, m_fPenaltyCountdown);
	}
	
	//------------------------------------------------------------------------------------------------
	int GetPenaltyStrikeCount()
	{
		return m_iPenaltyStrikeCount;
	}
	
	//------------------------------------------------------------------------------------------------
	int GetMaxPenaltyStrikes()
	{
		return m_iMaxPenaltyStrikes;
	}
	
	//------------------------------------------------------------------------------------------------
	// Event Invoker Getters
	//------------------------------------------------------------------------------------------------
	
	ScriptInvoker GetOnGameStart()
	{
		if (!m_OnGameStart)
			m_OnGameStart = new ScriptInvoker();
		return m_OnGameStart;
	}
	
	//------------------------------------------------------------------------------------------------
	ScriptInvoker GetOnVictoryTimeout()
	{
		if (!m_OnVictoryTimeout)
			m_OnVictoryTimeout = new ScriptInvoker();
		return m_OnVictoryTimeout;
	}
	
	//------------------------------------------------------------------------------------------------
	ScriptInvoker GetOnPenaltyStrike()
	{
		if (!m_OnPenaltyStrike)
			m_OnPenaltyStrike = new ScriptInvoker();
		return m_OnPenaltyStrike;
	}
	
	//------------------------------------------------------------------------------------------------
	ScriptInvoker GetOnMaxPenaltiesReached()
	{
		if (!m_OnMaxPenaltiesReached)
			m_OnMaxPenaltiesReached = new ScriptInvoker();
		return m_OnMaxPenaltiesReached;
	}
}
