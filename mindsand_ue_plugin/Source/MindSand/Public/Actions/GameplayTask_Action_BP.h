// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "ActionDescriptionAsset.h"
#include "CoreMinimal.h"
#include "GameplayTask_Action.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptInterface.h"

#include "GameplayTask_Action_BP.generated.h"

/**
 *
 */

// class UActionDescriptionAsset;

UCLASS(Abstract, BlueprintType, Blueprintable)
class MINDSAND_API UGameplayTask_Action_BP : public UGameplayTask_Action
{
    GENERATED_BODY()

  public:
    UGameplayTask_Action_BP(const FObjectInitializer &ObjectInitializer);

    UFUNCTION(BlueprintImplementableEvent)
    void ActivateBP();

    UFUNCTION(BlueprintImplementableEvent)
    FString GetDebugStringBP() const;

    UFUNCTION(BlueprintImplementableEvent)
    void TickTaskBP();

    UFUNCTION(BlueprintCallable)
    void EndActionBP(bool result);

    UFUNCTION(BlueprintNativeEvent)
    void ProcessParametersBP(const FString &ParameterJson);

    UFUNCTION(BlueprintCallable)
    AActor *GetActionOwner();

    UFUNCTION(BlueprintImplementableEvent)
    void OnEventBP(const FString &eventName, const FString &payload);

    UFUNCTION(BlueprintImplementableEvent)
    void OnActionEndBP();

    virtual void Activate() override;

    /** Return debug string describing task */
    virtual FString GetDebugString() const override;

    virtual void TickTask(float DeltaTime) override;

    virtual void OnEvent(const FString &eventName, const FString &payload) override;

    virtual void OnActionEnd() override;

    virtual void ProcessParameters(const FString &JsonParams) override;

  protected:
    UPROPERTY(EditDefaultsOnly)
    bool bTickableAction;

  public:
};
