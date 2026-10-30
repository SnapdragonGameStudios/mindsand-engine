// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "Teammates/LeaderTeammateCharacterAgentComponent.h"
#include "AIController.h"
#include "ActorRegistrySubsystem.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerState.h"
#include "MindSandLog.h"
#include "Perception/AIPerceptionComponent.h"

ULeaderTeammateCharacterAgentComponent::ULeaderTeammateCharacterAgentComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    TeamName = "";
}

void ULeaderTeammateCharacterAgentComponent::BeginPlay()
{
    Super::BeginPlay();

    APlayerController *PC = Cast<APlayerController>(GetOwner());
    if (!PC)
    {
        UE_LOG(LogMindSand, Warning,
               TEXT("PlayerRegistrationComponent on '%s' - owner is not a PlayerController; component disabled."),
               *GetNameSafe(GetOwner()));
        return;
    }
    if (!PC->IsLocalController())
    {
        return;
    }

    PC->OnPossessedPawnChanged.AddDynamic(this, &ULeaderTeammateCharacterAgentComponent::HandlePawnChanged);
    RegisterPlayerWithRegister(PC->GetPawn());
}

void ULeaderTeammateCharacterAgentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (APlayerController *PC = Cast<APlayerController>(GetOwner()))
    {
        PC->OnPossessedPawnChanged.RemoveDynamic(this, &ULeaderTeammateCharacterAgentComponent::HandlePawnChanged);
    }

    Super::EndPlay(EndPlayReason);
}

void ULeaderTeammateCharacterAgentComponent::HandlePawnChanged(APawn *OldPawn, APawn *NewPawn)
{
    RegisterPlayerWithRegister(NewPawn);
}

void ULeaderTeammateCharacterAgentComponent::RegisterPlayerWithRegister(APawn *Pawn)
{
    if (UActorRegistrySubsystem *Registry = UActorRegistrySubsystem::Get(this))
    {
        Registry->RegisterActor(Pawn, "player");
    }
}

void ULeaderTeammateCharacterAgentComponent::SendComand(const FString &command, const FString &base64)
{
    UAITeammateOrchestrator *Orchestator = GetWorld()->GetGameInstance()->GetSubsystem<UAITeammateOrchestrator>();
    if (Orchestator != nullptr)
    {
        Orchestator->SendTeamLeaderCommandGoal(command, this);
    }
}
