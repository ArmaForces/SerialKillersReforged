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
	
	[Attribute( defvalue: "US", desc: "Blufor faction key")]
	string m_sBluforFactionKey;	
	
	[Attribute( defvalue: "USSR", desc: "Redfor faction key")]
	string m_sRedforFactionKey;
	
	[Attribute( defvalue: "CIV", desc: "Civilian faction key")]
	string m_sCivilianFactionKey;
	
	protected int SK_RedforScore = 0;
	protected int SK_BluforScore = 0;
	
	const int SK_BluforMapColor = 7; //blue
	const int SK_CivMapColor = 0; //white
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		
		if(SCR_Global.IsEditMode())
			return;
		
		Print("SerialKillers gamemode initialising!", LogLevel.NORMAL);
		
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
	
	
	void OnUnitKilled(IEntity unit, Instigator instigator)
	{
		if (!IsMaster())
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
		GameEndCheck();
	}
	
	void HandleBluforKill(IEntity unit, Faction instigatorFaction)
	{
		SK_RedforScore += 2*m_iCivKilledScore;
		int blueScoreChange = 0;
		if (instigatorFaction) 
		{
			if (instigatorFaction.GetFactionKey() == "US")
			{
				blueScoreChange = -2 * m_iCivKilledScore;
			} 
			else 
			{
				blueScoreChange = m_iCivKilledScore;
			}
		}
		
		Rpc(RPC_DoOnKill, "Cop was killed", SK_RedforScore, blueScoreChange);
		RPC_DoOnKill("Cop was killed", SK_RedforScore, blueScoreChange);
		CreateKillMarker(unit, SK_BluforMapColor);
	}
	
	void HandleCivKill(IEntity unit, Faction instigatorFaction)
	{
		SK_RedforScore += m_iCivKilledScore;
		int blueScoreChange = 0;
		if (instigatorFaction) 
		{
			if (instigatorFaction.GetFactionKey() == "US")
			{
				blueScoreChange = -m_iCivKilledScore;
			} 
			else 
			{
				blueScoreChange = m_iCivKilledScore;
			}
		}
		Rpc(RPC_DoOnKill, "Civilian was killed", SK_RedforScore, blueScoreChange);
		RPC_DoOnKill("Civilian was killed", SK_RedforScore, blueScoreChange);
		CreateKillMarker(unit, SK_CivMapColor);
	}
	
	void ShowScoreHint(string message, int blueScoreChange)
	{
		string msg = "\t" + getNowTimeString() + "\n";
		msg = msg + "_____________\n";
		msg = msg + "\t Killers\n";
		msg = msg + "\t " + SK_RedforScore + "/" + m_iGameOverScore + "\n";
		msg = msg + "\n";
		msg = msg + "\t Police\n";
		msg = msg + "\t " + SK_BluforScore + "/" + m_iBluforResourcesScore + " (" + blueScoreChange + ")\n";
		msg = msg + message;
		
		SCR_HintManagerComponent.GetInstance().ShowCustom(msg, "", 10, false);
	}
	
	string getNowTimeString() 
	{
		ChimeraWorld world = GetWorld();
		TimeContainer time = world.GetTimeAndWeatherManager().GetTime();
		return time.m_iHours.ToString(2) + ":" + time.m_iMinutes.ToString(2) + ":"  + time.m_iSeconds.ToString(2);
	}
	
	void GameEndCheck() 
	{
		//TODO! Check if everyone in redfor is dead
		SCR_Faction	redfor = SCR_Faction.Cast(m_FactionManager.GetFactionByKey(m_sRedforFactionKey));
		SCR_Faction blufor = SCR_Faction.Cast(m_FactionManager.GetFactionByKey(m_sBluforFactionKey));
		array<int> bluPlayers = new array<int>;
		array<int> redPlayers = new array<int>;
		
		redfor.GetPlayersInFaction(redPlayers);
		blufor.GetPlayersInFaction(bluPlayers);
		if (false) 
		{
			Print("Blufor wins!");
				//TODO!: Add blufor player ids
			SCR_GameModeEndData endData = SCR_GameModeEndData.Create(
				EGameOverTypes.FACTION_VICTORY_SCORE,
				bluPlayers
			);
			EndGameMode(endData);
		}
		else if (SK_RedforScore >= m_iGameOverScore)
		{
			Print("Redfor wins!");
				//TODO!: Add redfor faction id and redfor player ids
			SCR_GameModeEndData endData = SCR_GameModeEndData.Create(
				EGameOverTypes.FACTION_VICTORY_SCORE,
				redPlayers
			);
			EndGameMode(endData);
		}
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_DoOnKill(string message, int redScore, int blueScoreChange)
	{
		SK_RedforScore = redScore;
		SK_BluforScore = SK_BluforScore + blueScoreChange;
		
		
		SCR_Faction blufor = SCR_Faction.Cast(m_FactionManager.GetFactionByKey(m_sBluforFactionKey));
		array<int> bluPlayers = new array<int>;
		blufor.GetPlayersInFaction(bluPlayers);
		
		foreach (int bluePlayerId: bluPlayers)
		{
			m_XPHandlerComponent.AwardXP(bluePlayerId, SCR_EXPRewards.CUSTOM_1, 1, false, blueScoreChange);
		}
		
		ShowScoreHint(message, blueScoreChange);
	}
	
	/*
		Ranks
	    -1: Renegade
		 0: Private
		8: Corporal
		16: Sergant
		24: Lieutanant
		32: Capitan
		40: Major
	*/
	
	
	//------------------------------------------------------------------------------------------------
	void SK_SerialKillersGameMode(IEntitySource src, IEntity parent)
	{
	}
	
	void ~SK_SerialKillersGameMode()
	{
		//TODO
	}
}