// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "Actions/GameplayTask_Action.h"
#include "CoreMinimal.h"
#include "GameplayTask_SearchAroundAction.generated.h"

/**
 *
 */
UCLASS()
class MINDSAND_API UGameplayTask_SearchAroundAction : public UGameplayTask_Action
{
    GENERATED_BODY()

  public:
    UPROPERTY()
    AActor *ReferenceActor;
    UPROPERTY(
        meta = (DisplayName = "RelativeDirection: [Front|Back|Left|Right|Leftmost|Rightmost|Farthest|Nearest|Around]"))
    FString RelativeDirection;
    UPROPERTY()
    TArray<AActor *> ActorsToLookFor;

  protected:
    enum SearchDirectionOption
    {
        SD_FRONT,
        SD_BACK,
        SD_LEFT,
        SD_RIGHT,
        SD_LEFTMOST,
        SD_RIGHTMOST,
        SD_FARTHEST,
        SD_NEAREST,
        SD_AROUND
    } SearchDirection;

    void Activate() override;
};
