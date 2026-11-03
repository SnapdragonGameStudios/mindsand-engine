// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "ActionRegistrySubsystem.h"
#include "Actions/ActionDescriptionAsset.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "HAL/IConsoleManager.h"
#include "MindSandLog.h"

static const FPrimaryAssetType ActionDescriptionAssetType(TEXT("ActionDescription"));

void UActionRegistrySubsystem::Initialize(FSubsystemCollectionBase &Collection)
{
    Super::Initialize(Collection);

    LoadActionsFromAssetManager();

    DumpActionsCmd = IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("MindSand.DumpActions"), TEXT("Print every action registered with UActionRegistrySubsystem."),
        FConsoleCommandDelegate::CreateWeakLambda(
            this,
            [this]() {
                UE_LOG(LogMindSand, Log, TEXT("ActionRegistry: %d actions"), Actions.Num());
                for (const TObjectPtr<UActionDescriptionAsset> &A : Actions)
                {
                    if (A)
                    {
                        UE_LOG(LogMindSand, Log, TEXT("  %s -> %s"), *A->ActionName, *A->GetPathName());
                    }
                }
            }),
        ECVF_Default);
}

void UActionRegistrySubsystem::Deinitialize()
{
    if (DumpActionsCmd)
    {
        IConsoleManager::Get().UnregisterConsoleObject(DumpActionsCmd);
        DumpActionsCmd = nullptr;
    }

    Actions.Reset();
    ActionsByName.Reset();

    Super::Deinitialize();
}

UActionDescriptionAsset *UActionRegistrySubsystem::FindActionByName(const FString &ActionName) const
{
    if (ActionName.IsEmpty())
    {
        return nullptr;
    }

    if (const TWeakObjectPtr<UActionDescriptionAsset> *Found = ActionsByName.Find(ActionName))
    {
        return Found->Get();
    }
    return nullptr;
}

TArray<UActionDescriptionAsset *> UActionRegistrySubsystem::GetActions(const TArray<FString> &ActionNames) const
{
    TArray<UActionDescriptionAsset *> Result;
    Result.Reserve(ActionNames.Num());
    for (const FString &Name : ActionNames)
    {
        if (UActionDescriptionAsset *A = FindActionByName(Name))
        {
            Result.Add(A);
        }
        else
        {
            UE_LOG(LogMindSand, Warning, TEXT("ActionRegistry::GetActions: no action registered with name '%s'."),
                   *Name);
        }
    }
    return Result;
}

void UActionRegistrySubsystem::LoadActionsFromAssetManager()
{
    UAssetManager &AM = UAssetManager::Get();

    // Recursively scan the project and plugin content roots. ScanPathsForPrimaryAssets
    // recurses into subfolders by default.
    const TArray<FString> ScanRoots = {TEXT("/Game"), TEXT("/MindSand")};
    AM.ScanPathsForPrimaryAssets(ActionDescriptionAssetType, ScanRoots, UActionDescriptionAsset::StaticClass(),
                                 /*bHasBlueprintClasses*/ false,
                                 /*bIsEditorOnly*/ false,
                                 /*bForceSynchronousScan*/ true);

    TArray<FPrimaryAssetId> Ids;
    AM.GetPrimaryAssetIdList(ActionDescriptionAssetType, Ids);

    if (Ids.Num() == 0)
    {
        UE_LOG(LogMindSand, Warning,
               TEXT("ActionRegistry: no primary assets of type '%s' found under /Game or /MindSand."),
               *ActionDescriptionAssetType.ToString());
        return;
    }

    if (TSharedPtr<FStreamableHandle> Handle = AM.LoadPrimaryAssets(Ids))
    {
        Handle->WaitUntilComplete();
    }

    Actions.Reserve(Ids.Num());
    ActionsByName.Reserve(Ids.Num());

    for (const FPrimaryAssetId &Id : Ids)
    {
        UActionDescriptionAsset *A = Cast<UActionDescriptionAsset>(AM.GetPrimaryAssetObject(Id));
        if (!A)
        {
            UE_LOG(LogMindSand, Warning, TEXT("ActionRegistry: primary asset %s failed to load or cast."),
                   *Id.ToString());
            continue;
        }

        if (A->ActionName.IsEmpty())
        {
            UE_LOG(LogMindSand, Warning, TEXT("ActionRegistry: asset %s has empty ActionName; skipping."),
                   *A->GetPathName());
            continue;
        }

        if (ActionsByName.Contains(A->ActionName))
        {
            UE_LOG(LogMindSand, Warning,
                   TEXT("ActionRegistry: duplicate ActionName '%s' (asset %s); keeping first entry."), *A->ActionName,
                   *A->GetPathName());
            continue;
        }

        Actions.Add(A);
        ActionsByName.Add(A->ActionName, A);
    }

    UE_LOG(LogMindSand, Log, TEXT("ActionRegistry: registered %d actions."), Actions.Num());
    for (const TObjectPtr<UActionDescriptionAsset> &A : Actions)
    {
        UE_LOG(LogMindSand, Log, TEXT("  %s"), *A->ActionName);
    }
}
