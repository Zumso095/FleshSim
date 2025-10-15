using UnrealBuildTool;

public class SimFleshTarget : TargetRules
{
	public SimFleshTarget(TargetInfo Target) : base(Target)
	{
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		Type = TargetType.Game;
		ExtraModuleNames.Add("SimFlesh");
	}
}
