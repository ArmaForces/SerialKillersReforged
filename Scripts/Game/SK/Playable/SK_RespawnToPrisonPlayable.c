class SK_RespawnToPrisonPlayableComponentClass: PS_PlayableComponentClass
{
}

class SK_RespawnToPrisonPlayableComponent: PS_PlayableComponent
{
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		if(SCR_Global.IsEditMode() || !Replication.IsServer())
			return;
		
		IEntity prison = GetGame().GetWorld().FindEntityByName(SK_PrisonManagerComponent.PRISON_ENTITY_NAME);
		if (!prison)
		{
			Print("Unable to find prison_spawn prefab, check world and prefab name!", LogLevel.ERROR);
		 	return;
		}
		
		prison.GetTransform(spawnTransform);
	}
	
	override ResourceName GetNextRespawn(bool nextPrefab)
	{
		if (!m_aRespawnPrefabs.IsIndexValid(0))
			return super.GetNextRespawn(nextPrefab);
		
		return m_aRespawnPrefabs[0];
	}
}