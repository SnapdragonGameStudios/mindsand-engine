// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "Actions/GameplayTask_Action.h"
#include "ActionParameterReflection.h"
#include "Actions/ActionPlanEngine.h"
#include "ActorRegistrySubsystem.h"
#include "Core/AITeammateOrchestrator.h"
#include "Core/MindSandConstants.h"
#include "MindSandLog.h"
#include "Teammates/TeammateCharacterAgentComponent.h"
#include <AIController.h>

namespace
{
// Actor references from model output are ActorRegistry IDs. Claim non-class object properties even when
// resolution fails, preventing the converter from instantiating arbitrary objects or resolving object paths.
// Class properties remain with the converter because they represent UClass values, not actors.
bool TranslateActorReference(const TSharedPtr<FJsonValue> &JsonValue, FProperty *Property, void *ValuePtr,
                             UObject *WorldContext)
{
    FObjectPropertyBase *ObjectProperty = CastField<FObjectPropertyBase>(Property);
    if (!ObjectProperty || Property->IsA<FClassProperty>() || Property->IsA<FSoftClassProperty>())
    {
        return false;
    }

    FString ActorId;
    if (!JsonValue->TryGetString(ActorId) || ActorId.IsEmpty())
    {
        UE_LOG(LogMindSand, Warning,
               TEXT("GameplayTask_Action: '%s' expects an ActorRegistry ID string; leaving it unset."),
               *Property->GetName());
        return true;
    }

    // The registry may be unavailable when the context has no world.
    const UActorRegistrySubsystem *Registry = UActorRegistrySubsystem::Get(WorldContext);
    if (!Registry)
    {
        UE_LOG(LogMindSand, Warning, TEXT("GameplayTask_Action: no ActorRegistry available for '%s'."),
               *Property->GetName());
        return true;
    }

    if (ActorId.Compare("RESOLVED") == 0 || ActorId.Compare("Pending") == 0 || ActorId.IsEmpty())
    {
        Property->GetOwnerUObject();
        if (UGameplayTask_Action *action = Cast<UGameplayTask_Action>(WorldContext))
        {
            AActor *ResolvedActor = action->GetTargetedActorForActions();
            ObjectProperty->SetObjectPropertyValue(ValuePtr, ResolvedActor);
            return true;
        }
    }

    AActor *ResolvedActor = Registry->GetActorFromID(ActorId);
    if (!ResolvedActor)
    {
        UE_LOG(LogMindSand, Warning,
               TEXT("GameplayTask_Action: '%s' referenced actor ID '%s', which is unknown or destroyed."),
               *Property->GetName(), *ActorId);
        return true;
    }

    // Reject actors that do not match the property's declared class.
    if (ObjectProperty->PropertyClass && !ResolvedActor->IsA(ObjectProperty->PropertyClass))
    {
        UE_LOG(LogMindSand, Warning,
               TEXT("GameplayTask_Action: actor '%s' is a %s, which is not a %s; leaving '%s' unset."), *ActorId,
               *ResolvedActor->GetClass()->GetName(), *ObjectProperty->PropertyClass->GetName(), *Property->GetName());
        return true;
    }

    // The property setter handles object, weak-object, and soft-object properties.
    ObjectProperty->SetObjectPropertyValue(ValuePtr, ResolvedActor);
    return true;
}
} // namespace

UGameplayTask_Action::UGameplayTask_Action(const FObjectInitializer &ObjectInitializer) : Super(ObjectInitializer)
{
    if (AAIController *AIController = Cast<AAIController>(GetOwnerActor()))
    {

        if (UTeammateCharacterAgentComponent *planner =
                AIController->GetComponentByClass<UTeammateCharacterAgentComponent>())
        {
            Planner = planner->GetActionPlanEngine();
            bConditionsValidationEnabled = Planner->GetUseConditionsAndEffects();
        }
    }
}

void UGameplayTask_Action::ActivateAction(IGameplayTaskOwnerInterface &InTaskOwner, const FString &JsonParams,
                                          const uint8 priority /*= FGameplayTasks::DefaultPriority*/)
{

    InitTask(InTaskOwner, Priority);
    ProcessParameters(JsonParams);

    if (bConditionsValidationEnabled)
    {
        bMeetConditions = VerifyConditions();
    }

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    UE_LOG(LogMindSand, Log, TEXT("Action: %s Started"), *GetDebugString());
#endif
    ReadyForActivation();
}

void UGameplayTask_Action::SetDataFromAsset(UActionDescriptionAsset *asset)
{
    for (auto &item : asset->OutEffects)
    {
        OutEffects.Add(item.Key, item.Value);
    }
    for (auto &item : asset->Conditions)
    {
        Conditions.Add(item.Key, item.Value);
    }

    Cost = asset->Cost;

    // ReflectedParameters is the reflection allowlist. AdditionalParameters is deliberately kept out of it:
    // those are advertised to the model but must not be written by reflection, so a ProcessParameters override
    // extracts them from the raw JSON instead.
    ActionParameters.Reserve(asset->ReflectedParameters.Num());
    for (const FNonEditableActionParameters &Reflected : asset->ReflectedParameters)
    {
        ActionParameters.Add({Reflected.Name, Reflected.Type, Reflected.ParameterDescription, Reflected.bIsOptional});
    }
    AdditionalParameters.Append(asset->AdditionalParameters);
}

void UGameplayTask_Action::ProcessParameters(const FString &ParametersJson)
{
    // CreateStatic appends its payload as the LAST argument, which is why TranslateActorReference takes
    // WorldContext last; the cast keeps the deduced payload type matching the function signature.
    ReflectParametersJson(
        this, ActionParameters, ParametersJson,
        FActionParameterValueTranslator::CreateStatic(&TranslateActorReference, static_cast<UObject *>(this)));
}

bool UGameplayTask_Action::BroadcastEndActionEvent()
{
    if (AActor *actor = Cast<AActor>(TaskOwner.Get()))
    {
        if (UTeammateCharacterAgentComponent *agent = actor->GetComponentByClass<UTeammateCharacterAgentComponent>())
        {
            UGameInstance *game = actor->GetGameInstance();
            if (UAITeammateOrchestrator *Orchestrator = game->GetSubsystem<UAITeammateOrchestrator>())
            {
                FString payload = agent->GetCharacterName();
                Orchestrator->SendEvent(MindSandConstants::TeammateActionFinishedEvent, payload);
                return true;
            }
        }
    }
    return false;
}

void UGameplayTask_Action::SetTargetedActorForActions(AActor *actor)
{
    if (AAIController *AIController = Cast<AAIController>(GetOwnerActor()))
    {

        if (UTeammateCharacterAgentComponent *planner =
                AIController->GetComponentByClass<UTeammateCharacterAgentComponent>())
        {
            planner->GetActionPlanEngine()->SetTargetedActorForActions(actor);
        }
    }
}

AActor *UGameplayTask_Action::GetTargetedActorForActions()
{
    if (AAIController *AIController = Cast<AAIController>(GetOwnerActor()))
    {

        if (UTeammateCharacterAgentComponent *planner =
                AIController->GetComponentByClass<UTeammateCharacterAgentComponent>())
        {
            return planner->GetActionPlanEngine()->GetTargetedActorForActions();
        }
    }
    return nullptr;
}

bool UGameplayTask_Action::VerifyConditions()
{
    if (Planner)
    {
        bool result = true;
        for (auto &condition : Conditions)
        {
            ensureMsgf(Planner->GetPlanStateVariableValue(condition.Key, result),
                       TEXT("Missing Planner State Variable"));
            if (!result)
            {
                return false;
            }
        }
    }
    return true;
}

void UGameplayTask_Action::SetEffects()
{
    if (Planner)
    {
        bool result = true;
        for (auto &effect : OutEffects)
        {
            ensureMsgf(Planner->UpdatePlanStateVariable(effect.Key, effect.Value),
                       TEXT("Missing Planner State Variable"));
        }
    }
}

void UGameplayTask_Action::EndAction(bool result)
{
    FName ObjectName = GetFName();
    FString StringVersion = ObjectName.ToString();
    UE_LOG(LogMindSand, Log, TEXT("UGameplayTask_Action::EndAction(): %s"), *StringVersion);
    if (StringVersion == TEXT("None"))
    {
        UE_LOG(LogMindSand, Error, TEXT("UGameplayTask_Action::EndAction(): Invalid Action (%s). Now what?!?!?"),
               *StringVersion);
        return;
    }

    FActionResult ActionResult;
    ActionResult.Result = result ? EActionResult::ActionSucess : EActionResult::ActionFail;

    if (bConditionsValidationEnabled && result)
    {
        SetEffects();
    }

    OnActionEnd();

    if (OnActionFinished != nullptr)
    {
        OnActionFinished(ActionResult);
        OnActionFinished = nullptr;
    }

    if (DispatchEndActionEventToPlanner)
        BroadcastEndActionEvent();

    OnFinish.Broadcast(result);
    EndTask();

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    UE_LOG(LogMindSand, Log, TEXT("Action: %s Finished with result: %s"), *GetDebugString(),
           result ? TEXT("success") : TEXT("fail"));
#endif
}

void UGameplayTask_Action::Abort(const FString &reason)
{
    FName ObjectName = GetFName();
    FString StringVersion = ObjectName.ToString();
    UE_LOG(LogMindSand, Log, TEXT("UGameplayTask_Action::EndAction(): %s"), *StringVersion);
    if (StringVersion == TEXT("None"))
    {
        UE_LOG(LogMindSand, Error, TEXT("UGameplayTask_Action::EndAction(): Invalid Action (%s). Now what?!?!?"),
               *StringVersion);
        return;
    }

    FActionResult ActionResult;
    ActionResult.Result = EActionResult::ActionAbort;
    ActionResult.Reason = reason;
    OnFinish.Broadcast(false);
    EndTask();
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    UE_LOG(LogMindSand, Log, TEXT("Action: %s Finished with result: %s"), *GetDebugString(), TEXT("Abort"));
#endif

    if (OnActionFinished != nullptr)
    {
        OnActionFinished(ActionResult);
        OnActionFinished = nullptr;
    }

    if (DispatchEndActionEventToPlanner)
        BroadcastEndActionEvent();
}

void UGameplayTask_Action::OnEvent(const FString &eventName, const FString &payload)
{
}

void UGameplayTask_Action::OnGameplayTaskActivated(UGameplayTask &Task)
{
    if (bConditionsValidationEnabled)
    {
        if (!VerifyConditions())
        {
            EndAction(false);
        }
    }
}
