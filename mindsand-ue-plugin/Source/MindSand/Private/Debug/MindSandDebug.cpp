// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "Debug/MindSandDebug.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "HAL/FileManager.h"
#include "MindSandLog.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace MindSandDebug_Internal
{
static int32 DumpJson = 0;
static FAutoConsoleVariableRef CVarDumpJson(
    TEXT("MindSand.DumpJson"), DumpJson,
    TEXT("When enabled (1), dumps LLM request and response text to Saved/Logs/MindSand/<Date>/. Default 0 (off)."),
    ECVF_Cheat);

static int32 ExpandStrings = 1;
static FAutoConsoleVariableRef CVarExpandStrings(
    TEXT("MindSand.DumpJson.ExpandStrings"), ExpandStrings,
    TEXT("When enabled (1, default), expands multi-line string fields into readable raw blocks. When 0, dumps standard "
         "pretty-printed JSON with no special string handling."),
    ECVF_Cheat);

// Standard pretty-printed JSON: valid JSON, no special-case handling of string fields.
static FString PrettyPrint(const FString &Json)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return Json;
    }

    FString Pretty;
    TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer =
        TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&Pretty);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    return Pretty;
}

static FString MakeIndent(int32 Level)
{
    FString Result;
    for (int32 i = 0; i < Level; ++i)
    {
        Result += TEXT("\t");
    }
    return Result;
}

static FString RenderValue(const TSharedPtr<FJsonValue> &Value, int32 IndentLevel);

// Multi-line strings are dumped as raw blocks between quotes so prompt/response text stays
// readable; single-line strings stay inline.
// NOTE: the result is intentionally NOT valid JSON (raw newlines inside strings) -- it is a
// display format for eyeballing, which is why the file is written as .txt.
static FString RenderString(const FString &Str, int32 IndentLevel)
{
    if (Str.Contains(TEXT("\n")))
    {
        const FString InnerIndent = MakeIndent(IndentLevel + 1);
        const FString Indented = InnerIndent + Str.Replace(TEXT("\n"), *(TEXT("\n") + InnerIndent));
        return FString::Printf(TEXT("\"\n%s\n%s\""), *Indented, *MakeIndent(IndentLevel));
    }
    return FString::Printf(TEXT("\"%s\""), *Str);
}

static FString RenderArray(const TArray<TSharedPtr<FJsonValue>> &Arr, int32 IndentLevel)
{
    if (Arr.Num() == 0)
    {
        return TEXT("[]");
    }

    const FString InnerIndent = MakeIndent(IndentLevel + 1);
    TArray<FString> Lines;
    for (const TSharedPtr<FJsonValue> &Elem : Arr)
    {
        Lines.Add(InnerIndent + RenderValue(Elem, IndentLevel + 1));
    }
    return TEXT("[\n") + FString::Join(Lines, TEXT(",\n")) + TEXT("\n") + MakeIndent(IndentLevel) + TEXT("]");
}

static FString RenderObject(const TSharedPtr<FJsonObject> &Obj, int32 IndentLevel)
{
    if (!Obj.IsValid() || Obj->Values.Num() == 0)
    {
        return TEXT("{}");
    }

    const FString InnerIndent = MakeIndent(IndentLevel + 1);
    TArray<FString> Lines;
    for (const TPair<FString, TSharedPtr<FJsonValue>> &Field : Obj->Values)
    {
        Lines.Add(InnerIndent +
                  FString::Printf(TEXT("\"%s\": %s"), *Field.Key, *RenderValue(Field.Value, IndentLevel + 1)));
    }
    return TEXT("{\n") + FString::Join(Lines, TEXT(",\n")) + TEXT("\n") + MakeIndent(IndentLevel) + TEXT("}");
}

// Renders any JSON value at the given nesting depth. Recurses into objects/arrays so that
// multi-line strings are expanded no matter how deep they're nested.
static FString RenderValue(const TSharedPtr<FJsonValue> &Value, int32 IndentLevel)
{
    switch (Value->Type)
    {
    case EJson::String:
        return RenderString(Value->AsString(), IndentLevel);
    case EJson::Number:
    case EJson::Boolean:
        return Value->AsString();
    case EJson::Array:
        return RenderArray(Value->AsArray(), IndentLevel);
    case EJson::Object:
        return RenderObject(Value->AsObject(), IndentLevel);
    default:
        return Value->AsString();
    }
}

// Turn a JSON payload into a JSON-shaped readable dump, with embedded prompt/response text
// expanded into real line breaks at any nesting depth. Falls back to the raw string unchanged
// if it fails to parse (e.g. plain prompt/response text, not JSON), so it's still captured.
static FString MakeReadable(const FString &Json)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return Json;
    }

    return RenderObject(JsonObject, 0);
}
} // namespace MindSandDebug_Internal

void DumpMindSandJson(const FString &TurnId, const TCHAR *Kind, const FString &Json)
{
    if (MindSandDebug_Internal::DumpJson == 0)
    {
        return;
    }

    const FDateTime Now = FDateTime::Now();
    const FString Date = Now.ToString(TEXT("%Y-%m-%d"));
    const FString Dir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Logs"), TEXT("MindSand"), Date);
    IFileManager::Get().MakeDirectory(*Dir, /*Tree=*/true);

    // Lead with date + time so files sort chronologically within the day's folder.
    const FString Timestamp = Now.ToString(TEXT("%Y-%m-%d_%H-%M-%S"));
    const FString FileName = FString::Printf(TEXT("%s_%s_%s.json"), *Timestamp, *TurnId, Kind);
    const FString FilePath = FPaths::Combine(Dir, FileName);

    // Force UTF-8 so a non-ANSI character in the prompt doesn't switch the whole file to UTF-16.
    const FString Contents = MindSandDebug_Internal::ExpandStrings != 0 ? MindSandDebug_Internal::MakeReadable(Json)
                                                                        : MindSandDebug_Internal::PrettyPrint(Json);
    if (!FFileHelper::SaveStringToFile(Contents, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        UE_LOG(LogMindSand, Error, TEXT("DumpMindSandJson: failed to write %s"), *FilePath);
    }
}
