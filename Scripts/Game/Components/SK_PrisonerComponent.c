[EntityEditorProps(category: "GameScripted/Components", description: "Is PC a prisoner?")]
class SK_PrisonerComponentClass : ScriptComponentClass
{
}

class SK_PrisonerComponent : ScriptComponent
{
	[RplProp()]
	protected bool m_bIsPrisoner = false;
	
	void SetState(bool isPrisoner)
	{
		Rpc(RpcAsk_SetState, isPrisoner);
	}
	
	bool GetState()
	{
		return m_bIsPrisoner;
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_SetState(bool state)
	{
		if (m_bIsPrisoner == state)
			return;
		
		m_bIsPrisoner = state;
		Replication.BumpMe();
	}
}
