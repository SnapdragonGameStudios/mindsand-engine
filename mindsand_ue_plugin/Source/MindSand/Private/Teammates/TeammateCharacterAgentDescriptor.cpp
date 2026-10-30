// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "Teammates/TeammateCharacterAgentDescriptor.h"
#include "ActorDescriptorComponent.h"
#include "GameFramework/Actor.h"
#include "MindSandLog.h"

UTeammateCharacterAgentDescriptor::UTeammateCharacterAgentDescriptor()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UTeammateCharacterAgentDescriptor::BeginPlay()
{
    Super::BeginPlay();

    // Blueprint component templates have no owner for the editor hook to update. Sync at runtime before
    // perception lazily mints the actor ID; teammate registration may happen later or not at all.
    SyncUniqueIDToActorDescriptor();
}

FString UTeammateCharacterAgentDescriptor::GetTeammateName() const
{
    return Desc.Name;
}

FString UTeammateCharacterAgentDescriptor::GetSquad() const
{
    return Desc.Squad;
}

FString UTeammateCharacterAgentDescriptor::GetPersonality() const
{
    return Desc.Personality;
}

TArray<FString> UTeammateCharacterAgentDescriptor::GetActions() const
{
    return Desc.Actions;
}

bool UTeammateCharacterAgentDescriptor::SetTeammateName(const FString &NewName)
{
    Desc.Name = NewName;
    return SyncUniqueIDToActorDescriptor();
}

void UTeammateCharacterAgentDescriptor::SetSquad(const FString &NewSquad)
{
    Desc.Squad = NewSquad;
}

void UTeammateCharacterAgentDescriptor::SetPersonality(const FString &NewPersonality)
{
    Desc.Personality = NewPersonality;
}

void UTeammateCharacterAgentDescriptor::SetActions(const TArray<FString> &NewActions)
{
    Desc.Actions = NewActions;
}

bool UTeammateCharacterAgentDescriptor::SetDesc(const FTeammateDesc &NewDesc)
{
    Desc = NewDesc;

    return SyncUniqueIDToActorDescriptor();
}

bool UTeammateCharacterAgentDescriptor::SyncUniqueIDToActorDescriptor()
{
    if (Desc.Name.IsEmpty())
    {
        return false;
    }

    AActor *Owner = GetOwner();
    if (Owner == nullptr)
    {
        // Blueprint templates have no owner; BeginPlay syncs the value at runtime.
        return false;
    }

    UActorDescriptorComponent *ActorDescriptor = Owner->FindComponentByClass<UActorDescriptorComponent>();
    if (ActorDescriptor == nullptr)
    {
        UE_LOG(LogMindSand, Verbose,
               TEXT("TeammateCharacterAgentDescriptor: '%s' has no UActorDescriptorComponent, so teammate name "
                    "'%s' cannot be mirrored into a UniqueID."),
               *GetNameSafe(Owner), *Desc.Name);
        return false;
    }

#if WITH_EDITOR
    // Make editor changes undoable and mark the owning package dirty.
    ActorDescriptor->Modify();
#endif

    // UniqueID is read when the registry first mints this actor's ID; later updates are rejected.
    return ActorDescriptor->SetUniqueID(Desc.Name);
}

#if WITH_EDITOR
void UTeammateCharacterAgentDescriptor::PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent)
{
    // Desc.Name is nested, so match the inner property name rather than the outer Desc member.
    const FName ChangedPropertyName =
        (PropertyChangedEvent.Property != nullptr ? PropertyChangedEvent.Property->GetFName() : NAME_None);

    if (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(FTeammateDesc, Name))
    {
        // Only placed actors have an owner here; BeginPlay handles other cases.
        SyncUniqueIDToActorDescriptor();
    }

    Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif
