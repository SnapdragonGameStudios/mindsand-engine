// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "ActorDescriptorComponent.h"
#include "ActorRegistrySubsystem.h"
#include "GameFramework/Actor.h"

UActorDescriptorComponent::UActorDescriptorComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UActorDescriptorComponent::GetOwnedGameplayTags(FGameplayTagContainer &TagContainer) const
{
    if (IDTag.IsValid())
    {
        TagContainer.AddTag(IDTag);
    }
}

bool UActorDescriptorComponent::SetUniqueID(const FString &InUniqueID)
{
    if (const AActor *Owner = GetOwner())
    {
        if (const UWorld *World = Owner->GetWorld())
        {
            if (const UActorRegistrySubsystem *Registry = World->GetSubsystem<UActorRegistrySubsystem>())
            {
                if (Registry->IsActorRegistered(Owner))
                {
                    UE_LOG(
                        LogTemp, Warning,
                        TEXT("ActorDescriptorComponent::SetUniqueID('%s') ignored on actor '%s' — already registered."),
                        *InUniqueID, *GetNameSafe(Owner));
                    return false;
                }
            }
        }
    }

    UniqueID = InUniqueID;
    return true;
}

void UActorDescriptorComponent::SetState(const FString &InState)
{
    State = InState;
}
