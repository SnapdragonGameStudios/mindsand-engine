// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "StreamingDSLActionPlanParser.h"

#include "DSLActionPlanParser.h"
#include "MindSandLog.h"

void FStreamingDSLActionPlanParser::Reset()
{
    PendingLine.Empty();
    CurrentTeammate.Empty();
    ActionCountsByTeammate.Empty();
    bSawAction = false;
    bFailed = false;
}

bool FStreamingDSLActionPlanParser::ProcessChunk(const FString &Text, bool bFinalChunk,
                                                 const FOnActionsParsed &OnActionsParsed)
{
    if (bFailed)
    {
        return false;
    }

    PendingLine += Text;
    int32 NewlineIndex = INDEX_NONE;
    while (PendingLine.FindChar(TEXT('\n'), NewlineIndex))
    {
        FString Line = PendingLine.Left(NewlineIndex);
        PendingLine = PendingLine.Mid(NewlineIndex + 1);
        if (!ProcessLine(MoveTemp(Line), OnActionsParsed))
        {
            bFailed = true;
            return false;
        }
    }

    if (!bFinalChunk)
    {
        return true;
    }

    if (!PendingLine.IsEmpty())
    {
        FString FinalLine = MoveTemp(PendingLine);
        PendingLine.Empty();
        if (!ProcessLine(MoveTemp(FinalLine), OnActionsParsed))
        {
            bFailed = true;
            return false;
        }
    }

    if (!ValidateCompleteResponse())
    {
        UE_LOG(LogMindSand, Warning, TEXT("[StreamingDSLActionPlanParser] Response failed final validation."));
        bFailed = true;
        return false;
    }

    return true;
}

bool FStreamingDSLActionPlanParser::ProcessLine(FString Line, const FOnActionsParsed &OnActionsParsed)
{
    Line.TrimStartAndEndInline();
    if (Line.IsEmpty())
    {
        return true;
    }

    if (Line.StartsWith(TEXT("@")))
    {
        const FString Teammate = Line.RightChop(1).TrimStartAndEnd();
        if (Teammate.IsEmpty() || Teammate.Contains(TEXT(" ")))
        {
            UE_LOG(LogMindSand, Warning, TEXT("[StreamingDSLActionPlanParser] Invalid teammate header [%s]."), *Line);
            return false;
        }

        CurrentTeammate = Teammate;
        ActionCountsByTeammate.FindOrAdd(CurrentTeammate);
        UE_LOG(LogMindSand, Verbose, TEXT("[StreamingDSLActionPlanParser] Current teammate [%s]."), *CurrentTeammate);
        return true;
    }

    if (CurrentTeammate.IsEmpty())
    {
        UE_LOG(LogMindSand, Warning, TEXT("[StreamingDSLActionPlanParser] Action has no teammate [%s]."), *Line);
        return false;
    }

    // Reuse the complete DSL parser for action grammar and value conversion so the buffered and streaming paths
    // cannot disagree about parameters, arrays, quoting, or escaping.
    const FString SingleActionResponse = FString::Printf(TEXT("@%s\n%s"), *CurrentTeammate, *Line);
    TArray<FParsedPlan> ParsedPlans;
    FDSLActionPlanParser Parser;
    if (!Parser.Parse(SingleActionResponse, ParsedPlans) || ParsedPlans.Num() != 1 || ParsedPlans[0].Actions.Num() != 1)
    {
        UE_LOG(LogMindSand, Warning, TEXT("[StreamingDSLActionPlanParser] Invalid action line [%s]."), *Line);
        return false;
    }

    ++ActionCountsByTeammate.FindChecked(CurrentTeammate);
    bSawAction = true;
    if (OnActionsParsed)
    {
        OnActionsParsed(CurrentTeammate, MoveTemp(ParsedPlans[0].Actions));
    }
    return true;
}

bool FStreamingDSLActionPlanParser::ValidateCompleteResponse() const
{
    if (!bSawAction || ActionCountsByTeammate.IsEmpty())
    {
        return false;
    }

    for (const TPair<FString, int32> &Pair : ActionCountsByTeammate)
    {
        if (Pair.Value == 0)
        {
            return false;
        }
    }
    return true;
}
