class SK_SerialKillersGameModeClass: SCR_BaseGameModeClass
{
}

class SK_SerialKillersGameMode : SCR_BaseGameMode
{
	protected SK_SerialKillersConfigComponent m_Config;
	protected SK_CivilianManagerComponent m_CiviliansManager;
	protected SCR_MapMarkerManagerComponent m_mapMarkerManager;
	protected SCR_XPHandlerComponent m_XPHandlerComponent;
	protected SCR_FactionManager m_FactionManager;
	
	[Attribute( defvalue: "1", desc: "Civilian killed score")]
	int m_iCivKilledScore;
	
	[Attribute( defvalue: "2", desc: "Blufor killed score")]
	int m_iBluforKilledScore;
	
	[Attribute( defvalue: "50", desc: "Redfor winning score")]
	int m_iGameOverScore;
	
	[Attribute( defvalue: "40", desc: "Blufor resources score")]
	int m_iBluforResourcesScore;
	
	[Attribute( defvalue: "30", desc: "Time in seconds to delay game start")]
	int m_iGameStartDelaySeconds;
	
	[Attribute( defvalue: "30", desc: "Time in minutes before Blufor wins")]
	int m_iGameOverTimeMinutes;
	
	[Attribute( defvalue: "900", desc: "Redfor inactivity time in seconds before penalty scoring")]
	int m_iRedforInactivityTime;
	
	[Attribute( defvalue: "US", desc: "Blufor faction key")]
	string m_sBluforFactionKey;	
	
	[Attribute( defvalue: "USSR", desc: "Redfor faction key")]
	string m_sRedforFactionKey;
	
	[Attribute( defvalue: "CIV", desc: "Civilian faction key")]
	string m_sCivilianFactionKey;
	
	
	protected int SK_RedforScore = 0;
	protected int SK_BluforScore = 0;
	
	protected ref ScriptInvoker m_OnMatchSituationChanged;
	
	protected WorldTimestamp m_fVictoryTimestamp;
	protected WorldTimestamp m_fStartTimestamp;
	
	const int SK_BluforMapColor = 7; //blue
	const int SK_CivMapColor = 0; //white
	
	private bool m_bHasGameStarted = false;
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		
		if(SCR_Global.IsEditMode())
			return;
		
		Print("SerialKillers gamemode initialising!", LogLevel.NORMAL);
		
		ChimeraWorld world = GetGame().GetWorld();
		m_Config = SK_Global.GetConfig();
		m_CiviliansManager = SK_Global.GetCiviliansManager();
		m_mapMarkerManager = SCR_MapMarkerManagerComponent.GetInstance();
		m_XPHandlerComponent = SCR_XPHandlerComponent.Cast(FindComponent(SCR_XPHandlerComponent));
		m_FactionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		
		if (m_CiviliansManager && IsMaster()) 
		{
			Print("Initialising civilians", LogLevel.NORMAL);
			m_CiviliansManager.Init(this);
		}
		
		m_fStartTimestamp = world.GetServerTimestamp().PlusSeconds(m_iGameStartDelaySeconds);
		m_fVictoryTimestamp = m_fStartTimestamp.PlusSeconds(m_iGameOverTimeMinutes * 60);
		GetGame().GetCallqueue().CallLater(StartGame, m_iGameStartDelaySeconds * 1000);
		GetGame().GetCallqueue().CallLater(TimeoutGameEnd, m_iGameOverTimeMinutes * 60 * 1000 + m_iGameStartDelaySeconds * 1000);
	}
	
	ScriptInvoker GetOnMatchSituationChanged()
	{
		if (!m_OnMatchSituationChanged)
			m_OnMatchSituationChanged = new ScriptInvoker();

		return m_OnMatchSituationChanged;
	}
	
	void OnMatchSituationChanged()
	{
		if (m_OnMatchSituationChanged)
			m_OnMatchSituationChanged.Invoke();
	}

	
	void CreateKillMarker(IEntity victim, int color)
	{
		vector worldPos[4];
		victim.GetWorldTransform(worldPos);
		
		SCR_MapMarkerBase marker = new SCR_MapMarkerBase();
		marker.SetType(SCR_EMapMarkerType.PLACED_CUSTOM);
		marker.SetCustomText(getNowTimeString());
		marker.SetWorldPos(worldPos[3][0], worldPos[3][2]);
		marker.SetColorEntry(color);
		
		m_mapMarkerManager.InsertStaticMarker(marker, false, true);
	}
	
	void StartGame()
	{
		if (!IsMaster())
			return;
		
		Print("Gamemode starting");
		Rpc(RPC_DoStartGame);
		RPC_DoStartGame();
		Replication.BumpMe();
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_DoStartGame()
	{
		if (m_bHasGameStarted) return;
		m_bHasGameStarted = true;
		
		SCR_HintManagerComponent.GetInstance().ShowCustom("Game is starting", "", 10, false);
		
		array<SCR_SpawnPoint> spawnPoints = SCR_SpawnPoint.GetSpawnPoints();
		foreach(SCR_SpawnPoint sp: spawnPoints)
		{
			if (sp.GetFactionKey() == m_sRedforFactionKey)
			{
				sp.SetSpawnPointEnabled_S(false);
			}
		}
		OnMatchSituationChanged();
	}
	
	void TimeoutGameEnd()
	{
		if (!IsMaster())
			return;
		
		Print("Timeout reached, ending game with blufor win");
		SCR_Faction blufor = SCR_Faction.Cast(m_FactionManager.GetFactionByKey(m_sBluforFactionKey));
		
		array<int> bluPlayers = new array<int>;
		blufor.GetPlayersInFaction(bluPlayers);
		
		SCR_GameModeEndData endData = SCR_GameModeEndData.Create(
			EGameOverTypes.FACTION_VICTORY_SCORE,
			bluPlayers, {m_FactionManager.GetFactionIndex(blufor)}
			);
		EndGameMode(endData);
	}
	
	
	void OnUnitKilled(IEntity unit, Instigator instigator)
	{
		if (!IsMaster())
			return;
		
		if (!m_bHasGameStarted)
			return;
		
		Print("Unit was killed! unit: " + unit.GetID());
		InstigatorType instigatorType = instigator.GetInstigatorType();
		IEntity instigatorEntity = instigator.GetInstigatorEntity();
		
		if (instigatorType == InstigatorType.INSTIGATOR_NONE) 
		{
			Print("None type kill - returning", LogLevel.WARNING);
			return;
		}
		
		Faction killedFaction = FactionAffiliationComponent.Cast(
					unit.FindComponent(FactionAffiliationComponent)
				).GetAffiliatedFaction();
		
		Faction instigatorFaction = FactionAffiliationComponent.Cast(
					instigatorEntity.FindComponent(FactionAffiliationComponent)
				).GetAffiliatedFaction();
		
		switch (killedFaction.GetFactionKey()) 
		{
			case m_sRedforFactionKey:
				break;
			case m_sBluforFactionKey:
				HandleBluforKill(unit, instigatorFaction);
				break;
			case m_sCivilianFactionKey:
				HandleCivKill(unit, instigatorFaction);
				break;
			default:
				Print("Unknown victim faction - " + killedFaction.GetFactionKey(), LogLevel.WARNING);
				return;
		}
		
		Replication.BumpMe();
		GameEndCheck();
	}
	
	void HandleBluforKill(IEntity unit, Faction instigatorFaction)
	{
		SK_RedforScore += m_iBluforKilledScore;
		int blueScoreChange = m_iBluforKilledScore;
		if (instigatorFaction && instigatorFaction.GetFactionKey() == m_sBluforFactionKey)
		{
			blueScoreChange = -m_iBluforKilledScore;
		} 
		
		string message = getNowTimeString() +  ": Cop was killed - " + SK_Global.GetLocalizationString(unit);
		
		Rpc(RPC_DoOnKill, message, SK_RedforScore, blueScoreChange);
		RPC_DoOnKill(message, SK_RedforScore, blueScoreChange);
		AddBluforXP(blueScoreChange);
		CreateKillMarker(unit, SK_BluforMapColor);
	}
	
	void HandleCivKill(IEntity unit, Faction instigatorFaction)
	{
		SK_RedforScore += m_iCivKilledScore;
		int blueScoreChange = m_iCivKilledScore;
		if (instigatorFaction && instigatorFaction.GetFactionKey() == m_sBluforFactionKey) 
		{
			blueScoreChange = -m_iCivKilledScore;
		}
		
		string message = getNowTimeString() + ": Civilian was killed - " + SK_Global.GetLocalizationString(unit);
		
		Rpc(RPC_DoOnKill, message, SK_RedforScore, blueScoreChange);
		RPC_DoOnKill(message, SK_RedforScore, blueScoreChange);
		AddBluforXP(blueScoreChange);
		CreateKillMarker(unit, SK_CivMapColor);
	}
	
	string getNowTimeString() 
	{
		ChimeraWorld world = GetWorld();
		TimeContainer time = world.GetTimeAndWeatherManager().GetTime();
		return time.m_iHours.ToString(2) + ":" + time.m_iMinutes.ToString(2) + ":"  + time.m_iSeconds.ToString(2);
	}
	
	void GameEndCheck() 
	{
		if (!m_bHasGameStarted)
			return;
		
		SCR_Faction redfor = SCR_Faction.Cast(m_FactionManager.GetFactionByKey(m_sRedforFactionKey));
		SCR_Faction blufor = SCR_Faction.Cast(m_FactionManager.GetFactionByKey(m_sBluforFactionKey));
		array<int> bluPlayers = new array<int>;
		array<int> redPlayers = new array<int>;
		
		redfor.GetPlayersInFaction(redPlayers);
		blufor.GetPlayersInFaction(bluPlayers);
		
		bool redforDead = true;
		foreach(int redforPlayerId: redPlayers)
		{
			PlayerController pc = GetGame().GetPlayerManager().GetPlayerController(redforPlayerId);
			if (pc)
			{
				SCR_ChimeraCharacter ent = SCR_ChimeraCharacter.Cast(pc.GetControlledEntity());
				SCR_DamageManagerComponent damageManager = ent.GetDamageManager();
				if (!damageManager.IsDestroyed()) {
					redforDead = false;
					break;
				}
			}
		}
		
		if (redforDead) 
		{
			Print("Blufor wins!");
			SCR_GameModeEndData endData = SCR_GameModeEndData.Create(
				EGameOverTypes.FACTION_VICTORY_SCORE,
				bluPlayers, {m_FactionManager.GetFactionIndex(blufor)}
			);
			EndGameMode(endData);
		}
		else if (SK_RedforScore >= m_iGameOverScore)
		{
			Print("Redfor wins!");
			SCR_GameModeEndData endData = SCR_GameModeEndData.Create(
				EGameOverTypes.FACTION_VICTORY_SCORE,
				redPlayers, {m_FactionManager.GetFactionIndex(redfor)}
			);
			EndGameMode(endData);
		}
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_DoOnKill(string message, int redScore, int blueScoreChange)
	{
		SK_RedforScore = redScore;
		SK_BluforScore = SK_BluforScore + blueScoreChange;

		OnMatchSituationChanged();
		SCR_ChatComponent.RadioProtocolMessage(message);
	}
	
	void AddBluforXP(int blueScoreChange)
	{	
		SCR_Faction blufor = SCR_Faction.Cast(m_FactionManager.GetFactionByKey(m_sBluforFactionKey));
		array<int> bluPlayers = new array<int>;
		blufor.GetPlayersInFaction(bluPlayers);
		
		foreach (int bluePlayerId: bluPlayers)
		{
			m_XPHandlerComponent.AwardXP(bluePlayerId, SCR_EXPRewards.CUSTOM_1, 1, false, blueScoreChange);
		}
		
	}
	
	SCR_Faction GetBluforFaction()
	{
		return SCR_Faction.Cast(m_FactionManager.GetFactionByKey(m_sBluforFactionKey));
	}
	
	SCR_Faction GetRedforFaction()
	{
		return SCR_Faction.Cast(m_FactionManager.GetFactionByKey(m_sRedforFactionKey));
	}
	
	
	int GetBluforScore()
	{
		return SK_BluforScore;
	}
	
	int GetRedforScore()
	{
		return SK_RedforScore;
	}
	
	int GetGameOverScore()
	{
		return m_iGameOverScore;
	}
	
	bool IsGameRunning()
	{
		return m_bHasGameStarted;
	}
	
	WorldTimestamp GetVictoryTimestamp()
	{
		return m_fVictoryTimestamp;
	}
	
	WorldTimestamp GetGameStartTimestamp()
	{
		return m_fStartTimestamp;
	}

	
	//------------------------------------------------------------------------------------------------
	void SK_SerialKillersGameMode(IEntitySource src, IEntity parent)
	{
	}
	
	void ~SK_SerialKillersGameMode()
	{
		//TODO
	}
}