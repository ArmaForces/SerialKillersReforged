[EntityEditorProps(category: "GameScripted/Components", description: "Manages prisoners")]
class SK_PrisonManagerComponentClass : ScriptComponentClass
{
}

class SK_PrisonManagerComponent : ScriptComponent
{
	static string PRISON_ENTITY_NAME = "prison_spawn";
	
	protected ref array<ref int> m_aImprisonedPlayers = new array<ref int>;
	protected RplComponent m_RplComponent;
	protected static SK_PrisonManagerComponent m_Manager = null;
	
	void Init(IEntity owner)
	{
		m_RplComponent = RplComponent.Cast(owner.FindComponent(RplComponent));
	}
	
	void RegisterPrisoner(int playerId)
	{
		if (!IsMaster())
			return;
		
		foreach(int id: m_aImprisonedPlayers)
		{
			if (id == playerId)
				return;
		}
		
		m_aImprisonedPlayers.Insert(playerId);
	}
	
	void FreeRandomPrisoner()
	{
		Rpc(RpcAsk_FreePrisoner);
	}
	
	bool IsPrisonEmpty()
	{
		return m_aImprisonedPlayers.Count() == 0;
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_FreePrisoner()
	{
		if (IsPrisonEmpty())
			return;
		
		int randomIndex = m_aImprisonedPlayers.GetRandomIndex();
		int playerId = m_aImprisonedPlayers[randomIndex];
		
		PlayerController pc = GetGame().GetPlayerManager().GetPlayerController(playerId);
		SK_PrisonerComponent prisoner = SK_PrisonerComponent.Cast(pc.FindComponent(SK_PrisonerComponent));
		prisoner.FreePrisoner(GetOwner().GetOrigin() + "15 0 0");
		
		m_aImprisonedPlayers.Remove(randomIndex);
	}
	
	protected bool IsMaster()
	{
		return (!m_RplComponent || m_RplComponent.IsMaster());
	}
	
	static SK_PrisonManagerComponent GetInstance()
	{
		if (m_Manager)
			return m_Manager;
		
		IEntity prison = GetGame().GetWorld().FindEntityByName(PRISON_ENTITY_NAME);
		
		if (!prison)
		{
			Print("Unable to find prison entity, check world", LogLevel.ERROR);
			return null;
		}
		
		m_Manager = SK_PrisonManagerComponent.Cast(prison.FindComponent(SK_PrisonManagerComponent));
		if (!m_Manager)
		{
			Print("SK_PrisonManagerComponent is missing from prison prefab!", LogLevel.ERROR);
			return null;
		}
		
		return m_Manager;
	}
	
}
