// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "MindSandEditorModule.h"

#include "Framework/Docking/TabManager.h"
#include "SMindSandPlanInjectorPanel.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

#define LOCTEXT_NAMESPACE "FMindSandEditorModule"

const FName FMindSandEditorModule::PlanInjectorTabName(TEXT("MindSandPlanInjector"));

void FMindSandEditorModule::StartupModule()
{
    FGlobalTabmanager::Get()
        ->RegisterNomadTabSpawner(PlanInjectorTabName,
                                  FOnSpawnTab::CreateRaw(this, &FMindSandEditorModule::SpawnPlanInjectorTab))
        .SetDisplayName(LOCTEXT("PlanInjectorTabTitle", "MindSand Plan Injector"))
        .SetTooltipText(LOCTEXT("PlanInjectorTabTooltip", "Feed a hand-written plan JSON to the AI teammate "
                                                          "orchestrator during Play-In-Editor."))
        .SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory());

    UToolMenus::RegisterStartupCallback(
        FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FMindSandEditorModule::RegisterMenus));
}

void FMindSandEditorModule::ShutdownModule()
{
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);

    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(PlanInjectorTabName);
}

void FMindSandEditorModule::RegisterMenus()
{
    FToolMenuOwnerScoped OwnerScoped(this);

    UToolMenu *ToolsMenu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.Tools"));
    if (ToolsMenu == nullptr)
    {
        return;
    }

    FToolMenuSection &Section = ToolsMenu->FindOrAddSection(TEXT("MindSand"), LOCTEXT("MindSandSection", "MindSand"));
    Section.AddMenuEntry(TEXT("MindSandPlanInjector"), LOCTEXT("PlanInjectorMenuLabel", "Plan Injector"),
                         LOCTEXT("PlanInjectorMenuTooltip", "Open the MindSand Plan Injector window."), FSlateIcon(),
                         FUIAction(FExecuteAction::CreateStatic(&FMindSandEditorModule::OpenPlanInjectorTab)));
}

TSharedRef<SDockTab> FMindSandEditorModule::SpawnPlanInjectorTab(const FSpawnTabArgs &Args)
{
    return SNew(SDockTab).TabRole(ETabRole::NomadTab)[SNew(SMindSandPlanInjectorPanel)];
}

void FMindSandEditorModule::OpenPlanInjectorTab()
{
    FGlobalTabmanager::Get()->TryInvokeTab(FTabId(PlanInjectorTabName));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMindSandEditorModule, MindSandEditor)
