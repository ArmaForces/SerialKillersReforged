class SK_XPHandlerComponentClass : SCR_XPHandlerComponentClass
{
}

class SK_XPHandlerComponent : SCR_XPHandlerComponent
{
	//------------------------------------------------------------------------------------------------
	override void OnPlayerSpawnFinalize_S(SCR_SpawnRequestComponent requestComponent, SCR_SpawnHandlerComponent handlerComponent, SCR_SpawnData data, IEntity entity)
	{
		super.OnPlayerSpawnFinalize_S(requestComponent, handlerComponent, data, entity);
		
		if (!requestComponent || !entity)
			return;

		PlayerController pc = requestComponent.GetPlayerController();

		if (!pc)
			return;

		SCR_PlayerXPHandlerComponent compXP = SCR_PlayerXPHandlerComponent.Cast(pc.FindComponent(SCR_PlayerXPHandlerComponent));
		SCR_PlayerFactionAffiliationComponent compFaction = SCR_PlayerFactionAffiliationComponent.Cast(pc.FindComponent(SCR_PlayerFactionAffiliationComponent));
		

		if (compXP && compFaction && compFaction.GetAffiliatedFactionKey() == "US")
		{
			SK_SerialKillersGameMode gm = SK_SerialKillersGameMode.Cast(GetGame().GetGameMode());
			int currentXP = compXP.GetPlayerXP();
			int bluforScore = gm.GetBluforScore();
			
			if (currentXP < bluforScore) 
			{
				compXP.AddPlayerXP(SCR_EXPRewards.CUSTOM_1, 1, false, bluforScore - currentXP);
				compXP.UpdatePlayerRank(false);
			}
			
		}
	}

}