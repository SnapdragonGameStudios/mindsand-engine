// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "Actions/ActionDescriptionAsset.h"
#include "Components/ActorComponent.h"
#include "Core/MindSandTypes.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Teammates/TeammateCharacterAgentDescriptor.h"
#include <Actions/ActionPlanEngine.h>

#include "TeammateCharacterAgentComponent.generated.h"

class UAITeammateOrchestrator;
class UAIPerceptionComponent;
struct FParsedPlan;
class FActionPlanEngine;

UCLASS(Blueprintable, BlueprintType, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MINDSAND_API UTeammateCharacterAgentComponent : public UActorComponent
{
    GENERATED_BODY()

  public:
    // Sets default values for this component's properties
    UTeammateCharacterAgentComponent();

  protected:
    // Called when the game starts
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION()
    void HandlePossessedPawnChanged(APawn *OldPawn, APawn *NewPawn);

  public:
    // Caches the possessed pawn's UTeammateCharacterAgentDescriptor into this component and adds it
    // to UTeammateRegistry. Called automatically on possession when the descriptor has
    // bRegisterOnPossess set; call it manually otherwise.
    //
    // Warns and does nothing when there is no possessed pawn, no descriptor on that pawn, or no
    // registry. Safe to call repeatedly: the previous registry entry is dropped first, so a
    // descriptor whose Desc.Squad changed moves squads instead of orphaning this component under
    // the old key. Each call resyncs the whole of Desc from the descriptor, which includes rebuilding
    // Desc.Actions -- any runtime RegisterActions/UnRegisterActions edits are discarded.
    UFUNCTION(BlueprintCallable, Category = "AITeammate|TeammateCharacterAgentComponent")
    void RegisterTeammate();

  private:
    // Located when the possessed pawn changes and re-affirmed by RegisterTeammate(); checked on every
    // identity-getter call and re-resolved on a cache miss so a descriptor added to the pawn after
    // possession is still found.
    mutable TWeakObjectPtr<UTeammateCharacterAgentDescriptor> CachedDescriptor;

    // The live, validated action set -- RegisterAction()/RegisterTeammate() populate it from the
    // descriptor's authored actions; RegisterActions/UnRegisterActions mutate it at runtime.
    UPROPERTY()
    TArray<FString> LiveActions;

    UTeammateCharacterAgentDescriptor *ResolveDescriptor() const;

    void Reset();

    UFUNCTION(BlueprintCallable, Category = "AITeammate|TeammateCharacterAgentComponent")
    bool RegisterActions(const TArray<FString> &ActionName);
    UFUNCTION(BlueprintCallable, Category = "AITeammate|TeammateCharacterAgentComponent")
    bool UnRegisterActions(const TArray<FString> &ActionName);

    bool RegisterAction(const FString &ActionName);
    bool UnRegisterAction(const FString &ActionName);

  public:
    UFUNCTION(BlueprintCallable, Category = "AITeammate|TeammateCharacterAgentComponent")
    bool UpdateStateVariableValue(const FString &VariableName, bool value);

    // ActionPlan Engine
    FActionPlanEngine *GetActionPlanEngine();
    void ReadStateVariables(TMap<FString, bool> &Results) const;
    UFUNCTION(BlueprintCallable, Category = "AITeammate|TeammateCharacterAgentComponent")
    void SetInterruptLogic(bool value);
    UFUNCTION(BlueprintCallable, Category = "AITeammate|TeammateCharacterAgentComponent")
    void SetUseConditionsAndEffects(bool value);

  public:
    // Live query to the possessed pawn's descriptor via the cached pointer. Returns empty if no pawn,
    // no descriptor, or unpossessed.
    UFUNCTION(BlueprintCallable, Category = "AITeammate|TeammateCharacterAgentComponent")
    FString GetCharacterPersonality() const;

    // Live query to the possessed pawn's descriptor via the cached pointer. Returns empty if no pawn,
    // no descriptor, or unpossessed.
    UFUNCTION(BlueprintCallable, Category = "AITeammate|TeammateCharacterAgentComponent")
    FString GetCharacterName() const;

    // Live query to the possessed pawn's descriptor via the cached pointer. Returns empty if no pawn,
    // no descriptor, or unpossessed. The squad this teammate was registered under -- the key it sits
    // at in UTeammateRegistry -- may differ; ask GetCharacterSquad() on the descriptor for what the
    // next RegisterTeammate() will use.
    UFUNCTION(BlueprintCallable, Category = "AITeammate|TeammateCharacterAgentComponent")
    FString GetCharacterSquad() const;

    // The live, validated action set: the descriptor's authored actions as re-validated by RegisterTeammate(),
    // plus/minus whatever RegisterActions/UnRegisterActions has done at runtime.
    //
    // Returns a copy rather than a const reference to match the by-value style of the identity getters.
    UFUNCTION(BlueprintCallable, Category = "AITeammate|TeammateCharacterAgentComponent")
    TArray<FString> GetAvailableActions() const;

    TSharedPtr<FJsonObject> GetJSON() const;

    static TArray<TSharedPtr<FJsonValue>> GetJSON(const TArray<UTeammateCharacterAgentComponent *> &Teammates);

  public:
    UPROPERTY()
    TObjectPtr<UAIPerceptionComponent> PerceptionComponent;

    UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "AITeammate|ActionPlanEngine")
    bool InterruptLogic = false;
    UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "AITeammate|ActionPlanEngine")
    bool UseConditionsAndEffects = false;

  protected:
    TUniquePtr<FActionPlanEngine> ActionPlanEngine;
};
