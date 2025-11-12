// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Multi : ModuleRules
{
	public Multi(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"Multi",
			"Multi/Variant_Horror",
			"Multi/Variant_Horror/UI",
			"Multi/Variant_Shooter",
			"Multi/Variant_Shooter/AI",
			"Multi/Variant_Shooter/UI",
			"Multi/Variant_Shooter/Weapons"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
		
		PublicDependencyModuleNames.AddRange(new string[] { "OnlineServicesInterface", "CoreOnline" });
	}
}
