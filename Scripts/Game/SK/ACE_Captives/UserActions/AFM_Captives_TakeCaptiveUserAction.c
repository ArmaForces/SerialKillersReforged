//------------------------------------------------------------------------------------------------
modded class ACE_Captives_TakeCaptiveUserAction : ACE_InstantGadgetUserAction
{
	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		
		IEntity gadget = GetHeldGadget(user);
		if (!gadget)
			return false;
		
		if (!gadget.FindComponent(ACE_Captives_HandcuffsGadgetComponent))
			return false;
		
		SCR_ChimeraCharacter ownerChar = SCR_ChimeraCharacter.Cast(GetOwner());
		if (!ownerChar)
			return false;
		
		if (ownerChar.IsInVehicle() && !ACE_AnimationTools.GetHelperCompartment(ownerChar))
			return false;
		
		SCR_CharacterControllerComponent ownerCharController = SCR_CharacterControllerComponent.Cast(ownerChar.GetCharacterController());
		if (!ownerCharController)
			return false;
		
		if (ownerCharController.ACE_IsCarried())
			return false;
		
		if (ownerCharController.ACE_Captives_IsCaptive())
			return false;
		
		return true;
	}
}
