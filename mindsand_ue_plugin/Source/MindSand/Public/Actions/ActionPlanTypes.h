// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "CoreMinimal.h"

// One executable action produced by a plan parser, prior to gameplay-task resolution.
struct FParsedAction
{
    // Matches UActionDescriptionAsset::ActionName; required by the parser.
    FString Name;

    // Serialized JSON of the action's "parameters" object; empty if none provided.
    FString ParametersJson;

    // If true, the action should broadcast a completion event to other teammates.
    bool bSignalEndTaskToTeammates = false;
};

// One plan (teammate + acknowledgment + ordered actions) extracted from a response.
struct FParsedPlan
{
    // May be empty; the orchestrator falls back to random selection when so.
    FString TeammateId;

    // Free-form acknowledgment string from the LLM.
    FString Acknowledgment;

    TArray<FParsedAction> Actions;
};
