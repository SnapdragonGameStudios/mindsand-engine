// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "Actions/Parsers/StreamingActionPlanTokenProcessor.h"

#include "MindSandLog.h"

FStreamingActionPlanTokenProcessor::FStreamingActionPlanTokenProcessor(TUniquePtr<IStreamingActionPlanParser> InParser)
    : Parser(MoveTemp(InParser))
{
}

FQueryId FStreamingActionPlanTokenProcessor::Begin(FOnActionsParsed ActionsCallback,
                                                   FOnTokenPlanFinished PlanFinishedCallback)
{
    if (InputState == EQueryInputState::Accepting)
    {
        Finish(false);
    }

    QueryId = NextQueryId++;
    if (NextQueryId == 0)
    {
        NextQueryId = 1;
    }
    InputState = EQueryInputState::Accepting;
    OnActionsParsed = MoveTemp(ActionsCallback);
    OnPlanFinished = MoveTemp(PlanFinishedCallback);
    TeammateOrder.Empty();
    TouchedTeammates.Empty();
    FailedTeammates.Empty();
    if (Parser.IsValid())
    {
        Parser->Reset();
    }

    UE_LOG(LogMindSand, Log, TEXT("[Query %llu] Started streaming token processing."),
           static_cast<unsigned long long>(QueryId));
    return QueryId;
}

bool FStreamingActionPlanTokenProcessor::ProcessTokens(ELLMSentenceCode Code, const FString &Token)
{
    if (InputState != EQueryInputState::Accepting || !Parser.IsValid())
    {
        return false;
    }

    if (Code == ELLMSentenceCode::Abort)
    {
        UE_LOG(LogMindSand, Warning, TEXT("[Query %llu] Token stream aborted."),
               static_cast<unsigned long long>(QueryId));
        Finish(false);
        return false;
    }

    const bool bFinalChunk = Code == ELLMSentenceCode::End || Code == ELLMSentenceCode::Complete;
    const bool bParsed =
        Parser->ProcessChunk(Token, bFinalChunk, [this](const FString &TeammateId, TArray<FParsedAction> &&Actions) {
            RecordTeammate(TeammateId);
            if (FailedTeammates.Contains(TeammateId))
            {
                return;
            }

            UE_LOG(LogMindSand, Verbose, TEXT("[Query %llu] Streaming %d action(s) for teammate [%s]."),
                   static_cast<unsigned long long>(QueryId), Actions.Num(), *TeammateId);
            if (!OnActionsParsed || !OnActionsParsed(QueryId, TeammateId, MoveTemp(Actions)))
            {
                FailedTeammates.Add(TeammateId);
                UE_LOG(LogMindSand, Warning,
                       TEXT("[Query %llu] Teammate [%s] rejected a streamed action; later actions will be ignored."),
                       static_cast<unsigned long long>(QueryId), *TeammateId);
            }
        });

    if (!bParsed)
    {
        UE_LOG(LogMindSand, Warning, TEXT("[Query %llu] Streaming action-plan parsing failed."),
               static_cast<unsigned long long>(QueryId));
        Finish(false);
        return false;
    }

    if (bFinalChunk)
    {
        const bool bSucceeded = FailedTeammates.IsEmpty();
        Finish(true);
        return bSucceeded;
    }

    return true;
}

void FStreamingActionPlanTokenProcessor::Stop()
{
    if (InputState == EQueryInputState::Accepting)
    {
        Finish(false);
    }
    else
    {
        InputState = EQueryInputState::Ended;
    }
}

void FStreamingActionPlanTokenProcessor::Finish(bool bParsingSucceeded)
{
    if (InputState != EQueryInputState::Accepting)
    {
        return;
    }

    InputState = EQueryInputState::Ended;
    UE_LOG(LogMindSand, Log, TEXT("[Query %llu] Ended streaming token processing (parse success: %s)."),
           static_cast<unsigned long long>(QueryId), bParsingSucceeded ? TEXT("true") : TEXT("false"));

    if (OnPlanFinished)
    {
        for (const FString &TeammateId : TeammateOrder)
        {
            OnPlanFinished(QueryId, TeammateId, bParsingSucceeded && !FailedTeammates.Contains(TeammateId));
        }
    }
}

void FStreamingActionPlanTokenProcessor::RecordTeammate(const FString &TeammateId)
{
    if (!TouchedTeammates.Contains(TeammateId))
    {
        TouchedTeammates.Add(TeammateId);
        TeammateOrder.Add(TeammateId);
    }
}

FQueryId FStreamingActionPlanTokenProcessor::GetQueryId() const
{
    return QueryId;
}

bool FStreamingActionPlanTokenProcessor::IsAccepting() const
{
    return InputState == EQueryInputState::Accepting;
}

bool FStreamingActionPlanTokenProcessor::IsInactive() const
{
    return InputState == EQueryInputState::Inactive;
}

bool FStreamingActionPlanTokenProcessor::IsEnded() const
{
    return InputState == EQueryInputState::Ended;
}
