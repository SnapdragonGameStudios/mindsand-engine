// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "Actions/Parsers/BufferedActionPlanTokenProcessor.h"

#include "MindSandLog.h"

FBufferedActionPlanTokenProcessor::FBufferedActionPlanTokenProcessor(TUniquePtr<IActionPlanParser> InParser)
    : Parser(MoveTemp(InParser))
{
}

FQueryId FBufferedActionPlanTokenProcessor::Begin(FOnActionsParsed ActionsCallback,
                                                  FOnTokenPlanFinished PlanFinishedCallback)
{
    if (InputState != EQueryInputState::Inactive)
    {
        Stop();
    }

    QueryId = NextQueryId++;
    if (NextQueryId == 0)
    {
        NextQueryId = 1;
    }
    InputState = EQueryInputState::Accepting;
    SavedResponse.Empty();
    OnActionsParsed = MoveTemp(ActionsCallback);
    OnPlanFinished = MoveTemp(PlanFinishedCallback);

    UE_LOG(LogMindSand, Log, TEXT("[Query %llu] Started token processing."), static_cast<unsigned long long>(QueryId));
    return QueryId;
}

bool FBufferedActionPlanTokenProcessor::ProcessTokens(ELLMSentenceCode Code, const FString &Token)
{
    if (InputState != EQueryInputState::Accepting)
    {
        return false;
    }

    if (Code == ELLMSentenceCode::Abort)
    {
        Stop();
        return false;
    }

    SavedResponse += Token;
    if (Code != ELLMSentenceCode::End && Code != ELLMSentenceCode::Complete)
    {
        return true;
    }

    UE_LOG(LogMindSand, VeryVerbose, TEXT("[Query %llu] Full response: %s"), static_cast<unsigned long long>(QueryId),
           *SavedResponse);

    TArray<FParsedPlan> ParsedPlans;
    const bool bParsed = Parser.IsValid() && Parser->Parse(SavedResponse, ParsedPlans);
    SavedResponse.Empty();
    InputState = EQueryInputState::Ended;

    if (!bParsed)
    {
        UE_LOG(LogMindSand, Warning, TEXT("[Query %llu] Failed to parse generated plans."),
               static_cast<unsigned long long>(QueryId));
        return false;
    }

    TArray<FString> TeammateOrder;
    TSet<FString> SeenTeammates;
    TMap<FString, TArray<FParsedAction>> ActionsByTeammate;
    TSet<FString> FailedTeammates;
    for (FParsedPlan &ParsedPlan : ParsedPlans)
    {
        if (!SeenTeammates.Contains(ParsedPlan.TeammateId))
        {
            SeenTeammates.Add(ParsedPlan.TeammateId);
            TeammateOrder.Add(ParsedPlan.TeammateId);
        }
        ActionsByTeammate.FindOrAdd(ParsedPlan.TeammateId).Append(MoveTemp(ParsedPlan.Actions));
    }

    for (const FString &TeammateId : TeammateOrder)
    {
        TArray<FParsedAction> &Actions = ActionsByTeammate.FindChecked(TeammateId);
        if (!Actions.IsEmpty() && (!OnActionsParsed || !OnActionsParsed(QueryId, TeammateId, MoveTemp(Actions))))
        {
            FailedTeammates.Add(TeammateId);
        }
    }

    if (OnPlanFinished)
    {
        for (const FString &TeammateId : TeammateOrder)
        {
            OnPlanFinished(QueryId, TeammateId, !FailedTeammates.Contains(TeammateId));
        }
    }
    return FailedTeammates.IsEmpty();
}

void FBufferedActionPlanTokenProcessor::Stop()
{
    if (InputState == EQueryInputState::Accepting)
    {
        UE_LOG(LogMindSand, Log, TEXT("[Query %llu] Ended token processing."),
               static_cast<unsigned long long>(QueryId));
    }
    InputState = EQueryInputState::Ended;
    SavedResponse.Empty();
}

FQueryId FBufferedActionPlanTokenProcessor::GetQueryId() const
{
    return QueryId;
}

bool FBufferedActionPlanTokenProcessor::IsAccepting() const
{
    return InputState == EQueryInputState::Accepting;
}

bool FBufferedActionPlanTokenProcessor::IsInactive() const
{
    return InputState == EQueryInputState::Inactive;
}

bool FBufferedActionPlanTokenProcessor::IsEnded() const
{
    return InputState == EQueryInputState::Ended;
}
