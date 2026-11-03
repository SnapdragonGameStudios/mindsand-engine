// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "PromptContext/PromptContextGatherer.h"

#include "AIController.h"
#include "Actions/ActionDescriptionAsset.h"
#include "Actions/ActionRegistrySubsystem.h"
#include "ActorRegistrySubsystem.h"
#include "Core/MindSandConstants.h"
#include "Core/MindSandJsonKeys.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Json.h"
#include "MindSandSettings.h"
#include "Perception/AIPerceptionComponent.h"
#include "StateGatherer/PerceptionStateGatherer.h"
#include "Teammates/TeammateCharacterAgentComponent.h"
#include "Teammates/TeammateRegistry.h"
#include <GameplayTagsManager.h>

namespace
{
TSharedRef<FJsonObject> GetPerceptionsObject(UWorld *World, const FString &TeamName)
{
    if (!World)
    {
        return MakeShared<FJsonObject>();
    }

    TSet<FString> CollectedTypes;
    TMap<FString, FString> AllActorStates;
    TArray<TPair<FString, UAIPerceptionComponent *>> AllPerceptionComps;

    APlayerController *PlayerController = World->GetFirstPlayerController();
    if (PlayerController)
    {
        UAIPerceptionComponent *PlayerPerception = PlayerController->GetComponentByClass<UAIPerceptionComponent>();
        if (PlayerPerception)
        {
            AllPerceptionComps.Add(TPair<FString, UAIPerceptionComponent *>(TEXT("player"), PlayerPerception));
        }
    }

    UTeammateRegistry *TeammateRegistry = UTeammateRegistry::Get(World);
    if (TeammateRegistry)
    {
        TArray<UTeammateCharacterAgentComponent *> TeamTeammates = TeammateRegistry->GetTeammatesForSquad(TeamName);
        for (UTeammateCharacterAgentComponent *Teammate : TeamTeammates)
        {
            if (!Teammate || !Teammate->GetOwner())
            {
                continue;
            }

            AController *Controller = Cast<AController>(Teammate->GetOwner());
            if (Controller)
            {
                UAIPerceptionComponent *TeammatePerception = Controller->GetComponentByClass<UAIPerceptionComponent>();
                if (TeammatePerception)
                {
                    FString TeammateName =
                        Teammate->GetCharacterName().IsEmpty() ? Controller->GetName() : Teammate->GetCharacterName();
                    AllPerceptionComps.Add(TPair<FString, UAIPerceptionComponent *>(TeammateName, TeammatePerception));
                }
            }
        }
    }

    TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> KnownActors;

    const UMindSandSettings *Settings = GetDefault<UMindSandSettings>();
    if (Settings && Settings->bUseUnifiedPerception)
    {
        TSet<FString> UnifiedPerceptions;
        for (const auto &Pair : AllPerceptionComps)
        {
            TMap<FString, FString> ActorStates;
            TArray<FString> ActorIDs =
                UPerceptionStateGatherer::GetKnownActorsArray(Pair.Value, CollectedTypes, ActorStates);

            for (const FString &ActorID : ActorIDs)
            {
                UnifiedPerceptions.Add(ActorID);
            }

            for (const TPair<FString, FString> &StatePair : ActorStates)
            {
                AllActorStates.Add(StatePair.Key, StatePair.Value);
            }
        }

        for (const FString &ActorID : UnifiedPerceptions)
        {
            KnownActors.Add(MakeShared<FJsonValueString>(ActorID));
        }
    }
    else
    {
        for (const auto &Pair : AllPerceptionComps)
        {
            TMap<FString, FString> ActorStates;
            TArray<FString> ActorIDs =
                UPerceptionStateGatherer::GetKnownActorsArray(Pair.Value, CollectedTypes, ActorStates);

            TArray<TSharedPtr<FJsonValue>> ActorsJson;
            for (const FString &ActorID : ActorIDs)
            {
                ActorsJson.Add(MakeShared<FJsonValueString>(ActorID));
            }

            TSharedRef<FJsonObject> TeammateArrayJson = MakeShared<FJsonObject>();
            TeammateArrayJson->SetArrayField(*Pair.Key, ActorsJson);
            KnownActors.Add(MakeShared<FJsonValueObject>(TeammateArrayJson));

            for (const TPair<FString, FString> &StatePair : ActorStates)
            {
                AllActorStates.Add(StatePair.Key, StatePair.Value);
            }
        }
    }

    TSharedRef<FJsonObject> ObjectStatesObj = MakeShared<FJsonObject>();
    for (const TPair<FString, FString> &StatePair : AllActorStates)
    {
        ObjectStatesObj->SetStringField(*StatePair.Key, *StatePair.Value);
    }

    TSharedRef<FJsonObject> DescriptionsObj = MakeShared<FJsonObject>();
    UActorRegistrySubsystem *ActorRegistry = World->GetSubsystem<UActorRegistrySubsystem>();
    if (ActorRegistry)
    {
        for (const FString &TypeID : CollectedTypes)
        {
            FString Description = ActorRegistry->GetDescriptionForTag(TypeID);
            if (!Description.IsEmpty())
            {
                DescriptionsObj->SetStringField(*TypeID, *Description);
            }
        }
    }

    Root->SetObjectField(MindSandJsonKeys::DynamicGameInfo::ActorDescriptions, DescriptionsObj);
    Root->SetArrayField(MindSandJsonKeys::DynamicGameInfo::KnownActors, KnownActors);
    Root->SetObjectField(MindSandJsonKeys::DynamicGameInfo::ObjectStates, ObjectStatesObj);
    return Root;
}

TSet<FString> GetUniqueActionsForSquad(UWorld *World, const FString &TeamName)
{
    TSet<FString> UniqueActions;
    UTeammateRegistry *Registry = UTeammateRegistry::Get(World);
    if (!Registry)
    {
        return UniqueActions;
    }

    for (UTeammateCharacterAgentComponent *Teammate : Registry->GetTeammatesForSquad(TeamName))
    {
        if (!Teammate)
        {
            continue;
        }
        for (const FString &Action : Teammate->GetAvailableActions())
        {
            UniqueActions.Add(Action);
        }
    }
    return UniqueActions;
}

void GetGlobalTeammatesEventsList(TArray<FString> &EventList)
{
    const FString TagString = TEXT("Tactix.Event");
    EventList.Add(MindSandConstants::TeammateActionFinishedEvent);

    FGameplayTag RootTag = UGameplayTagsManager::Get().RequestGameplayTag(FName(*TagString), false);
    if (RootTag.IsValid())
    {
        FGameplayTagContainer Container = UGameplayTagsManager::Get().RequestGameplayTagChildren(RootTag);
        TArray<FGameplayTag> List = Container.GetGameplayTagArray();
        for (FGameplayTag &Tag : List)
        {
            EventList.Add(Tag.GetTagLeafName().ToString());
        }
    }
}
} // namespace

TSharedRef<FJsonObject> FPromptContextGatherer::GetStaticGameInfo(UWorld *World, const FString &TeamName)
{
    if (World == nullptr || World->GetGameInstance() == nullptr)
    {
        return MakeShared<FJsonObject>();
    }

    UActionRegistrySubsystem *ActionRegistry = World->GetGameInstance()->GetSubsystem<UActionRegistrySubsystem>();

    TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetArrayField(
        MindSandJsonKeys::StaticGameInfo::ActionDescriptions,
        UActionDescriptionAsset::GetJSON(ActionRegistry, GetUniqueActionsForSquad(World, TeamName).Array()));
    Root->SetArrayField(
        MindSandJsonKeys::StaticGameInfo::Teammates,
        UTeammateCharacterAgentComponent::GetJSON(UTeammateRegistry::Get(World)->GetTeammatesForSquad(TeamName)));

    TArray<FString> EventsList;
    GetGlobalTeammatesEventsList(EventsList);

    TArray<TSharedPtr<FJsonValue>> GlobalEvents;
    for (auto &EventName : EventsList)
    {
        GlobalEvents.Add(MakeShared<FJsonValueString>(EventName));
    }

    Root->SetArrayField(MindSandJsonKeys::StaticGameInfo::GlobalEvents, GlobalEvents);
    return Root;
}

TSharedRef<FJsonObject> FPromptContextGatherer::GetDynamicGameInfo(UWorld *World, const FString &TeamName)
{
    return GetPerceptionsObject(World, TeamName);
}
