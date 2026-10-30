// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "PerceptionStateGatherer.h"

#include "Core/MindSandJsonKeys.h"
#include "Core/Utils.h"
#include "Dom/JsonObject.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

#include "ActorRegistrySubsystem.h"

namespace
{
constexpr bool bIncludeStimuliInfo = false;
}

TSharedPtr<FJsonObject> UPerceptionStateGatherer::GetKnownActorsJSON(UAIPerceptionComponent *PerceptionComp,
                                                                     TSet<FString> &OutCollectedTypes,
                                                                     TSubclassOf<UAISense> SenseToInclude)
{
    TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetArrayField(MindSandJsonKeys::DynamicGameInfo::KnownActors,
                        BuildKnownActorsArray(PerceptionComp, OutCollectedTypes, SenseToInclude));
    return Root;
}

TSharedPtr<FJsonObject> UPerceptionStateGatherer::GetKnownActorsJSON(
    const TArray<TPair<FString, UAIPerceptionComponent *>> &NamedComps, TSet<FString> &OutCollectedTypes,
    TSubclassOf<UAISense> SenseToInclude)
{
    TSharedPtr<FJsonObject> KnownActors = MakeShared<FJsonObject>();
    for (const TPair<FString, UAIPerceptionComponent *> &Pair : NamedComps)
    {
        KnownActors->SetArrayField(Pair.Key, BuildKnownActorsArray(Pair.Value, OutCollectedTypes, SenseToInclude));
    }

    TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetObjectField(MindSandJsonKeys::DynamicGameInfo::KnownActors, KnownActors);
    return Root;
}

FString UPerceptionStateGatherer::GetKnownActors(UAIPerceptionComponent *PerceptionComp,
                                                 TSet<FString> &OutCollectedTypes, TSubclassOf<UAISense> SenseToInclude)
{
    const TSharedPtr<FJsonObject> Root = GetKnownActorsJSON(PerceptionComp, OutCollectedTypes, SenseToInclude);

    return Utils::Serialize(Root.ToSharedRef());
}

FString UPerceptionStateGatherer::GetKnownActors(const TArray<TPair<FString, UAIPerceptionComponent *>> &NamedComps,
                                                 TSet<FString> &OutCollectedTypes, TSubclassOf<UAISense> SenseToInclude)
{
    const TSharedPtr<FJsonObject> Root = GetKnownActorsJSON(NamedComps, OutCollectedTypes, SenseToInclude);

    return Utils::Serialize(Root.ToSharedRef());
}

TArray<FString> UPerceptionStateGatherer::GetKnownActorsArray(UAIPerceptionComponent *PerceptionComp,
                                                              TSet<FString> &OutCollectedTypes,
                                                              TMap<FString, FString> &OutActorStates,
                                                              TSubclassOf<UAISense> SenseToInclude)
{
    if (!PerceptionComp)
    {
        return {};
    }

    TArray<AActor *> KnownActors;
    PerceptionComp->GetKnownPerceivedActors(SenseToInclude, KnownActors);

    TArray<FString> ActorIDs;
    for (AActor *Actor : KnownActors)
    {
        if (!Actor)
            continue;

        UWorld *WorldObj = PerceptionComp->GetOwner()->GetWorld();
        UActorRegistrySubsystem *ActorRegistry = WorldObj->GetSubsystem<UActorRegistrySubsystem>();

        FString UniqueID;
        FString TypeID;
        FString State;

        ActorRegistry->GetInfoFromActor(Actor, UniqueID, TypeID, State, ERegistryLookup::RegisterIfMissing);
        if (!UniqueID.IsEmpty())
        {
            OutCollectedTypes.Add(TypeID);
            ActorIDs.Add(UniqueID);
            if (!State.IsEmpty())
            {
                OutActorStates.Add(UniqueID, State);
            }
        }
    }

    return ActorIDs;
}

TSharedPtr<FJsonValue> UPerceptionStateGatherer::BuildKnownActorEntry(UAIPerceptionComponent *PerceptionComp,
                                                                      AActor *TargetActor,
                                                                      TSet<FString> &OutCollectedTypes)
{
    if (!PerceptionComp || !TargetActor)
        return nullptr;

    UWorld *WorldObj = PerceptionComp->GetOwner()->GetWorld();
    UActorRegistrySubsystem *ActorRegistry = WorldObj->GetSubsystem<UActorRegistrySubsystem>();

    FString UniqueID;
    FString TypeID;
    FString State;

    ActorRegistry->GetInfoFromActor(TargetActor, UniqueID, TypeID, State, ERegistryLookup::RegisterIfMissing);
    if (UniqueID.IsEmpty())
    {
        return nullptr;
    }

    // Add TypeID to the persistent type set (NOT to JSON)
    OutCollectedTypes.Add(TypeID);

    if constexpr (!bIncludeStimuliInfo)
    {
        TSharedPtr<FJsonObject> ActorObj = MakeShared<FJsonObject>();

        if (!State.IsEmpty())
        {
            ActorObj->SetStringField(MindSandJsonKeys::Perception::ActorId, UniqueID);
            ActorObj->SetStringField(MindSandJsonKeys::Perception::State, State);
            return MakeShared<FJsonValueObject>(ActorObj);
        }
        else
        {
            return MakeShared<FJsonValueString>(UniqueID);
        }
    }
    else
    {
        TSharedPtr<FJsonObject> ActorObj = MakeShared<FJsonObject>();
        ActorObj->SetStringField(MindSandJsonKeys::Perception::ActorId, UniqueID);
        if (!State.IsEmpty())
        {
            ActorObj->SetStringField(MindSandJsonKeys::Perception::State, State);
        }

        // Retrieve perception info
        FActorPerceptionBlueprintInfo Info;
        PerceptionComp->GetActorsPerception(TargetActor, Info);

        // Build stimuli array
        TArray<TSharedPtr<FJsonValue>> StimuliArray;

        for (const FAIStimulus &Stimulus : Info.LastSensedStimuli)
        {
            if (!Stimulus.IsValid())
                continue;

            TSharedPtr<FJsonObject> StimObj = MakeShared<FJsonObject>();
            StimObj->SetStringField(MindSandJsonKeys::Perception::Sense, Stimulus.Type.Name.ToString());
            StimObj->SetBoolField(MindSandJsonKeys::Perception::SuccessfullySensed, Stimulus.WasSuccessfullySensed());
            StimObj->SetNumberField(MindSandJsonKeys::Perception::Age, Stimulus.GetAge());

            StimuliArray.Add(MakeShared<FJsonValueObject>(StimObj));
        }

        ActorObj->SetArrayField(MindSandJsonKeys::Perception::Stimuli, StimuliArray);
        return MakeShared<FJsonValueObject>(ActorObj);
    }
}

TArray<TSharedPtr<FJsonValue>> UPerceptionStateGatherer::BuildKnownActorsArray(UAIPerceptionComponent *PerceptionComp,
                                                                               TSet<FString> &OutCollectedTypes,
                                                                               TSubclassOf<UAISense> SenseToInclude)
{
    if (!PerceptionComp)
    {
        return {};
    }

    TArray<AActor *> KnownActors;
    PerceptionComp->GetKnownPerceivedActors(SenseToInclude, KnownActors);

    TArray<TSharedPtr<FJsonValue>> Array;
    for (AActor *Actor : KnownActors)
    {
        TSharedPtr<FJsonValue> Entry = BuildKnownActorEntry(PerceptionComp, Actor, OutCollectedTypes);

        if (Entry.IsValid())
        {
            Array.Add(Entry);
        }
    }

    return Array;
}
