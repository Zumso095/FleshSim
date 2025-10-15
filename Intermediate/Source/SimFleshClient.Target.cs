using UnrealBuildTool;

public class SimFleshClientTarget : TargetRules
{
	public SimFleshClientTarget(TargetInfo Target) : base(Target)
	{
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		Type = TargetType.Client;
		ExtraModuleNames.Add("SimFlesh");
	}
}
