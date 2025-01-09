class SK_SerialKillersGameModeClass: SCR_BaseGameModeClass
{
}

class SK_SerialKillersGameMode : SCR_BaseGameMode
{
	protected SK_SerialKillersConfigComponent m_Config;
	protected SK_CivilianManagerComponent m_CiviliansManager;
	
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
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		
		if(SCR_Global.IsEditMode())
			return;
		
		Print("SerialKillers gamemode initialising!", LogLevel.NORMAL);
		
		m_Config = SK_Global.GetConfig();
		m_CiviliansManager = SK_Global.GetCiviliansManager();
		
		if (m_CiviliansManager) 
		{
			Print("Initialising civilians", LogLevel.NORMAL);
			m_CiviliansManager.Init(this);
		}
	}
	
	void OnUnitKilled(IEntity unit, Instigator instigator)
	{
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
		
		if (instigatorFaction)
		{
			Print("Killer faction: " + instigatorFaction.GetFactionKey());
		}
		
		if (killedFaction)
		{
			Print("Victim faction: " + killedFaction.GetFactionKey());
		} 
		else 
		{
			Print("Victim faction unknown - returning", LogLevel.WARNING);
			return;
		}
		
		switch (killedFaction.GetFactionKey()) 
		{
			case m_sRedforFactionKey:
				return;
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
		int redScore = 2*m_iCivKilledScore, bluScore = 0;
		if (instigatorFaction) 
		{
			if (instigatorFaction.GetFactionKey() == "US")
			{
				bluScore = -2 * m_iCivKilledScore;
				return;
			} 
			else 
			{
				bluScore = m_iCivKilledScore;
			}
		}
		
		SK_RedforScore += redScore;
		SK_BluforScore += bluScore;
		ShowScoreHint(redScore, bluScore, "Cop was killed");
	}
	
	void HandleCivKill (IEntity unit, Faction instigatorFaction)
	{
		int redScore = m_iCivKilledScore, bluScore = 0;
		if (instigatorFaction) 
		{
			if (instigatorFaction.GetFactionKey() == "US")
			{
				bluScore = -m_iCivKilledScore;
			} 
			else 
			{
				bluScore = m_iCivKilledScore;
			}
		}
		
		SK_RedforScore += redScore;
		SK_BluforScore += bluScore;
		ShowScoreHint(redScore, bluScore, "Civilian was killed");
	}
	
	void ShowScoreHint(int scoreChange, int eqScoreChange, string message)
	{
		ChimeraWorld world = GetWorld();
		TimeContainer time = world.GetTimeAndWeatherManager().GetTime();
		
		string msg = "\t" + time.m_iHours.ToString(2) + ":" + time.m_iMinutes.ToString(2) + ":"  + time.m_iSeconds.ToString(2) + "\n";
		msg = msg + "_____________\n";
		msg = msg + "\t Killers\n";
		msg = msg + "\t " + SK_RedforScore + "/" + m_iGameOverScore + " (" + scoreChange + ")\n";
		msg = msg + "\n";
		msg = msg + "\t Police\n";
		msg = msg + "\t " + SK_BluforScore + "/" + m_iBluforResourcesScore + " (" + eqScoreChange + ")\n";
		msg = msg + message;
		
		SCR_HintManagerComponent.GetInstance().ShowCustom(msg, "", 10, false);
	}
	
	void GameEndCheck() 
	{
		//TODO! Check if everyone in redfor is dead
		SCR_FactionManager fm = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		SCR_Faction	redfor = SCR_Faction.Cast(fm.GetFactionByKey(m_sRedforFactionKey));
		SCR_Faction blufor = SCR_Faction.Cast(fm.GetFactionByKey(m_sBluforFactionKey));
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
	
	//------------------------------------------------------------------------------------------------
	void SK_SerialKillersGameMode(IEntitySource src, IEntity parent)
	{
	}
	
	void ~SK_SerialKillersGameMode()
	{
		//TODO
	}
}