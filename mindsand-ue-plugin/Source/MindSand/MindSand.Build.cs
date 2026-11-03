// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

using UnrealBuildTool;

public class MindSand : ModuleRules
{
    public MindSand(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core"
        });

        PrivateDependencyModuleNames.AddRange(new string[] {
            "CoreUObject", "Engine", "Slate", "SlateCore", "AIModule", "LLMPipelines", "Json", "JsonUtilities",
            "GameplayTasks", "GameplayTags", "ActorRegistry", "LatencyProfiler", "AssetRegistry", "DeveloperSettings"
        });
    }
}
