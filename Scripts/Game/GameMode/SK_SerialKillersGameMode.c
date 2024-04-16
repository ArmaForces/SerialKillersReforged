class SK_SerialKillersGameModeClass: SCR_BaseGameModeClass
{
}

class SK_SerialKillersGameMode : SCR_BaseGameMode
{
	protected SK_SerialKillersConfigComponent m_Config;
	protected SK_CivilianManagerComponent m_CiviliansManager;
	
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
	
	//------------------------------------------------------------------------------------------------
	void SK_SerialKillersGameMode(IEntitySource src, IEntity parent)
	{
	}
	
	void ~SK_SerialKillersGameMode()
	{
		//TODO
	}
}