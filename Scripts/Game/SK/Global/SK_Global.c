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
		int i = 0;
		
		BaseWorld world = GetGame().GetWorld();
		float ground = world.GetSurfaceY(pos[0],pos[2]);
				
		while(i < 30)
		{
			i++;
			
			vector checkpos = s_AIRandomGenerator.GenerateRandomPointInRadius(0,10,pos,false);
			checkpos[1] = pos[1] + s_AIRandomGenerator.RandFloatXY(0, 2);
						
			//check if a box on that position collides with anything
			autoptr TraceBox trace = new TraceBox;
			trace.Flags = TraceFlags.ENTS;
			trace.Start = checkpos;
			trace.Mins = mins;
			trace.Maxs = maxs;
				
			if (world.TracePosition(trace, null) >= 0)
			{
				return checkpos;
			}
		}
		
		PrintFormat("Safe spawn not found %1", pos, level: LogLevel.WARNING);
		return "0 0 0";
	}
	
		
	static IEntity SpawnEntityPrefab(ResourceName prefab, vector origin, bool randomize = false, vector orientation = "0 0 0", bool global = true)
	{
		EntitySpawnParams spawnParams();

		spawnParams.TransformMode = ETransformMode.WORLD;
		Math3D.AnglesToMatrix(orientation, spawnParams.Transform);
		spawnParams.Transform[3] = origin;

		if (!global) return GetGame().SpawnEntityPrefabLocal(Resource.Load(prefab), GetGame().GetWorld(), spawnParams);

		if (randomize) 
			return GetGame().SpawnEntityPrefab(Resource.Load(SCR_EditableEntityComponentClass.GetRandomVariant(prefab)), GetGame().GetWorld(), spawnParams);
		
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
	
	static string GetLocalizationString(IEntity unit)
	{
		SCR_EditableEntityCore core = SCR_EditableEntityCore.Cast(SCR_EditableEntityCore.GetInstance(SCR_EditableEntityCore));
	       if (!core)
	           return "";

		vector posUnit = unit.GetOrigin();
		
		SCR_EditableEntityComponent nearest = core.FindNearestEntity(posUnit, EEditableEntityType.COMMENT, EEditableEntityFlag.LOCAL);
		if (!nearest)
			return "";
		GenericEntity nearestLocation = nearest.GetOwner();
		SCR_MapDescriptorComponent mapDescr = SCR_MapDescriptorComponent.Cast(nearestLocation.FindComponent(SCR_MapDescriptorComponent));
		string closestLocationName;
		if (!mapDescr)
			return "";
		MapItem item = mapDescr.Item();
		closestLocationName = item.GetDisplayName();

		vector lastLocationPos = nearestLocation.GetOrigin();
	
		LocalizedString closeLocationAzimuth;
		vector result = posUnit - lastLocationPos;
		result.Normalize();
	
		float angle1 = vector.DotXZ(result,vector.Forward);
		float angle2 = vector.DotXZ(result,vector.Right);
		const float angleA = 0.775;
		const float angleB = 0.325;
			
		if (angle2 > 0)
		{
			if (angle1 >= angleA)
				closeLocationAzimuth = "#AR-MapLocationHint_DirectionNorth";
			if (angle1 < angleA && angle1 >= angleB )
				closeLocationAzimuth = "#AR-MapLocationHint_DirectionNorthEast";
			if (angle1 < angleB && angle1 >=-angleB)
				closeLocationAzimuth = "#AR-MapLocationHint_DirectionEast";
			if (angle1 < -angleB && angle1 >=-angleA)
				closeLocationAzimuth = "#AR-MapLocationHint_DirectionSouthEast";
			if (angle1 < -angleA)
				closeLocationAzimuth = "#AR-MapLocationHint_DirectionSouth";
		}
		else
		{
			if (angle1 >= angleA)
				closeLocationAzimuth = "#AR-MapLocationHint_DirectionNorth";
			if (angle1 < angleA && angle1 >= angleB )
				closeLocationAzimuth = "#AR-MapLocationHint_DirectionNorthWest";
			if (angle1 < angleB && angle1 >=-angleB)
				closeLocationAzimuth = "#AR-MapLocationHint_DirectionWest";
			if (angle1 < -angleB && angle1 >=-angleA)
				closeLocationAzimuth = "#AR-MapLocationHint_DirectionSouthWest";
			if (angle1 < -angleA)
				closeLocationAzimuth = "#AR-MapLocationHint_DirectionSouth";
		};
		
		return WidgetManager.Translate(closeLocationAzimuth, closestLocationName);
	}
}