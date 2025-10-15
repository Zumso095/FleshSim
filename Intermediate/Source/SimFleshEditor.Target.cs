using UnrealBuildTool;

public class SimFleshEditorTarget : TargetRules
{
	public SimFleshEditorTarget(TargetInfo Target) : base(Target)
	{
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		Type = TargetType.Editor;
		ExtraModuleNames.Add("SimFlesh");
	}
}
