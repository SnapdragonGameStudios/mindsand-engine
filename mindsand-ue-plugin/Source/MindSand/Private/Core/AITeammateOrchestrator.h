// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
// teammate Actions
#include "Actions/ActionPlanTypes.h"
#include "Actions/GameplayTask_Action.h"
#include "Core/MindSandTypes.h"
#include "GameplayTagContainer.h"

#include "AITeammateOrchestrator.generated.h"

class UTeammateCharacterAgentComponent;
class ULeaderTeammateCharacterAgentComponent;
class UDEPRECATED_ActionPlannerComponent;
class ULLMPlanner;

/**
 *
 */
UCLASS(BlueprintType, Config = Game)
class MINDSAND_API UAITeammateOrchestrator : public UGameInstanceSubsystem
{
    GENERATED_BODY()

    friend class ULeaderTeammateCharacterAgentComponent;

  public:
    /** Initializes the LLM integration and token processor. */
    void Initialize(FSubsystemCollectionBase &Collection) override;

    /** Stops token processing and releases orchestrator runtime state. */
    void Deinitialize() override;

    /** Broadcasts a gameplay event identified by tag to the teammate system. */
    UFUNCTION(BlueprintCallable, Category = "MindSand|Orchestrator")
    void SendEvent(FGameplayTag tag, const FString payload);

    /** Parses and dispatches a complete externally supplied LLM response. */
    UFUNCTION(BlueprintCallable, Category = "MindSand|Orchestrator")
    bool InjectResponse(const FString &Response);

    /** Broadcasts a gameplay event identified by name to the teammate system. */
    void SendEvent(const FString &eventName, const FString &payload);

  private:
    bool DispatchActions(uint64 QueryId, const FString &TeammateId, TArray<FParsedAction> Actions);

  private:
    void SendTeamLeaderCommandGoal(const FString &command, const ULeaderTeammateCharacterAgentComponent *Leader);

    UPROPERTY(Transient)
    TObjectPtr<ULLMPlanner> LLMPlanner;

  public:
    /** Test path for directly dispatching one action to a teammate. */
    UFUNCTION(BlueprintCallable, Category = "MindSand|Orchestrator")
    void TestAction(const FString &Teammate, const FString &ActionName, const FString &parameters);
};
