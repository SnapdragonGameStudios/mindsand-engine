// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "ActionDescriptionAsset.h"
#include "Core/MindSandTypes.h"
#include "CoreMinimal.h"
#include "GameplayTask.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptInterface.h"
#include <functional>

#include "GameplayTask_Action.generated.h"

class FActionPlanEngine;

UENUM(BlueprintType)
enum class EActionResult : uint8
{
    ActionFail UMETA(DisplayName = "ActionFail"),
    ActionSucess UMETA(DisplayName = "ActionSucess"),
    ActionAbort UMETA(DisplayName = "ActionAbort")
};

struct MINDSAND_API FActionResult
{
    FActionResult() : Result(EActionResult::ActionAbort)
    {
    }

    explicit FActionResult(EActionResult InResult, FString InReason = FString())
        : Result(InResult), Reason(MoveTemp(InReason))
    {
    }

    EActionResult Result;
    FString Reason;
};

/**
 *
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTaskActionDelegate, bool, Success);

typedef std::function<void(const FActionResult &)> OnActionFinish;

UCLASS(Abstract, BlueprintType)
class MINDSAND_API UGameplayTask_Action : public UGameplayTask
{
    GENERATED_BODY()

  public:
    UGameplayTask_Action(const FObjectInitializer &ObjectInitializer);

    UPROPERTY(BlueprintAssignable)
    FTaskActionDelegate OnFinish;

    OnActionFinish OnActionFinished;

    void ActivateAction(IGameplayTaskOwnerInterface &InTaskOwner, const FString &JsonParams,
                        const uint8 priority = FGameplayTasks::DefaultPriority);

    void SetDataFromAsset(UActionDescriptionAsset *asset);

    void SetDispatchEndEventToPlannerEnabled(bool value)
    {
        DispatchEndActionEventToPlanner = value;
    }

    bool IsConditionsValidationEnabled()
    {
        return bConditionsValidationEnabled;
    }

    bool VerifyConditions();

    void SetEffects();

    void EndAction(bool result);

    void Abort(const FString &reason);

    virtual void OnEvent(const FString &eventName, const FString &payload);

    void OnGameplayTaskActivated(UGameplayTask &Task) override;

    UFUNCTION(BlueprintCallable)
    void SetTargetedActorForActions(AActor *actor);

    UFUNCTION(BlueprintCallable)
    AActor *GetTargetedActorForActions();

  protected:
    virtual void ProcessParameters(const FString &JsonParams);

    bool BroadcastEndActionEvent();

    virtual void OnActionEnd() {};

  public:
    TMap<FString, bool> OutEffects;
    TMap<FString, bool> Conditions;
    float Cost = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FActionParameters> ActionParameters;

    // Declared by the action asset but never reflected onto a UPROPERTY - a ProcessParameters override
    // extracts these from the raw JSON itself. Exposed so an override can see what was declared.
    UPROPERTY(BlueprintReadOnly)
    TArray<FActionParameters> AdditionalParameters;

    bool DispatchEndActionEventToPlanner = false;

  protected:
    TMap<FString, ParameterValue> Parameters;

    bool bConditionsValidationEnabled = false;
    FActionPlanEngine *Planner = nullptr;
    bool bMeetConditions = true;
};
