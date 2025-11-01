class SK_MapMarkerComponentClass :ScriptComponentClass
{
}

class SK_MapMarkerComponent :ScriptComponent
{
	[Attribute("5", desc: "Refresh rate in seconds")]
	int m_iRefreshRateSeconds;
	
	protected IEntity m_Owner;
	
	[RplProp()]
	protected vector m_lastPos = "0 0 0";
	
	protected override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		m_Owner = owner;
		
		GetGame().GetCallqueue().CallLater(CreateMapMarker, 5 * 1000);
	}
	
	protected void CreateMapMarker()
	{
		
		GetGame().GetCallqueue().CallLater(UpdateMapMarker, m_iRefreshRateSeconds * 1000, true);
	}
	
	protected void UpdateMapMarker()
	{
		if (!m_Owner)
			return;
		
		vector newPos = m_Owner.GetOrigin();
		if (vector.Distance(m_lastPos, newPos) < 1)
			return;
		m_lastPos = newPos;
		
	}
}