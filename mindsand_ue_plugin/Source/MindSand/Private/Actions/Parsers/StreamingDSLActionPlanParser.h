// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "IStreamingActionPlanParser.h"

/** Incrementally parses the Python-like DSL action-plan response format line by line. */
class FStreamingDSLActionPlanParser : public IStreamingActionPlanParser
{
  public:
    /** Resets buffered lines, teammate state, and parser validation state. */
    void Reset() override;

    /** Processes a DSL token chunk and emits any complete action lines. */
    bool ProcessChunk(const FString &Text, bool bFinalChunk, const FOnActionsParsed &OnActionsParsed) override;

  private:
    bool ProcessLine(FString Line, const FOnActionsParsed &OnActionsParsed);
    bool ValidateCompleteResponse() const;

    FString PendingLine;
    FString CurrentTeammate;
    TMap<FString, int32> ActionCountsByTeammate;
    bool bSawAction = false;
    bool bFailed = false;
};
