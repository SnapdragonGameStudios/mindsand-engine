// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "CoreMinimal.h"
#include "IActionPlanParser.h"

// IActionPlanParser implementation that reads plans from a JSON-formatted LLM response
// of the form: { "Plans": [ { "teammate": "...", "acknowledgment": "...",
//                             "Actions": [ { "Name": "...", "SignalEndTaskToTeammates": bool,
//                                            "parameters": { ... } } ] } ] }
class FJsonActionPlanParser : public IActionPlanParser
{
  public:
    // Returns false on malformed JSON, missing "Plans" field, or any action entry missing a "Name".
    virtual bool Parse(const FString &Response, TArray<FParsedPlan> &OutPlans) const override;
};
