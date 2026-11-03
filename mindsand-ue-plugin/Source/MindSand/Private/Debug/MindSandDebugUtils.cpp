// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "Debug/MindSandDebugUtils.h"
#include "Core/AITeammateOrchestrator.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "MindSandLog.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "StateGatherer/PerceptionStateGatherer.h"
#include "Teammates/TeammateCharacterAgentComponent.h"
#include "Teammates/TeammateRegistry.h"

bool UMindSandDebugUtils::ExecutePlan(const UObject *WorldContextObject, const FString &ResponseJson)
{
    if (WorldContextObject == nullptr)
    {
        UE_LOG(LogMindSand, Warning, TEXT("ExecutePlan: WorldContextObject is null"));
        return false;
    }

    const UWorld *World = WorldContextObject->GetWorld();
    if (World == nullptr)
    {
        UE_LOG(LogMindSand, Warning, TEXT("ExecutePlan: no world from context object"));
        return false;
    }

    UGameInstance *GameInstance = World->GetGameInstance();
    if (GameInstance == nullptr)
    {
        UE_LOG(LogMindSand, Warning, TEXT("ExecutePlan: no GameInstance"));
        return false;
    }

    UAITeammateOrchestrator *Orchestrator = GameInstance->GetSubsystem<UAITeammateOrchestrator>();
    if (Orchestrator == nullptr)
    {
        UE_LOG(LogMindSand, Warning, TEXT("ExecutePlan: UAITeammateOrchestrator subsystem unavailable"));
        return false;
    }

    UpdatePerception(WorldContextObject);

    return Orchestrator->InjectResponse(ResponseJson);
}

void UMindSandDebugUtils::UpdatePerception(const UObject *WorldContextObject)
{
    if (WorldContextObject == nullptr)
    {
        UE_LOG(LogMindSand, Warning, TEXT("UpdatePerception: WorldContextObject is null"));
        return;
    }

    UTeammateRegistry *TeammateRegistry = UTeammateRegistry::Get(WorldContextObject);
    if (TeammateRegistry == nullptr)
    {
        UE_LOG(LogMindSand, Warning, TEXT("UpdatePerception: UTeammateRegistry unavailable"));
        return;
    }

    TSet<FString> CollectedTypes;
    for (UTeammateCharacterAgentComponent *Agent : TeammateRegistry->GetAllTeammates())
    {
        if (Agent == nullptr)
        {
            continue;
        }
        UAIPerceptionComponent *PerceptionComponent = Agent->PerceptionComponent;
        if (PerceptionComponent == nullptr)
        {
            continue;
        }
        UPerceptionStateGatherer::GetKnownActorsJSON(PerceptionComponent, CollectedTypes,
                                                     UAISense_Sight::StaticClass());
    }
}
