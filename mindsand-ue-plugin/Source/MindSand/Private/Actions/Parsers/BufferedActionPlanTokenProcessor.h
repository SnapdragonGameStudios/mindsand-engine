// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "Actions/Parsers/IActionPlanParser.h"
#include "Core/IActionPlanTokenProcessor.h"

/** Buffers the complete response before parsing and dispatching action plans. */
class MINDSAND_API FBufferedActionPlanTokenProcessor : public IActionPlanTokenProcessor
{
  public:
    /** Creates a buffered processor without a parser. */
    FBufferedActionPlanTokenProcessor() = default;
    /** Creates a buffered processor with a complete-response parser. */
    explicit FBufferedActionPlanTokenProcessor(TUniquePtr<IActionPlanParser> InParser);
    /** Starts a new query and clears the buffered response. */
    FQueryId Begin(FOnActionsParsed ActionsCallback, FOnTokenPlanFinished PlanFinishedCallback) override;
    /** Buffers tokens and parses the response at its terminal token. */
    bool ProcessTokens(ELLMSentenceCode Code, const FString &Token) override;
    /** Stops processing and clears the buffered response. */
    void Stop() override;
    /** Returns the current query identifier. */
    FQueryId GetQueryId() const override;
    /** Returns whether tokens are accepted. */
    bool IsAccepting() const override;
    /** Returns whether no query is active. */
    bool IsInactive() const override;
    /** Returns whether processing has ended. */
    bool IsEnded() const override;

  private:
    FQueryId QueryId = 0;
    FQueryId NextQueryId = 1;
    EQueryInputState InputState = EQueryInputState::Inactive;
    FString SavedResponse;
    FOnActionsParsed OnActionsParsed;
    FOnTokenPlanFinished OnPlanFinished;
    TUniquePtr<IActionPlanParser> Parser;
};
