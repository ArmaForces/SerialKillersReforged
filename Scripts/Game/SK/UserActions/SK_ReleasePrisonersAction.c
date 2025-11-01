class SK_ReleasePrisonerAction : ScriptedUserAction
{
	override bool CanBePerformedScript(IEntity user)
	{
		if (SCR_PlayerController.GetLocalControlledEntity() != user)
			return false;

		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!factionManager)
			return false;

		Faction faction = factionManager.GetLocalPlayerFaction();
		if (!faction)
			return false;

		return faction.GetFactionKey() == "USSR";
	}
	
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		Print("Releasing prisoners...", LogLevel.DEBUG);
		GetPrisonManager().FreeRandomPrisoner();
	}
	
	protected SK_PrisonManagerComponent GetPrisonManager()
	{
		IEntity sp = GetGame().GetWorld().FindEntityByName("prison_spawn");
		if (!sp)
		{
			Print("No prison entity found in world! Please make sure spawn point exists and its named prison_spawn", LogLevel.ERROR);
			return null;
		}
		
		return SK_PrisonManagerComponent.Cast(sp.FindComponent(SK_PrisonManagerComponent));
	}
}
