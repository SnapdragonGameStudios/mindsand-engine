// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "Actions/GameplayTask_Action.h"
#include "CoreMinimal.h"
#include "GameplayTask_ResolveAction.generated.h"

/**
 *
 */
UCLASS()
class MINDSAND_API UGameplayTask_ResolveAction : public UGameplayTask_Action
{
    GENERATED_BODY()

  public:
    // clang-format off
    UPROPERTY(meta = (ToolTip = "ReferenceActor is the ActorID of the object to use as center reference point to locate the RESOLVED object necessary for subsequent actions. This can also have the word \"NONE\" if the search has to be done among a group of objects and there is no reference, the center of the group will be use as reference position"))
    AActor *ReferenceActor;
    // clang-format on

    // clang-format off
    UPROPERTY(meta = (ToolTip = "RelativeDirection: [Front|Back|Left|Right|Leftmost|Rightmost|Farthest|Nearest|Around]. This parameter is extremely case sensitive, Stick to these specified options."))
    FString RelativeDirection;
    // clang-format on

    UPROPERTY(meta = (ToolTip = "CategoryTag: FGameplayTag category of the required object to look for (e.g. ID.NPC)"))
    FString CategoryTag;

  protected:
    FVector ComputeCentroid(TArray<AActor *> objects);

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
