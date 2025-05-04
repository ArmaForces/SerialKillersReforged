[BaseContainerProps(), SCR_ContainerActionTitle()]
class SK_ScenarioFrameworkOnTaskCompleted : SCR_ScenarioFrameworkActionBase
{
	[Attribute(desc: "Faction to award points to")]
	FactionKey m_sFactionKey;
	
	[Attribute(desc: "Award points")]
	int m_iAwardPoints;
	
	[Attribute(desc: "Name of corresponding task to fail")]
	string m_sTaskToFailName;
	
	override void Init(IEntity entity)
	{
		super.Init(entity);
		PrintFormat("ScenarioFramework: SK_ScenarioFrameworkOnTaskCompleted init on {0}",
		 entity, level: LogLevel.DEBUG);
	}
	
	override void OnActivate(IEntity object)
	{
		SK_SerialKillersGameMode game = SK_SerialKillersGameMode.Cast(GetGame().GetGameMode());
		
		if (!game)
		{
			Print("Gamemode not found!", LogLevel.ERROR);
			return;
		}
		
		game.TaskCompletedAward(m_sFactionKey, m_iAwardPoints);
		
		if (m_sTaskToFailName.IsEmpty())
			return;
		
		SCR_ScenarioFrameworkTask task = SCR_ScenarioFrameworkTask.Cast(GetGame().GetWorld().FindEntityByName(m_sTaskToFailName));
		if (!task)	
		{
			Print("Task to fail entity not found!", LogLevel.ERROR);
			return;
		}
		
		task.Fail();
	}
}