// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "Modules/ModuleManager.h"

class FSpawnTabArgs;
class SDockTab;

/**
 * Editor-only companion to the MindSand runtime module. Hosts the Plan Injector tab
 * (Tools > MindSand > Plan Injector), which feeds hand-written plan JSON into
 * UMindSandDebugUtils::ExecutePlan against the running Play-In-Editor session.
 */
class FMindSandEditorModule : public IModuleInterface
{
  public:
    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    /** Tab id registered with the global tab manager. */
    static const FName PlanInjectorTabName;

  private:
    /** Adds the Tools > MindSand > Plan Injector entry once UToolMenus is ready. */
    void RegisterMenus();

    TSharedRef<SDockTab> SpawnPlanInjectorTab(const FSpawnTabArgs &Args);

    static void OpenPlanInjectorTab();
};
