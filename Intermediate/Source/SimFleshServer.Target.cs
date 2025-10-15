using UnrealBuildTool;

public class SimFleshServerTarget : TargetRules
{
	public SimFleshServerTarget(TargetInfo Target) : base(Target)
	{
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		Type = TargetType.Server;
		ExtraModuleNames.Add("SimFlesh");
	}
}
