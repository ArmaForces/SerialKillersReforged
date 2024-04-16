/*class SK_MapPOIData: Managed
{
	ref SCR_UIInfo m_UiInfo;
}

class SK_MapIcons: SCR_MapUIBaseComponent
{
	[Attribute()]
	protected ResourceName m_Layout;
	
	override void OnMapOpen(MapConfiguration config) 
	{
		super.onMapOpen(config);
		BaseWorld world = GetGame().GetWorld();
		
		SK_CivilianManagerComponent civiliansManager = SK_Global.GetCiviliansManager();
		foreach(EntityID civID : civiliansManager.GetCivilians())
		{
			IEntity civ = world.FindEntityByID(civID);
			
		}
	}
}*/