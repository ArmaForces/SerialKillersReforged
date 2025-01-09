[BaseContainerProps(category: "Respawn")]
class SK_SpawnLogic : SCR_SpawnLogic
{
	[Attribute("", uiwidget: UIWidgets.EditBox, category: "Respawn", desc: "Allow respawn only for this faction")]
	protected FactionKey m_sForcedFaction;
	
	[Attribute("{A1CE9D1EC16DA9BE}UI/layouts/Menus/MainMenu/SplashScreen.layout", desc: "Layout shown before deploy menu opens on client")]
	protected ResourceName m_sLoadingLayout;

	protected Widget m_wLoadingPlaceholder;
	protected SCR_LoadingSpinner m_LoadingPlaceholder;
	
	protected override void OnPlayerRegistered_S(int playerId)
	{

		super.OnPlayerRegistered_S(playerId);
	
		// Probe reconnection component first
		IEntity returnedEntity;
		if (ResolveReconnection(playerId, returnedEntity))
		{
			// User was reconnected, their entity was returned
			return;
		}

		Faction forcedFaction;
		if (GetForcedFaction(forcedFaction))
		{
			PlayerController pc = GetGame().GetPlayerManager().GetPlayerController(playerId);
			SCR_PlayerFactionAffiliationComponent playerFactionComp = SCR_PlayerFactionAffiliationComponent.Cast(pc.FindComponent(SCR_PlayerFactionAffiliationComponent));
			Faction playerFaction = playerFactionComp.GetAffiliatedFaction();
			
			
			if (playerFaction && playerFaction.GetFactionKey() != forcedFaction.GetFactionKey()) 
			{
				Print("Faction " + playerFaction.GetFactionKey() + " respawn is disabled", LogLevel.WARNING);
				return;
			}
		}
		
		
		// Send a notification to registered client:
		// Always ensure to hook OnLocalPlayer callbacks prior to sending such notification,
		// otherwise the notification will be disregarded
		GetPlayerRespawnComponent_S(playerId).NotifyReadyForSpawn_S();

	}
	
	protected bool GetForcedFaction(out Faction faction)
	{
		if (m_sForcedFaction.IsEmpty())
			return false;

		faction = GetGame().GetFactionManager().GetFactionByKey(m_sForcedFaction);
		if (!faction)
		{
			Print(string.Format("Auto spawn logic did not find faction by name: %1", m_sForcedFaction), LogLevel.WARNING);
			return false;
		}

		return true;
	}
	
	
};