// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "Actions/ActionPlanEngine.h"
#include "AIController.h"
#include "Actions/ActionRegistrySubsystem.h"
#include "Actions/GameplayTask_Action.h"
#include "Actions/Parsers/IActionPlanParser.h"
#include "BrainComponent.h"
#include "GameplayTask.h"
#include "Kismet/GameplayStatics.h"
#include "MindSandLog.h"

FActionPlanEngine::FActionPlanEngine(AActor *owner)
{
    Owner = owner;
}

FActionPlanEngine::~FActionPlanEngine()
{
    // Detach the running task first. It holds a lambda capturing `this` (see StartNextAction), so leaving it
    // attached lets its next EndAction/Abort call into freed memory - the owning component's EndPlay resets this
    // engine while gameplay tasks on the AIController are still live.
    //
    // Do not restore behavior logic while the owner is being torn down.
    CancelPlanInternal(/*bRestoreBehaviorLogic=*/false);
}

void FActionPlanEngine::RegisterPlanStateVariable(const FString &variableName)
{
    if (!PlanStateVariables.Contains(variableName))
    {
        PlanStateVariables.Add(variableName) = false;
    }
}

bool FActionPlanEngine::UpdatePlanStateVariable(const FString &variableName, bool value)
{
    if (PlanStateVariables.Contains(variableName))
    {
        PlanStateVariables.Add(variableName) = value;
        return true;
    }
    return false;
}

bool FActionPlanEngine::GetPlanStateVariableValue(const FString &variableName, bool &value)
{
    if (PlanStateVariables.Contains(variableName))
    {
        value = PlanStateVariables[variableName];
        return true;
    }
    return false;
}

void FActionPlanEngine::ResetPlanStateVariables()
{
    for (auto &pair : PlanStateVariables)
    {
        pair.Value = false;
    }
}

void FActionPlanEngine::ClearPlanStateVariables()
{
    PlanStateVariables.Empty();
}

void FActionPlanEngine::RegisterPlanStateVariables(const UActionDescriptionAsset *ActionAsset)
{
    if (ActionAsset != nullptr)
    {
        for (auto &effect : ActionAsset->Conditions)
        {
            RegisterPlanStateVariable(effect.Key);
        }
        for (auto &effect : ActionAsset->OutEffects)
        {
            RegisterPlanStateVariable(effect.Key);
        }
    }
}

void FActionPlanEngine::ReadState(TMap<FString, bool> &Result)
{
    for (auto &item : PlanStateVariables)
    {
        Result.Add(item.Key, item.Value);
    }
}

void FActionPlanEngine::StopBehaviorLogic()
{
    // Already stopped by this engine: do not stop twice, and above all do not clear the flag - it is the only
    // record that the tree still needs restarting.
    if (bLogicInterrupted || !InterruptLogic || AIController == nullptr)
    {
        return;
    }

    UE_LOG(LogMindSand, Log, TEXT("[ActionPlan] Stopping Behavior Logic"));

    // Stop the current behavior tree.
    UBrainComponent *bt = AIController->GetBrainComponent();

    // IsRunning() only tells us whether the tree is currently running; it cannot tell us whether this engine should
    // restart it later. Do not take ownership of a tree already stopped by something else, such as a stunned or
    // dead pawn or a cutscene, because finishing this plan must not revive logic that another system disabled.
    if (bt != nullptr && bt->IsRunning())
    {
        bt->StopLogic("Plan Started");
        bLogicInterrupted = true;
    }
}

void FActionPlanEngine::RestoreBehaviorLogic()
{
    // Deliberately does not consult InterruptLogic: the flag alone decides, so that flipping InterruptLogic
    // mid-plan cannot strand a stopped tree. If the brain component is unreachable the flag stays set rather
    // than being cleared for a restart that never happened.
    if (!bLogicInterrupted || AIController == nullptr)
    {
        return;
    }

    UE_LOG(LogMindSand, Log, TEXT("[ActionPlan] Restoring Behavior Logic"));

    UBrainComponent *bt = AIController->GetBrainComponent();
    if (bt)
    {
        bt->StartLogic();
        bLogicInterrupted = false;
    }
}

void FActionPlanEngine::FinalizeActionPlan(const FActionResult &Result)
{
    UE_LOG(LogMindSand, Log, TEXT("[ActionPlan][%s] Finished with result %d: %s."), *GetNameSafe(Owner),
           static_cast<int32>(Result.Result), *Result.Reason);
    RestoreBehaviorLogic();

    // Clear the running-plan state BEFORE notifying. A completion handler may enqueue a replacement plan
    // re-entrantly; resetting state after the callback would clobber that plan's queued and running action state.
    RunningTask.Reset();
    PendingActions.Empty();
    ActionPlanState =
        Result.Result == EActionResult::ActionSucess ? EActionPlanState::Inactive : EActionPlanState::Closed;

    if (OnPlanFinished)
    {
        OnPlanFinished(ActiveQueryId, Result);
    }
}

void FActionPlanEngine::ProcessActionResult(const FActionResult &Result)
{
    UE_LOG(LogMindSand, Log, TEXT("[ActionPlan][%s] Action completed with result %d: %s."), *GetNameSafe(Owner),
           static_cast<int32>(Result.Result), *Result.Reason);
    UnregisterAsEventListener(RunningTask.Get());
    RunningTask.Reset();

    // Anything but success ends the plan. There is no retry: the plan is an ordered sequence whose later actions
    // assume the earlier ones happened, so there is no correct place to resume from after a failure.
    const bool bStopPlan = Result.Result != EActionResult::ActionSucess;

    if (bStopPlan)
    {
        FinalizeActionPlan(Result);
        return;
    }
    StartNextAction();
}

static UGameplayTask_Action *SpawnAction(const FParsedAction &Action, UObject *Owner)
{
    TObjectPtr<UGameplayTask_Action> GP_Action = nullptr;
    UActionRegistrySubsystem *actionRegistry =
        UGameplayStatics::GetGameInstance(Owner)->GetSubsystem<UActionRegistrySubsystem>();
    if (actionRegistry != nullptr)
    {
        UActionDescriptionAsset *asset = actionRegistry->FindActionByName(Action.Name);

        if (asset)
        {
            GP_Action = TObjectPtr<UGameplayTask_Action>(asset->CreateActionGameplayTask(Owner));
            if (GP_Action.Get() == nullptr)
            {
                ensureMsgf(false, TEXT("Asset has a null action implementation, plan will be aborted"));
            }
            else
            {
                GP_Action->SetDataFromAsset(asset);
                GP_Action->SetDispatchEndEventToPlannerEnabled(Action.bSignalEndTaskToTeammates);
            }
        }
    }

    return GP_Action;
}

void FActionPlanEngine::StartNextAction()
{
    if (RunningTask.IsValid())
    {
        return;
    }

    FParsedAction NewAction;
    if (!PendingActions.Dequeue(NewAction))
    {
        if (ActionPlanState == EActionPlanState::Closed)
        {
            FinalizeActionPlan(FActionResult(EActionResult::ActionSucess));
        }
        else
        {
            UE_LOG(LogMindSand, Log, TEXT("[ActionPlan][%s] Waiting for more streamed actions."), *GetNameSafe(Owner));
        }
        return;
    }

    UE_LOG(LogMindSand, Log, TEXT("[ActionPlan][%s] Starting action: %s."), *GetNameSafe(Owner), *NewAction.Name);
    RunningTask = SpawnAction(NewAction, Owner);
    if (!RunningTask.IsValid())
    {
        const FString FailedActionName = NewAction.Name;
        UE_LOG(LogMindSand, Error, TEXT("FActionPlanEngine: could not spawn action '%s' for %s; aborting plan."),
               *FailedActionName, *GetNameSafe(Owner));

        FinalizeActionPlan(FActionResult(EActionResult::ActionAbort,
                                         FString::Printf(TEXT("Could not spawn action '%s'"), *FailedActionName)));
        return;
    }

    RunningTask->OnActionFinished = [this](const FActionResult &action_result) { ProcessActionResult(action_result); };
    RegisterAsEventListener(RunningTask.Get());
    RunningTask->ActivateAction(*Cast<IGameplayTaskOwnerInterface>(Owner), NewAction.ParametersJson);
}

bool FActionPlanEngine::StartPlan(uint64 QueryId)
{
    AIController = Cast<AAIController>(Owner);
    if (AIController == nullptr || !Owner->Implements<UGameplayTaskOwnerInterface>())
    {
        UE_LOG(LogMindSand, Error, TEXT("[ActionPlan][%s] Cannot open plan: invalid gameplay-task owner."),
               *GetNameSafe(Owner));
        return false;
    }

    const bool bHasPlanToReplace = ActionPlanState != EActionPlanState::Inactive;
    const uint64 PreviousQueryId = ActiveQueryId;

    if (bHasPlanToReplace)
    {
        UE_LOG(LogMindSand, Warning,
               TEXT("[ActionPlan][%s] Replacing active action sequence (active query: %llu, incoming query: %llu)."),
               *GetNameSafe(Owner), static_cast<unsigned long long>(PreviousQueryId),
               static_cast<unsigned long long>(QueryId));
    }
    CancelPlanInternal(/*bRestoreBehaviorLogic=*/false);
    ActiveQueryId = QueryId;
    ActionPlanState = EActionPlanState::Open;
    StopBehaviorLogic();

    if (bHasPlanToReplace && OnPlanFinished)
    {
        OnPlanFinished(PreviousQueryId,
                       FActionResult(EActionResult::ActionAbort, TEXT("Plan replaced by a new query")));
    }
    return true;
}

bool FActionPlanEngine::EnqueueActions(uint64 QueryId, const TArray<FParsedAction> &Actions)
{
    // A new query owns a new plan for this teammate. Starting here keeps plan lifecycle policy inside the
    // engine and prevents callers from having to coordinate StartPlan with the first streamed action batch.
    if (ActionPlanState == EActionPlanState::Inactive || ActiveQueryId != QueryId)
    {
        if (!StartPlan(QueryId))
        {
            return false;
        }
    }

    if (ActionPlanState != EActionPlanState::Open)
    {
        UE_LOG(LogMindSand, Error, TEXT("[ActionPlan][%s] Cannot append actions for query %llu: sequence is not open."),
               *GetNameSafe(Owner), static_cast<unsigned long long>(QueryId));
        return false;
    }

    for (const FParsedAction &Action : Actions)
    {
        PendingActions.Enqueue(Action);
    }
    StartNextAction();
    return true;
}

bool FActionPlanEngine::ClosePlan()
{
    if (ActionPlanState != EActionPlanState::Open)
    {
        UE_LOG(LogMindSand, Error, TEXT("[ActionPlan][%s] Cannot close actions: sequence is not open."),
               *GetNameSafe(Owner));
        return false;
    }

    ActionPlanState = EActionPlanState::Closed;
    StartNextAction();
    return true;
}

void FActionPlanEngine::CancelPlan()
{
    // Cancellation is terminal for the current plan, so restore behavior logic and return control to the owner.
    const bool bHasPlanToCancel = ActionPlanState != EActionPlanState::Inactive;
    const uint64 CancelledQueryId = ActiveQueryId;
    CancelPlanInternal(/*bRestoreBehaviorLogic=*/true);

    if (bHasPlanToCancel && OnPlanFinished)
    {
        OnPlanFinished(CancelledQueryId, FActionResult(EActionResult::ActionAbort, TEXT("Plan cancelled")));
    }
}

bool FActionPlanEngine::IsPlanClosed() const
{
    return ActionPlanState == EActionPlanState::Closed;
}

void FActionPlanEngine::CancelPlanInternal(bool bRestoreBehaviorLogic)
{
    if (RunningTask.IsValid() || !PendingActions.IsEmpty())
    {
        UE_LOG(LogMindSand, Warning, TEXT("[ActionPlan][%s] Cancelling active plan (restore logic: %s)."),
               *GetNameSafe(Owner), bRestoreBehaviorLogic ? TEXT("true") : TEXT("false"));
    }

    UGameplayTask_Action *PendingTaskToFinish = nullptr;
    if (RunningTask.IsValid())
    {
        PendingTaskToFinish = RunningTask.Get();
    }
    RunningTask.Reset();
    ResetTargetedActorForActions();
    EventListeners.Empty();
    PendingActions.Empty();
    ActionPlanState = EActionPlanState::Inactive;

    if (PendingTaskToFinish != nullptr)
    {
        // Clear the callback BEFORE aborting. Abort invokes OnActionFinished, which would re-enter this engine -
        // and on the destructor path, re-enter one that is already half destroyed.
        PendingTaskToFinish->OnActionFinished = nullptr;
        if (PendingTaskToFinish->IsActive())
        {
            PendingTaskToFinish->Abort(TEXT("Plan cancelled"));
        }
    }

    // Last, so the tree does not start making decisions while the cancelled action is still unwinding.
    if (bRestoreBehaviorLogic)
    {
        RestoreBehaviorLogic();
    }
}

void FActionPlanEngine::SetTargetedActorForActions(AActor *actor)
{
    TargetedActor = actor;
}

void FActionPlanEngine::ResetTargetedActorForActions()
{
    TargetedActor = nullptr;
}

AActor *FActionPlanEngine::GetTargetedActorForActions()
{
    return TargetedActor.Get();
}

void FActionPlanEngine::RegisterAsEventListener(UGameplayTask_Action *action)
{
    EventListeners.Add(action);
}

void FActionPlanEngine::UnregisterAsEventListener(UGameplayTask_Action *action)
{
    EventListeners.Remove(action);
}

void FActionPlanEngine::SendEvent(const FString &tag, const FString &payload)
{
    for (auto &listener : EventListeners)
    {
        if (listener.IsValid())
        {
            listener.Pin()->OnEvent(tag, payload);
        }
    }
}
