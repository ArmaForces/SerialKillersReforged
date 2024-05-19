class SK_Global 
{
	static SK_SerialKillersConfigComponent GetConfig()
	{
		return SK_SerialKillersConfigComponent.GetInstance();
	}
	
	static SK_CivilianManagerComponent GetCiviliansManager()
	{
		return SK_CivilianManagerComponent.GetInstance();
	}
	
	static vector FindSafeSpawnPosition(vector pos, vector mins = "-0.5 0 -0.5", vector maxs = "0.5 2 0.5")
	{
		//a crude and brute-force way to find a spawn position, try to improve this later
		vector foundpos = pos;
		int i = 0;
		
		BaseWorld world = GetGame().GetWorld();
		float ground = world.GetSurfaceY(pos[0],pos[2]);
				
		while(i < 30)
		{
			i++;
			
			//Get a random vector in a 3m radius sphere centered on pos and above the ground
			vector checkpos = s_AIRandomGenerator.GenerateRandomPointInRadius(0,3,pos,false);
			checkpos[1] = pos[1] + s_AIRandomGenerator.RandFloatXY(0, 2);
						
			//check if a box on that position collides with anything
			autoptr TraceBox trace = new TraceBox;
			trace.Flags = TraceFlags.ENTS;
			trace.Start = checkpos;
			trace.Mins = mins;
			trace.Maxs = maxs;
			
			float result = world.TracePosition(trace, null);
				
			if (result < 0)
			{
				//collision, try again
				continue;
			}else{
				//no collision, this pos is safe
				foundpos = checkpos;
				break;
			}
		}
		
		return foundpos;
	}
	
		
	static IEntity SpawnEntityPrefab(ResourceName prefab, vector origin, bool randomize = false, vector orientation = "0 0 0", bool global = true)
	{
		EntitySpawnParams spawnParams();

		spawnParams.TransformMode = ETransformMode.WORLD;

		Math3D.AnglesToMatrix(orientation, spawnParams.Transform);
		spawnParams.Transform[3] = origin;

		if (!global) return GetGame().SpawnEntityPrefabLocal(Resource.Load(prefab), GetGame().GetWorld(), spawnParams);

		//TODO! Enable randomization in call below
		return GetGame().SpawnEntityPrefab(Resource.Load(prefab), GetGame().GetWorld(), spawnParams);
	}
	
		
	static bool IsOceanAtPosition(vector checkpos)
	{		
		World world = GetGame().GetWorld();
		return world.GetOceanBaseHeight() > world.GetSurfaceY(checkpos[0],checkpos[2]);
	}
	
	static vector GetRandomNonOceanPositionNear(vector pos, float range)
	{
		int i = 0;
		while(i < 30)
		{
			i++;			
			
			vector checkpos = s_AIRandomGenerator.GenerateRandomPointInRadius(0,range,pos,false);
			
			if(!IsOceanAtPosition(checkpos))
			{	
				checkpos[1] = GetGame().GetWorld().GetSurfaceY(checkpos[0],checkpos[2]) + 1;
				return checkpos;
			}
		}
		
		return pos;
	}
}