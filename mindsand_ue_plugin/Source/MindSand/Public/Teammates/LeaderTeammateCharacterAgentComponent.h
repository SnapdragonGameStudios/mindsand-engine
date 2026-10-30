// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "Core/AITeammateOrchestrator.h"
#include "CoreMinimal.h"
#include "TeammateCharacterAgentComponent.h"

#include "LeaderTeammateCharacterAgentComponent.generated.h"

class UAIPerceptionComponent;

UCLASS(Blueprintable, BlueprintType, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent),
       hideCategories = "ActionPlanEngine")
class MINDSAND_API ULeaderTeammateCharacterAgentComponent : public UActorComponent
{
    GENERATED_BODY()

  public:
    UPROPERTY(BlueprintReadWrite)
    FString TeamName;

    ULeaderTeammateCharacterAgentComponent();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION(BlueprintCallable)
    void SendComand(const FString &command, const FString &base64);

  private:
    void RegisterPlayerWithRegister(APawn *Pawn);

    UFUNCTION()
    void HandlePawnChanged(APawn *OldPawn, APawn *NewPawn);
};
