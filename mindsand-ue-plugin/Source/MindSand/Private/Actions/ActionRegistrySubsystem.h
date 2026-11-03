// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "ActionRegistrySubsystem.generated.h"

class UActionDescriptionAsset;

UCLASS()
class MINDSAND_API UActionRegistrySubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

  public:
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "MindSand|Actions")
    UActionDescriptionAsset *FindActionByName(const FString &ActionName) const;

    UFUNCTION(BlueprintCallable, Category = "MindSand|Actions")
    TArray<UActionDescriptionAsset *> GetActions(const TArray<FString> &ActionNames) const;

  private:
    UPROPERTY()
    TArray<TObjectPtr<UActionDescriptionAsset>> Actions;

    TMap<FString, TWeakObjectPtr<UActionDescriptionAsset>> ActionsByName;

    void LoadActionsFromAssetManager();

    IConsoleCommand *DumpActionsCmd = nullptr;
};
