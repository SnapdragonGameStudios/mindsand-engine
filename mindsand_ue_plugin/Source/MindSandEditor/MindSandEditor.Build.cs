// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

using UnrealBuildTool;

public class MindSandEditor : ModuleRules
{
    public MindSandEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core"
        });

        PrivateDependencyModuleNames.AddRange(new string[] {
            "CoreUObject", "Engine", "Slate", "SlateCore", "InputCore", "UnrealEd", "ToolMenus",
            "WorkspaceMenuStructure", "Json", "MindSand"
        });
    }
}
