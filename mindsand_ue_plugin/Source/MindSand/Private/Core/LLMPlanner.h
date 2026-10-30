// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "Actions/ActionPlanTypes.h"
#include "Core/IActionPlanTokenProcessor.h"
#include "CoreMinimal.h"
#include "Framework/ConversationEngine.h"

#include "LLMPlanner.generated.h"

class ULeaderTeammateCharacterAgentComponent;

/** Owns LLM initialization, prompt construction, response processing, and query state. */
UCLASS()
class MINDSAND_API ULLMPlanner : public UObject
{
    GENERATED_BODY()

  public:
    using FOnActionsParsed = IActionPlanTokenProcessor::FOnActionsParsed;
    using FOnPlanFinished = IActionPlanTokenProcessor::FOnTokenPlanFinished;

    /** Initializes the LLM model, conversation engine integration, and action-plan token processor. */
    void Initialize();

    /** Stops token processing, clears planner callbacks, and resets query state. */
    void Deinitialize();

    /** Installs callbacks for parsed teammate actions and per-teammate plan completion. */
    void SetCallbacks(FOnActionsParsed InActionsCallback, FOnPlanFinished InPlanFinishedCallback);

    /** Processes a complete externally supplied LLM response through the action-plan parser. */
    bool InjectResponse(const FString &Response);

    /** Builds and sends a prompt for a team-leader command. */
    void RequestPlan(const FString &Command, const FString &TeamName);

  private:
    TUniquePtr<IActionPlanTokenProcessor> TokenProcessor;
    FOnActionsParsed ActionsCallback;
    FOnPlanFinished PlanFinishedCallback;

    bool bFirstQuery = true;

    void SendQuery(const FString &Text);
    uint64 BeginTokenProcessing();

    UPROPERTY(Transient)
    TObjectPtr<ULLMModel> LLMModel;

    UPROPERTY(Transient)
    TObjectPtr<UConversationEngine> ConversationEngine;

    FString CurrentQueryKey;
    FString CurrentResponse;

    UFUNCTION()
    void OnLLMInit(bool bSuccess);

    UFUNCTION()
    void OnConvEngineInit(bool bSuccess);

    UFUNCTION()
    void OnConvEngineToken(ELLMSentenceCode Code, FString Token);
};
