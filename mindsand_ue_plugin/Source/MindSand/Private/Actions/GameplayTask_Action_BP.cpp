// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "Actions/GameplayTask_Action_BP.h"

UGameplayTask_Action_BP::UGameplayTask_Action_BP(const FObjectInitializer &ObjectInitializer) : Super(ObjectInitializer)
{
}

void UGameplayTask_Action_BP::EndActionBP(bool result)
{
    EndAction(result);
}

void UGameplayTask_Action_BP::ProcessParametersBP_Implementation(const FString &ParameterJson)
{
    Super::ProcessParameters(ParameterJson);
}

AActor *UGameplayTask_Action_BP::GetActionOwner()
{
    return GetOwnerActor();
}

void UGameplayTask_Action_BP::Activate()
{
    if (bTickableAction)
    {
        bTickingTask = 1;
    }

    ActivateBP();
}

FString UGameplayTask_Action_BP::GetDebugString() const
{
    return GetDebugStringBP();
}

void UGameplayTask_Action_BP::TickTask(float DeltaTime)
{
    TickTaskBP();
}

void UGameplayTask_Action_BP::OnEvent(const FString &eventName, const FString &payload)
{
    OnEventBP(eventName, payload);
}

void UGameplayTask_Action_BP::OnActionEnd()
{
    OnActionEndBP();
}

void UGameplayTask_Action_BP::ProcessParameters(const FString &ParametersJson)
{
    ProcessParametersBP(ParametersJson);
}
