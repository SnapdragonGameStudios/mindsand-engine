// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "Core/AITeammateOrchestrator.h"
#include "Actions/ActionPlanEngine.h"
#include "Actions/GameplayTask_Action.h"
#include "Core/LLMPlanner.h"
#include "MindSandLog.h"
#include "Teammates/LeaderTeammateCharacterAgentComponent.h"
#include "Teammates/TeammateCharacterAgentComponent.h"
#include "Teammates/TeammateRegistry.h"
#include <GameplayTagsManager.h>

void UAITeammateOrchestrator::Deinitialize()
{
    if (LLMPlanner != nullptr)
    {
        LLMPlanner->Deinitialize();
        LLMPlanner = nullptr;
    }
    Super::Deinitialize();
}

void UAITeammateOrchestrator::Initialize(FSubsystemCollectionBase &Collection)
{
    Super::Initialize(Collection);

    LLMPlanner = NewObject<ULLMPlanner>(this);
    TWeakObjectPtr<UAITeammateOrchestrator> WeakThis(this);
    LLMPlanner->SetCallbacks(
        [WeakThis](FQueryId QueryId, const FString &TeammateId, TArray<FParsedAction> &&Actions) -> bool {
            if (UAITeammateOrchestrator *Orchestrator = WeakThis.Get())
            {
                return Orchestrator->DispatchActions(QueryId, TeammateId, MoveTemp(Actions));
            }
            return false;
        },
        [WeakThis](FQueryId QueryId, const FString &TeammateId, bool bSuccess) {
            if (UAITeammateOrchestrator *Orchestrator = WeakThis.Get())
            {
                UE_LOG(LogMindSand, Log, TEXT("[Query %llu] Plan finished for teammate %s (success: %s)."),
                       static_cast<unsigned long long>(QueryId), *TeammateId, bSuccess ? TEXT("true") : TEXT("false"));
                UTeammateRegistry *Registry = UTeammateRegistry::Get(Orchestrator->GetWorld());
                UTeammateCharacterAgentComponent *Agent = Registry ? Registry->GetTeammate(TeammateId) : nullptr;
                FActionPlanEngine *Engine = Agent ? Agent->GetActionPlanEngine() : nullptr;
                if (Engine != nullptr && !Engine->IsPlanClosed())
                {
                    Engine->ClosePlan();
                }
            }
        });
    LLMPlanner->Initialize();
}

bool UAITeammateOrchestrator::DispatchActions(uint64 QueryId, const FString &TeammateId, TArray<FParsedAction> Actions)
{
    UTeammateRegistry *Registry = UTeammateRegistry::Get(GetWorld());
    UTeammateCharacterAgentComponent *Agent = Registry ? Registry->GetTeammate(TeammateId) : nullptr;
    if (Agent == nullptr)
    {
        UE_LOG(LogMindSand, Warning, TEXT("[Query %llu] Ignoring actions for unknown teammate %s."),
               static_cast<unsigned long long>(QueryId), *TeammateId);
        return false;
    }

    FActionPlanEngine *Engine = Agent->GetActionPlanEngine();
    if (Engine == nullptr)
    {
        UE_LOG(LogMindSand, Warning, TEXT("[Query %llu] Teammate %s has no action plan engine."),
               static_cast<unsigned long long>(QueryId), *TeammateId);
        return false;
    }

    UE_LOG(LogMindSand, Log, TEXT("[Query %llu] Dispatching %d action(s) to teammate %s."),
           static_cast<unsigned long long>(QueryId), Actions.Num(), *TeammateId);

    if (!Engine->EnqueueActions(QueryId, Actions))
    {
        UE_LOG(LogMindSand, Warning, TEXT("[Query %llu] Failed to enqueue actions for teammate %s."),
               static_cast<unsigned long long>(QueryId), *TeammateId);
        return false;
    }

    return true;
}

bool UAITeammateOrchestrator::InjectResponse(const FString &Response)
{
    return LLMPlanner != nullptr && LLMPlanner->InjectResponse(Response);
}

void UAITeammateOrchestrator::SendTeamLeaderCommandGoal(const FString &command,
                                                        const ULeaderTeammateCharacterAgentComponent *Leader)
{
    if (LLMPlanner != nullptr && Leader != nullptr)
    {
        LLMPlanner->RequestPlan(command, Leader->TeamName);
    }
}

void UAITeammateOrchestrator::SendEvent(FGameplayTag tag, const FString payload)
{
    FString TagString = TEXT("Tactix.Event");
    FGameplayTag RootTag = UGameplayTagsManager::Get().RequestGameplayTag(FName(*TagString), true);
    if (RootTag.IsValid())
    {
        if (tag.MatchesTag(RootTag))
        {
            SendEvent(tag.GetTagLeafName().ToString(), payload);
        }
    }
}

void UAITeammateOrchestrator::SendEvent(const FString &eventName, const FString &payload)
{
    AsyncTask(ENamedThreads::GameThread, [eventName, payload, this]() {
        TArray<UTeammateCharacterAgentComponent *> teammates = UTeammateRegistry::Get(GetWorld())->GetAllTeammates();

        // send the event to all teammates
        for (auto &agent : teammates)
        {
            agent->GetActionPlanEngine()->SendEvent(eventName, payload);
        }
    });
}

void UAITeammateOrchestrator::TestAction(const FString &Teammate, const FString &ActionName, const FString &parameters)
{
    FParsedAction action;
    action.bSignalEndTaskToTeammates = false;
    action.Name = ActionName;
    action.ParametersJson = parameters;

    FParsedPlan plan;
    plan.Acknowledgment = "";
    plan.Actions.Add(action);
    plan.TeammateId = Teammate;

    UTeammateRegistry *Registry = UTeammateRegistry::Get(GetWorld());
    UTeammateCharacterAgentComponent *agent = nullptr;
    if (Registry)
    {
        agent = Registry->GetTeammateByName(plan.TeammateId);
    }

    if (agent && agent->GetActionPlanEngine() != nullptr)
    {
        FActionPlanEngine *Engine = agent->GetActionPlanEngine();
        if (Engine->EnqueueActions(/*QueryId=*/0, plan.Actions))
        {
            Engine->ClosePlan();
        }
    }
    else
    {
        UE_LOG(LogMindSand, Error, TEXT("Teammate %s not found"), *Teammate);
    }
}
