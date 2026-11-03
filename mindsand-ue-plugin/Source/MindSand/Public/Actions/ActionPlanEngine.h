// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "Actions/ActionPlanTypes.h"
#include "Actions/GameplayTask_Action.h"
#include "Components/ActorComponent.h"
#include "Containers/Queue.h"
#include "Core/MindSandTypes.h"
#include "CoreMinimal.h"
#include <functional>

class AAIController;

using FOnPlanFinished = std::function<void(uint64 QueryId, const FActionResult &)>;

class MINDSAND_API FActionPlanEngine
{

  public:
    /** Creates an action-plan engine owned by the specified actor. */
    FActionPlanEngine(AActor *owner);

    /** Destructor */
    ~FActionPlanEngine();

  protected:
    void RegisterPlanStateVariable(const FString &variableName);
    void ClearPlanStateVariables();

  public:
    /**
     * Appends actions to the plan identified by QueryId and starts execution when possible.
     * The first enqueue, or an enqueue for a new query, implicitly activates a plan and replaces
     * any previous plan for this owner.
     *
     * @return true when the actions were accepted; false when the owner is invalid or the plan is closed.
     */
    bool EnqueueActions(uint64 QueryId, const TArray<FParsedAction> &Actions);

    /** Closes the plan to further appends and allows queued actions to finish.
     *  @return true when an open plan was closed; false when no plan can be closed.
     */
    bool ClosePlan();

    /** Aborts the active action, clears queued actions, and restores behavior logic interrupted by this plan.
     */
    void CancelPlan();

    /** @return true when no more actions may be appended to the current plan. */
    bool IsPlanClosed() const;

    /** Sets the actor that action implementations should treat as the current target. */
    void SetTargetedActorForActions(AActor *actor);

    /** Clears the actor currently targeted by action implementations. */
    void ResetTargetedActorForActions();

    /** @return the actor currently targeted by action implementations, or nullptr. */
    AActor *GetTargetedActorForActions();

    /** Registers an action to receive events sent through SendEvent. */
    void RegisterAsEventListener(UGameplayTask_Action *action);

    /** Stops an action from receiving events sent through SendEvent. */
    void UnregisterAsEventListener(UGameplayTask_Action *action);

    /** Sends an event to all currently registered action listeners. */
    void SendEvent(const FString &tag, const FString &payload);

    /** @return whether action conditions and effects are enabled for this engine. */
    bool GetUseConditionsAndEffects()
    {
        return bUseConditionsAndEffects;
    }

    /** Updates a registered plan-state variable.
     *  @return true when the variable exists and was updated; false otherwise.
     */
    bool UpdatePlanStateVariable(const FString &variableName, bool value);

    /** Reads a registered plan-state variable into value.
     *  @return true when the variable exists; false otherwise.
     */
    bool GetPlanStateVariableValue(const FString &variableName, bool &value);

    /** Sets all registered plan-state variables to false. */
    void ResetPlanStateVariables();

    /** Registers state variables referenced by an action asset's conditions and effects. */
    void RegisterPlanStateVariables(const UActionDescriptionAsset *ActionAsset);

    /** Copies the current plan-state variables into Result. */
    void ReadState(TMap<FString, bool> &Result);

  protected:
    AActor *Owner;
    enum class EActionPlanState : uint8
    {
        // No plan is active; a new plan can be started.
        Inactive,

        // The plan accepts appended actions and may be executing or waiting for more actions.
        Open,

        // No more actions may be appended; queued/running actions may still finish. Failures also remain closed.
        Closed
    };

    TQueue<FParsedAction> PendingActions;
    TMap<FString, bool> PlanStateVariables;
    AAIController *AIController;
    TWeakObjectPtr<UGameplayTask_Action> RunningTask;
    EActionPlanState ActionPlanState = EActionPlanState::Inactive;
    uint64 ActiveQueryId = 0;

    // Whether this engine has stopped the owner's behavior tree and still owes it a StartLogic(). Mutated ONLY
    // by StopBehaviorLogic / RestoreBehaviorLogic, and only when the corresponding call actually happened -
    // clearing it anywhere else loses the record and strands a stopped tree. InterruptLogic cannot stand in for
    // it: SetInterruptLogic is BlueprintCallable and writes this engine directly, so a mid-plan flip would
    // otherwise leave the tree stopped forever or restart one this plan never touched.
    bool bLogicInterrupted = false;
    TArray<TWeakObjectPtr<UGameplayTask_Action>> EventListeners;

    TWeakObjectPtr<AActor> TargetedActor;

  public:
    bool InterruptLogic = true;
    bool bUseConditionsAndEffects = false;
    FOnPlanFinished OnPlanFinished;

  private:
    void ProcessActionResult(const FActionResult &Result);
    void StartNextAction();
    bool StartPlan(uint64 QueryId);

    // The single exit point for a plan, however it ended. Every path that stops executing actions must go
    // through here, or the behavior tree StartPlan stopped is never restarted and the owner goes inert.
    void FinalizeActionPlan(const FActionResult &Result);

    // The only two writers of bLogicInterrupted. Both are idempotent, so a repeated call is harmless.
    void StopBehaviorLogic();
    void RestoreBehaviorLogic();

    // Shared body of CancelPlan and the destructor. Detaches the running task's OnActionFinished before aborting
    // it, so a cancel can never re-enter this engine. bRestoreBehaviorLogic is false for the two callers that
    // must not hand control back: starting a replacement plan (which re-stops the tree immediately) and the destructor
    // (the owner is being torn down).
    void CancelPlanInternal(bool bRestoreBehaviorLogic);
};
