// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "DSLActionPlanParser.h"

#include "MindSandLog.h"

#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
bool SplitTopLevel(const FString &Text, TCHAR Separator, TArray<FString> &Parts)
{
    int32 Start = 0, Parentheses = 0, Brackets = 0, Braces = 0;
    bool bInString = false, bEscaped = false;
    for (int32 Index = 0; Index < Text.Len(); ++Index)
    {
        const TCHAR Character = Text[Index];
        if (bInString)
        {
            if (bEscaped)
                bEscaped = false;
            else if (Character == TEXT('\\'))
                bEscaped = true;
            else if (Character == TEXT('"'))
                bInString = false;
            continue;
        }
        if (Character == TEXT('"'))
            bInString = true;
        else if (Character == TEXT('('))
            ++Parentheses;
        else if (Character == TEXT(')'))
        {
            if (--Parentheses < 0)
                return false;
        }
        else if (Character == TEXT('['))
            ++Brackets;
        else if (Character == TEXT(']'))
        {
            if (--Brackets < 0)
                return false;
        }
        else if (Character == TEXT('{'))
            ++Braces;
        else if (Character == TEXT('}'))
        {
            if (--Braces < 0)
                return false;
        }
        else if (Character == Separator && Parentheses == 0 && Brackets == 0 && Braces == 0)
        {
            Parts.Add(Text.Mid(Start, Index - Start));
            Start = Index + 1;
        }
    }
    if (bInString || Parentheses != 0 || Brackets != 0 || Braces != 0)
        return false;
    Parts.Add(Text.Mid(Start));
    return true;
}

bool SplitParameter(const FString &Text, FString &OutName, FString &OutValue)
{
    UE_LOG(LogMindSand, Verbose, TEXT("[DSLActionPlanParser] SplitParameter input [%s]."), *Text);
    TArray<FString> Parts;
    if (!SplitTopLevel(Text, TEXT(':'), Parts) || Parts.Num() != 2)
    {
        UE_LOG(LogMindSand, Warning,
               TEXT("[DSLActionPlanParser] SplitParameter failed: expected one top-level colon; got %d part(s)."),
               Parts.Num());
        return false;
    }
    OutName = Parts[0].TrimStartAndEnd();
    OutValue = Parts[1].TrimStartAndEnd();
    const bool bValid = !OutName.IsEmpty() && !OutValue.IsEmpty();
    UE_LOG(LogMindSand, Verbose, TEXT("[DSLActionPlanParser] SplitParameter name [%s], value [%s], valid=%d."),
           *OutName, *OutValue, bValid ? 1 : 0);
    return bValid;
}

bool UnescapeQuotedString(const FString &Text, FString &OutValue)
{
    OutValue.Empty();
    OutValue.Reserve(Text.Len());

    for (int32 Index = 0; Index < Text.Len(); ++Index)
    {
        const TCHAR Character = Text[Index];
        if (Character != TEXT('\\'))
        {
            if (Character == TEXT('"'))
                return false;
            OutValue.AppendChar(Character);
            continue;
        }

        if (++Index >= Text.Len())
            return false;

        switch (Text[Index])
        {
        case TEXT('"'):
            OutValue.AppendChar(TEXT('"'));
            break;
        case TEXT('\\'):
            OutValue.AppendChar(TEXT('\\'));
            break;
        case TEXT('/'):
            OutValue.AppendChar(TEXT('/'));
            break;
        case TEXT('b'):
            OutValue.AppendChar(TEXT('\b'));
            break;
        case TEXT('f'):
            OutValue.AppendChar(TEXT('\f'));
            break;
        case TEXT('n'):
            OutValue.AppendChar(TEXT('\n'));
            break;
        case TEXT('r'):
            OutValue.AppendChar(TEXT('\r'));
            break;
        case TEXT('t'):
            OutValue.AppendChar(TEXT('\t'));
            break;
        case TEXT('u'): {
            if (Index + 4 >= Text.Len())
                return false;

            const FString Hex = Text.Mid(Index + 1, 4);
            for (int32 HexIndex = 0; HexIndex < Hex.Len(); ++HexIndex)
            {
                const TCHAR HexCharacter = Hex[HexIndex];
                const bool bIsHexDigit = (HexCharacter >= TEXT('0') && HexCharacter <= TEXT('9')) ||
                                         (HexCharacter >= TEXT('a') && HexCharacter <= TEXT('f')) ||
                                         (HexCharacter >= TEXT('A') && HexCharacter <= TEXT('F'));
                if (!bIsHexDigit)
                    return false;
            }
            const int32 CodePoint = FCString::Strtoi(*Hex, nullptr, 16);
            OutValue.AppendChar(static_cast<TCHAR>(CodePoint));
            Index += 4;
            break;
        }
        default:
            return false;
        }
    }

    return true;
}

bool ParseValue(const FString &Text, TSharedPtr<FJsonValue> &OutValue)
{
    UE_LOG(LogMindSand, Verbose, TEXT("[DSLActionPlanParser] ParseValue input [%s]."), *Text);
    if (Text.IsEmpty())
    {
        UE_LOG(LogMindSand, Warning, TEXT("[DSLActionPlanParser] ParseValue rejected empty value."));
        return false;
    }

    if (Text[0] == TEXT('[') || Text[Text.Len() - 1] == TEXT(']'))
    {
        if (Text.Len() < 2 || Text[0] != TEXT('[') || Text[Text.Len() - 1] != TEXT(']'))
        {
            UE_LOG(LogMindSand, Warning, TEXT("[DSLActionPlanParser] ParseValue rejected malformed array [%s]."),
                   *Text);
            return false;
        }

        const FString ArrayText = Text.Mid(1, Text.Len() - 2).TrimStartAndEnd();
        TArray<TSharedPtr<FJsonValue>> Elements;
        if (!ArrayText.IsEmpty())
        {
            TArray<FString> ElementTexts;
            if (!SplitTopLevel(ArrayText, TEXT(','), ElementTexts))
            {
                UE_LOG(LogMindSand, Warning,
                       TEXT("[DSLActionPlanParser] ParseValue rejected array with malformed elements [%s]."), *Text);
                return false;
            }

            for (const FString &ElementText : ElementTexts)
            {
                TSharedPtr<FJsonValue> Element;
                if (!ParseValue(ElementText.TrimStartAndEnd(), Element))
                {
                    UE_LOG(LogMindSand, Warning, TEXT("[DSLActionPlanParser] ParseValue rejected array element [%s]."),
                           *ElementText);
                    return false;
                }
                Elements.Add(Element);
            }
        }

        OutValue = MakeShared<FJsonValueArray>(Elements);
        const bool bParsed = OutValue.IsValid();
        UE_LOG(LogMindSand, Verbose,
               TEXT("[DSLActionPlanParser] ParseValue created JSON array with %d element(s), valid=%d."),
               Elements.Num(), bParsed ? 1 : 0);
        return bParsed;
    }

    const bool bStartsWithQuote = Text[0] == TEXT('"');
    const bool bEndsWithQuote = Text[Text.Len() - 1] == TEXT('"');
    if (bStartsWithQuote != bEndsWithQuote)
    {
        UE_LOG(LogMindSand, Warning, TEXT("[DSLActionPlanParser] ParseValue rejected mismatched quotes [%s]."), *Text);
        return false;
    }

    if (!bStartsWithQuote)
    {
        if (Text.Contains(TEXT("\"")))
        {
            UE_LOG(LogMindSand, Warning, TEXT("[DSLActionPlanParser] ParseValue rejected unexpected quote [%s]."),
                   *Text);
            return false;
        }

        OutValue = MakeShared<FJsonValueString>(Text);
        const bool bParsed = OutValue.IsValid();
        UE_LOG(LogMindSand, Verbose, TEXT("[DSLActionPlanParser] ParseValue accepted unquoted string value, valid=%d."),
               bParsed ? 1 : 0);
        return bParsed;
    }

    // FJsonSerializer::Deserialize does not accept a scalar JSON string in this UE build.
    // For quoted values, strip the delimiters and create the corresponding JSON value directly.
    FString StringValue;
    if (!UnescapeQuotedString(Text.Mid(1, Text.Len() - 2), StringValue))
    {
        UE_LOG(LogMindSand, Warning, TEXT("[DSLActionPlanParser] ParseValue rejected invalid string escape."));
        return false;
    }
    OutValue = MakeShared<FJsonValueString>(StringValue);
    const bool bParsed = OutValue.IsValid();
    UE_LOG(LogMindSand, Verbose, TEXT("[DSLActionPlanParser] ParseValue created JSON string value valid=%d."),
           bParsed ? 1 : 0);
    return bParsed;
}
} // namespace

bool FDSLActionPlanParser::Parse(const FString &Response, TArray<FParsedPlan> &OutPlans) const
{
    UE_LOG(LogMindSand, Verbose, TEXT("[DSLActionPlanParser] Parsing response (%d chars)."), Response.Len());
    TArray<FString> Lines;
    Response.ParseIntoArrayLines(Lines, false);

    TArray<FParsedPlan> ParsedPlans;
    int32 CurrentPlanIndex = INDEX_NONE;

    for (FString Line : Lines)
    {
        Line.TrimStartAndEndInline();
        if (Line.IsEmpty())
            continue;

        if (Line.StartsWith(TEXT("@")))
        {
            const FString Teammate = Line.RightChop(1).TrimStartAndEnd();
            UE_LOG(LogMindSand, Verbose, TEXT("[DSLActionPlanParser] Found teammate header [%s]."), *Teammate);
            if (Teammate.IsEmpty() || Teammate.Contains(TEXT(" ")))
                return false;

            CurrentPlanIndex = INDEX_NONE;
            for (int32 Index = 0; Index < ParsedPlans.Num(); ++Index)
            {
                if (ParsedPlans[Index].TeammateId == Teammate)
                {
                    CurrentPlanIndex = Index;
                    break;
                }
            }
            if (CurrentPlanIndex == INDEX_NONE)
            {
                CurrentPlanIndex = ParsedPlans.AddDefaulted();
                ParsedPlans[CurrentPlanIndex].TeammateId = Teammate;
            }
            continue;
        }

        if (CurrentPlanIndex == INDEX_NONE)
            return false;

        const int32 OpenParen = Line.Find(TEXT("("));
        if (OpenParen <= 0 || !Line.EndsWith(TEXT(")")))
        {
            UE_LOG(LogMindSand, Warning, TEXT("[DSLActionPlanParser] Malformed action line [%s]."), *Line);
            return false;
        }

        FParsedAction Action;
        Action.Name = Line.Left(OpenParen).TrimStartAndEnd();
        UE_LOG(LogMindSand, Verbose, TEXT("[DSLActionPlanParser] Parsing action [%s]."), *Action.Name);
        if (Action.Name.IsEmpty())
            return false;

        const FString ParameterText = Line.Mid(OpenParen + 1, Line.Len() - OpenParen - 2).TrimStartAndEnd();
        if (!ParameterText.IsEmpty())
        {
            TArray<FString> Parameters;
            if (!SplitTopLevel(ParameterText, TEXT(','), Parameters))
                return false;

            TSet<FName> ParameterNames;
            TSharedRef<FJsonObject> ParametersObject = MakeShared<FJsonObject>();
            for (const FString &Parameter : Parameters)
            {
                FString Name, ValueText;
                TSharedPtr<FJsonValue> Value;
                if (!SplitParameter(Parameter, Name, ValueText))
                {
                    UE_LOG(LogMindSand, Warning, TEXT("[DSLActionPlanParser] Invalid parameter [%s] for action [%s]."),
                           *Parameter, *Action.Name);
                    return false;
                }

                Name.ToLowerInline();
                const FName ParameterName(*Name);
                if (ParameterNames.Contains(ParameterName) || !ParseValue(ValueText, Value))
                {
                    UE_LOG(LogMindSand, Warning,
                           TEXT("[DSLActionPlanParser] Duplicate or invalid parameter [%s] for action [%s]."),
                           *Parameter, *Action.Name);
                    return false;
                }

                ParameterNames.Add(ParameterName);
                ParametersObject->SetField(Name, Value);
            }

            FString Output;
            const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
            FJsonSerializer::Serialize(ParametersObject, Writer);
            Writer->Close();
            Action.ParametersJson = Output;
            UE_LOG(LogMindSand, Verbose, TEXT("[DSLActionPlanParser] Action [%s] parameters JSON: %s"), *Action.Name,
                   *Action.ParametersJson);
        }
        ParsedPlans[CurrentPlanIndex].Actions.Add(MoveTemp(Action));
    }

    if (ParsedPlans.IsEmpty())
        return false;
    for (const FParsedPlan &Plan : ParsedPlans)
        if (Plan.Actions.IsEmpty())
            return false;

    UE_LOG(LogMindSand, Log, TEXT("[DSLActionPlanParser] Parsed %d plan(s)."), ParsedPlans.Num());
    OutPlans.Append(MoveTemp(ParsedPlans));
    return true;
}
