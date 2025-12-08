// Copyright 2025 RLoris

using UnrealBuildTool;

public class FileHelper : ModuleRules
{
	public FileHelper(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        PrecompileForTargets = PrecompileTargetsType.Any;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"Engine",
				"JsonUtilities",
				"Json"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"ImageCore",
				"ImageWriteQueue",
				"Slate",
				"SlateCore"
			}
		);
	}
}
