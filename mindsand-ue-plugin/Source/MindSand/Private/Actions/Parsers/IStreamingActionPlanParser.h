// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "Actions/ActionPlanTypes.h"
#include "CoreMinimal.h"

/** Converts response token chunks into complete teammate action batches. */
class IStreamingActionPlanParser
{
  public:
    /** Receives complete action batches synchronously in source order. */
    using FOnActionsParsed = TFunction<void(const FString &TeammateId, TArray<FParsedAction> &&Actions)>;

    /** Releases parser state. */
    virtual ~IStreamingActionPlanParser() = default;

    /** Clears all incremental parser state and starts a new response. */
    virtual void Reset() = 0;

    /**
     * Processes a token chunk and emits complete actions synchronously. When bFinalChunk is true, the parser flushes
     * and validates any remaining input. Returns false after malformed input or failed final validation.
     */
    virtual bool ProcessChunk(const FString &Text, bool bFinalChunk, const FOnActionsParsed &OnActionsParsed) = 0;
};
