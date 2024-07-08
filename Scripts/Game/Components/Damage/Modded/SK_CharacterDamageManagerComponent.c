modded class SCR_CharacterDamageManagerComponent : SCR_DamageManagerComponent
{
	
	override void OnInit(IEntity owner) 
	{
		GetOnDamageStateChanged().Insert(SK_OnDamageStateChanged);
		super.OnInit(owner);
	}
	
	void SK_OnDamageStateChanged(EDamageState state)
	{		
		Instigator instigator = GetInstigator();
		
		if(state == EDamageState.DESTROYED)
		{
			SK_SerialKillersGameMode gm = SK_SerialKillersGameMode.Cast(GetGame().GetGameMode());
			gm.OnUnitKilled(GetOwner(), instigator);
		}		
	}
}