// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "CoreMinimal.h"

/**
 *
 */
class MINDSAND_API FActionPlanState
{
  public:
    FActionPlanState();
    ~FActionPlanState();

  public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AITeammate|ActionPlan")
    TMap<FString, bool> PlanStateVariables;
};
