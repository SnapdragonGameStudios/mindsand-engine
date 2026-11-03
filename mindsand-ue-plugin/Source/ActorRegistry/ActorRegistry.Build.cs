// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

using UnrealBuildTool;

public class ActorRegistry : ModuleRules
{
    public ActorRegistry(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[] { "Core", "CoreUObject", "Engine", "GameplayTags", "DeveloperSettings" });

        PrivateDependencyModuleNames.AddRange(new string[] { "AssetRegistry", "Json" });
    }
}
