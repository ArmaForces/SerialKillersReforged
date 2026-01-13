[EntityEditorProps(category: "GameScripted/Components", description: "Is PC a prisoner?")]
class SK_PrisonerComponentClass : ScriptComponentClass
{
}

class SK_PrisonerComponent : ScriptComponent
{
	[RplProp()]
	protected bool m_bIsPrisoner = false;
	
	[RplProp()]
	protected int m_iPlayerId = -1;
	
	[RplProp()]
	protected RplId m_iPlayableId = RplId.Invalid();
	
	void SetPrisoner(int playerId, RplId playableId)
	{
		//if (!IsMaster())
		//	return;
		
		m_iPlayerId = playerId;
		m_iPlayableId = playableId;
		m_bIsPrisoner = true;
		
		Replication.BumpMe();
	}
	
	bool GetState()
	{
		return m_bIsPrisoner;
	}
	
	void FreePrisoner(vector position)
	{
		Rpc(RpcAsk_SwitchToPrisonerEntity, position);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_SwitchToPrisonerEntity(vector position)
	{
		m_bIsPrisoner = false;
		
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		playableManager.SetPlayerPlayable(m_iPlayerId, m_iPlayableId);
		playableManager.ForceSwitch(m_iPlayerId);
		
		SCR_Global.TeleportPlayer(m_iPlayerId, position, SCR_EPlayerTeleportedReason.FAST_TRAVEL);
		
		Replication.BumpMe();
	}
	
	
	
	
	/*protected bool IsMaster()
	{
		return (!m_RplComponent || m_RplComponent.IsMaster());
	}
	*/
}
