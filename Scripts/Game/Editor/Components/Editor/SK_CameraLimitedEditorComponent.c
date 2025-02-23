/** @ingroup Editor_Components
*/
/*!
Limited version of camera manager.

Contains two prefabs - one used only when all editor modes have also limited camera, and the other when there is at least one unlimited camera.
*/
modded class SCR_CameraLimitedEditorComponent : SCR_CameraEditorComponent
{
	override protected ResourceName GetCameraPrefab()
	{
		SCR_EditorManagerEntity manager = SCR_EditorManagerEntity.GetInstance();
		if (!manager) 
			return ResourceName.Empty;
		
		PlayerController pc = GetGame().GetPlayerController();
		SK_PrisonerComponent prisoner = SK_PrisonerComponent.Cast(pc.FindComponent(SK_PrisonerComponent));
		bool isPrisoner = prisoner && prisoner.GetState();
		
		bool isLimited = manager.IsLimited() && !isPrisoner;
		

		if (isLimited)
			return m_LimitedCameraPrefab;
		else
			return m_CameraPrefab;
	}
}
