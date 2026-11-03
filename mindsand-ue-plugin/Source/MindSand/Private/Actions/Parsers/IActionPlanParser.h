// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "Actions/ActionPlanTypes.h"
#include "CoreMinimal.h"

/** Converts a complete raw LLM response into parsed teammate action plans. */
class IActionPlanParser
{
  public:
    /** Releases parser state. */
    virtual ~IActionPlanParser() = default;

    /** Parses Response into OutPlans. Returns false when the response is invalid or contains no usable plan. */
    virtual bool Parse(const FString &Response, TArray<FParsedPlan> &OutPlans) const = 0;
};
