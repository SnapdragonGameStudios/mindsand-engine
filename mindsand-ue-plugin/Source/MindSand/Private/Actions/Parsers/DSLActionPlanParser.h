// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "CoreMinimal.h"
#include "IActionPlanParser.h"

/** Parses the complete Python-like DSL action-plan response format. */
class FDSLActionPlanParser : public IActionPlanParser
{
  public:
    /** Parses DSL plans; quoted and unquoted parameter values are supported. */
    virtual bool Parse(const FString &Response, TArray<FParsedPlan> &OutPlans) const override;
};
