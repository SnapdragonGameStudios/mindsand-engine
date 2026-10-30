// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "Actions/Parsers/IStreamingActionPlanParser.h"
#include "Core/IActionPlanTokenProcessor.h"

/** Forwards tokens to an incremental parser and dispatches completed action groups during generation. */
class MINDSAND_API FStreamingActionPlanTokenProcessor : public IActionPlanTokenProcessor
{
  public:
    /** Creates a streaming processor without a parser. */
    FStreamingActionPlanTokenProcessor() = default;
    /** Creates a streaming processor with an incremental parser. */
    explicit FStreamingActionPlanTokenProcessor(TUniquePtr<IStreamingActionPlanParser> InParser);
    /** Starts a new query and resets parser and teammate state. */
    FQueryId Begin(FOnActionsParsed ActionsCallback, FOnTokenPlanFinished PlanFinishedCallback) override;
    /** Forwards a token and dispatches completed action groups. */
    bool ProcessTokens(ELLMSentenceCode Code, const FString &Token) override;
    /** Stops processing and clears parser state. */
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
    void Finish(bool bParsingSucceeded);
    void RecordTeammate(const FString &TeammateId);

    FQueryId QueryId = 0;
    FQueryId NextQueryId = 1;
    EQueryInputState InputState = EQueryInputState::Inactive;
    FOnActionsParsed OnActionsParsed;
    FOnTokenPlanFinished OnPlanFinished;
    TUniquePtr<IStreamingActionPlanParser> Parser;
    TArray<FString> TeammateOrder;
    TSet<FString> TouchedTeammates;
    TSet<FString> FailedTeammates;
};
