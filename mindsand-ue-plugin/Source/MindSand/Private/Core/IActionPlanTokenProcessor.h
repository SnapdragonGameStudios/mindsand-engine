// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "Actions/ActionPlanTypes.h"
#include "CoreMinimal.h"
#include "Framework/ConversationEngine.h"

using FQueryId = uint64;

enum class EQueryInputState : uint8
{
    Inactive,  // No query has been started.
    Accepting, // Tokens are accepted for the current query.
    Ended      // The current query reached a terminal state.
};

/**
 * Consumes response tokens and translates parsed action plans into teammate action callbacks.
 *
 * Implementations own parser state for one query at a time. Call Begin() before supplying tokens, forward each
 * token through ProcessTokens(), and use the per-teammate completion callback to close the corresponding action plan.
 */
class MINDSAND_API IActionPlanTokenProcessor
{
  public:
    /** Called when a complete action batch for a teammate has been parsed. Return false to report dispatch failure. */
    using FOnActionsParsed =
        TFunction<bool(FQueryId QueryId, const FString &TeammateId, TArray<FParsedAction> &&Actions)>;

    /** Called once for each teammate when that teammate's plan processing is complete. */
    using FOnTokenPlanFinished = TFunction<void(FQueryId QueryId, const FString &TeammateId, bool bSuccess)>;

    /** Releases the processor and any parser state it owns. */
    virtual ~IActionPlanTokenProcessor() = default;

    /** Starts a new query and installs callbacks for parsed actions and per-teammate completion. */
    virtual FQueryId Begin(FOnActionsParsed ActionsCallback, FOnTokenPlanFinished PlanFinishedCallback) = 0;

    /** Supplies the next response token. Returns true when terminal processing succeeds. */
    virtual bool ProcessTokens(ELLMSentenceCode Code, const FString &Token) = 0;

    /** Stops the current query and discards input/parser state. */
    virtual void Stop() = 0;

    /** Returns the identifier of the current query. */
    virtual FQueryId GetQueryId() const = 0;

    /** Returns true while the processor accepts response tokens. */
    virtual bool IsAccepting() const = 0;

    /** Returns true when no query has been started. */
    virtual bool IsInactive() const = 0;

    /** Returns true after processing has reached a terminal state or Stop() was called. */
    virtual bool IsEnded() const = 0;
};
