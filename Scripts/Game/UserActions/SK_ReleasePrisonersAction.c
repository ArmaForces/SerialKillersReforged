class SK_ReleasePrisonersAction : ScriptedUserAction
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
	}
}