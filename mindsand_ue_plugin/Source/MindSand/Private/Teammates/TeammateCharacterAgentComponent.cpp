// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "Teammates/TeammateCharacterAgentComponent.h"
#include "AIController.h"
#include "Actions/ActionPlanEngine.h"
#include "Actions/ActionRegistrySubsystem.h"
#include "Core/MindSandJsonKeys.h"
#include "Dom/JsonObject.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "GameplayTagsManager.h"
#include "MindSandLog.h"
#include "Perception/AIPerceptionComponent.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Teammates/TeammateCharacterAgentDescriptor.h"
#include "Teammates/TeammateRegistry.h"

namespace
{
UTeammateCharacterAgentDescriptor *FindDescriptorOnActor(AActor *Actor)
{
    if (Actor == nullptr)
    {
        UE_LOG(LogMindSand, Warning, TEXT("FindDescriptorOnActor: Actor is null"));
        return nullptr;
    }

    AController *Controller = Cast<AController>(Actor);
    if (Controller == nullptr)
    {
        UE_LOG(LogMindSand, Warning, TEXT("FindDescriptorOnActor: Owner is not an AController"));
        return nullptr;
    }

    APawn *Pawn = Controller->GetPawn();
    if (Pawn == nullptr)
    {
        UE_LOG(LogMindSand, Warning, TEXT("FindDescriptorOnActor: No possessed pawn"));
        return nullptr;
    }

    UTeammateCharacterAgentDescriptor *Descriptor = Pawn->FindComponentByClass<UTeammateCharacterAgentDescriptor>();
    if (Descriptor == nullptr)
    {
        UE_LOG(LogMindSand, Warning, TEXT("FindDescriptorOnActor: Pawn has no UTeammateCharacterAgentDescriptor"));
    }

    return Descriptor;
}
} // namespace

UTeammateCharacterAgentComponent::UTeammateCharacterAgentComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

UTeammateCharacterAgentDescriptor *UTeammateCharacterAgentComponent::ResolveDescriptor() const
{
    if (CachedDescriptor.IsValid())
    {
        return CachedDescriptor.Get();
    }

    UTeammateCharacterAgentDescriptor *Descriptor = FindDescriptorOnActor(GetOwner());
    if (Descriptor != nullptr)
    {
        CachedDescriptor = Descriptor;
    }

    return Descriptor;
}

void UTeammateCharacterAgentComponent::Reset()
{
    UTeammateRegistry *Registry = UTeammateRegistry::Get(GetOwner());
    if (Registry != nullptr)
    {
        Registry->UnregisterTeammate(this);
    }

    LiveActions.Empty();
    CachedDescriptor = nullptr;
}

FString UTeammateCharacterAgentComponent::GetCharacterPersonality() const
{
    const UTeammateCharacterAgentDescriptor *Descriptor = ResolveDescriptor();
    return Descriptor ? Descriptor->GetPersonality() : FString();
}

FString UTeammateCharacterAgentComponent::GetCharacterName() const
{
    const UTeammateCharacterAgentDescriptor *Descriptor = ResolveDescriptor();
    return Descriptor ? Descriptor->GetTeammateName() : FString();
}

FString UTeammateCharacterAgentComponent::GetCharacterSquad() const
{
    const UTeammateCharacterAgentDescriptor *Descriptor = ResolveDescriptor();
    return Descriptor ? Descriptor->GetSquad() : FString();
}

TArray<FString> UTeammateCharacterAgentComponent::GetAvailableActions() const
{
    return LiveActions;
}

void UTeammateCharacterAgentComponent::BeginPlay()
{
    Super::BeginPlay();

    AActor *owner = this->GetOwner();

    AController *OwnerController = Cast<AController>(owner);
    ensureMsgf(OwnerController != nullptr, TEXT("UTeammateCharacterAgentComponent must live on an AController, not %s"),
               *GetNameSafe(GetOwner()));

    {
        ActionPlanEngine = MakeUnique<FActionPlanEngine>(owner);
        ActionPlanEngine->InterruptLogic = InterruptLogic;
        ActionPlanEngine->bUseConditionsAndEffects = UseConditionsAndEffects;
    }

    PerceptionComponent = owner->GetComponentByClass<UAIPerceptionComponent>();
    if (PerceptionComponent == nullptr)
    {
        UE_LOG(LogMindSand, Warning,
               TEXT("There is no perception component on this actor, so it wont be able to get information about the "
                    "world."));
    }

    if (OwnerController != nullptr)
    {
        OwnerController->OnPossessedPawnChanged.AddDynamic(
            this, &UTeammateCharacterAgentComponent::HandlePossessedPawnChanged);

        // Handle the case where possession already happened before BeginPlay.
        if (APawn *AlreadyPossessed = OwnerController->GetPawn())
        {
            HandlePossessedPawnChanged(nullptr, AlreadyPossessed);
        }
    }
}

void UTeammateCharacterAgentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (ActionPlanEngine.IsValid())
    {
        ActionPlanEngine.Reset();
    }

    if (AController *OwnerController = Cast<AController>(GetOwner()))
    {
        OwnerController->OnPossessedPawnChanged.RemoveDynamic(
            this, &UTeammateCharacterAgentComponent::HandlePossessedPawnChanged);
    }

    if (UTeammateRegistry *Registry = UTeammateRegistry::Get(GetOwner()))
    {
        Registry->UnregisterTeammate(this);
    }
    Super::EndPlay(EndPlayReason);
}

void UTeammateCharacterAgentComponent::HandlePossessedPawnChanged(APawn *OldPawn, APawn *NewPawn)
{
    // Any possession change invalidates what was cached from the previous pawn, so reset all state
    // before looking at the new pawn.
    Reset();

    if (NewPawn == nullptr)
    {
        return;
    }

    UTeammateCharacterAgentDescriptor *Descriptor = FindDescriptorOnActor(GetOwner());
    CachedDescriptor = Descriptor;

    if (Descriptor != nullptr && Descriptor->bRegisterOnPossess)
    {
        RegisterTeammate();
    }

    // Manual mode (or a pawn with no descriptor at all): nothing is registered until something calls
    // RegisterTeammate() explicitly, but GetCharacterName/Personality/Squad will still work via
    // the cached descriptor pointer if one is present.
}

void UTeammateCharacterAgentComponent::RegisterTeammate()
{
    UTeammateCharacterAgentDescriptor *Descriptor = FindDescriptorOnActor(GetOwner());
    if (Descriptor == nullptr)
    {
        return;
    }

    UTeammateRegistry *Registry = UTeammateRegistry::Get(GetOwner());
    if (Registry == nullptr)
    {
        APawn *Pawn = Cast<AController>(GetOwner())->GetPawn();
        UE_LOG(LogMindSand, Warning, TEXT("RegisterTeammate: no UTeammateRegistry for pawn %s"), *GetNameSafe(Pawn));
        return;
    }

    // Drop any existing entry first: the descriptor's Squad may have changed since the last call, and
    // RegisterTeammate would otherwise leave this component under its old squad key.
    Registry->UnregisterTeammate(this);

    CachedDescriptor = Descriptor;

    // Every call resyncs the action set from the descriptor, so runtime RegisterActions/UnRegisterActions
    // edits are discarded. The authored names are dropped and re-added one at a time through RegisterAction
    // so each is validated against the action registry and gets its plan state variables registered.
    UE_LOG(LogMindSand, Log, TEXT("Registering [%s] : seeding %d default actions from descriptor"),
           *Descriptor->GetTeammateName(), Descriptor->GetActions().Num());

    LiveActions.Empty();
    for (const FString &Action : Descriptor->GetActions())
    {
        ensureMsgf(RegisterAction(Action), TEXT("Action %s not found"), *Action);
    }

    Registry->RegisterTeammate(Descriptor->GetSquad(), this);
}

bool UTeammateCharacterAgentComponent::RegisterAction(const FString &ActionName)
{
    UActionRegistrySubsystem *ActionRegistry = GetOwner()->GetGameInstance()->GetSubsystem<UActionRegistrySubsystem>();
    UActionDescriptionAsset *asset = ActionRegistry->FindActionByName(ActionName);

    if (asset == nullptr)
    {
        UE_LOG(LogMindSand, Error, TEXT("RegisterAction [%s] Failed"), *ActionName);
        return false;
    }

    LiveActions.Add(ActionName);

    if (ActionPlanEngine.IsValid() && UseConditionsAndEffects)
    {
        ActionPlanEngine->RegisterPlanStateVariables(asset);
    }
    return true;
}

bool UTeammateCharacterAgentComponent::RegisterActions(const TArray<FString> &ActionName)
{
    bool result = true;
    for (auto &item : ActionName)
    {
        result &= RegisterAction(item);
    }
    return result;
}

bool UTeammateCharacterAgentComponent::UnRegisterActions(const TArray<FString> &ActionName)
{
    bool result = true;
    for (auto &item : ActionName)
    {
        result &= UnRegisterAction(item);
    }
    return result;
}

bool UTeammateCharacterAgentComponent::UnRegisterAction(const FString &ActionName)
{
    return (1 == LiveActions.Remove(ActionName));
}

bool UTeammateCharacterAgentComponent::UpdateStateVariableValue(const FString &VariableName, bool value)
{
    if (ActionPlanEngine.IsValid())
    {
        return ActionPlanEngine->UpdatePlanStateVariable(VariableName, value);
    }
    return false;
}

FActionPlanEngine *UTeammateCharacterAgentComponent::GetActionPlanEngine()
{
    return ActionPlanEngine.Get();
}

void UTeammateCharacterAgentComponent::ReadStateVariables(TMap<FString, bool> &Results) const
{
    if (ActionPlanEngine.IsValid())
    {
        ActionPlanEngine->ReadState(Results);
    }
}

void UTeammateCharacterAgentComponent::SetInterruptLogic(bool value)
{
    if (ActionPlanEngine.IsValid())
    {
        InterruptLogic = value;
        ActionPlanEngine->InterruptLogic = InterruptLogic;
    }
}

void UTeammateCharacterAgentComponent::SetUseConditionsAndEffects(bool value)
{
    if (ActionPlanEngine.IsValid())
    {
        UseConditionsAndEffects = value;
        ActionPlanEngine->bUseConditionsAndEffects = UseConditionsAndEffects;
    }
}

TSharedPtr<FJsonObject> UTeammateCharacterAgentComponent::GetJSON() const
{
    TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetStringField(MindSandJsonKeys::Teammate::Name, GetCharacterName());
    Root->SetStringField(MindSandJsonKeys::Teammate::Personality, GetCharacterPersonality());

    TArray<TSharedPtr<FJsonValue>> ActionsArray;
    ActionsArray.Reserve(LiveActions.Num());
    for (const FString &Action : LiveActions)
    {
        ActionsArray.Add(MakeShared<FJsonValueString>(Action));
    }
    Root->SetArrayField(MindSandJsonKeys::Teammate::AvailableActions, ActionsArray);

    if (UseConditionsAndEffects)
    {
        TSharedRef<FJsonObject> TeammateState = MakeShared<FJsonObject>();
        TMap<FString, bool> state;
        ReadStateVariables(state);
        for (auto &item : state)
        {
            TeammateState->SetBoolField(item.Key, item.Value);
        }
        Root->SetObjectField(MindSandJsonKeys::Teammate::StateVariables, TeammateState);
    }

    return Root;
}

TArray<TSharedPtr<FJsonValue>> UTeammateCharacterAgentComponent::GetJSON(
    const TArray<UTeammateCharacterAgentComponent *> &Teammates)
{
    TArray<TSharedPtr<FJsonValue>> Result;
    Result.Reserve(Teammates.Num());
    for (UTeammateCharacterAgentComponent *Teammate : Teammates)
    {
        if (!Teammate)
        {
            continue;
        }
        Result.Add(MakeShared<FJsonValueObject>(Teammate->GetJSON()));
    }
    return Result;
}
