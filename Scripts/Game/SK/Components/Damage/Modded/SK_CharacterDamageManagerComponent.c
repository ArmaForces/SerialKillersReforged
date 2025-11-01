modded class SCR_CharacterDamageManagerComponent : SCR_DamageManagerComponent
{
	
	override void OnPostInit(IEntity owner) 
	{
		GetOnDamageStateChanged().Insert(SK_OnDamageStateChanged);
		super.OnPostInit(owner);
	}
	
	void SK_OnDamageStateChanged(EDamageState state)
	{		
		Instigator instigator = GetInstigator();
		IEntity owner = GetOwner();
		
		if(state == EDamageState.DESTROYED)
		{
			SK_SerialKillersGameMode gm = SK_SerialKillersGameMode.Cast(GetGame().GetGameMode());
			gm.OnUnitKilled(owner, instigator);
		}		
	}
}