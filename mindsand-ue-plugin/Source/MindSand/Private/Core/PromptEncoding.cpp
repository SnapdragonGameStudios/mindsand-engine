// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "Core/PromptEncoding.h"
#include "Core/MindSandJsonKeys.h"
#include "Core/Utils.h"
#include "Json.h"
#include "MindSandLog.h"

namespace
{
FString EncodeAction(const TSharedPtr<FJsonObject> &ActionDescription)
{
    FString Output;
    FString Name;
    FString Description;
    if (!ActionDescription->TryGetStringField(MindSandJsonKeys::Action::Name, Name) ||
        !ActionDescription->TryGetStringField(MindSandJsonKeys::Action::Description, Description))
    {
        return Output;
    }

    const TArray<TSharedPtr<FJsonValue>> *Parameters = nullptr;
    if (!ActionDescription->TryGetArrayField(MindSandJsonKeys::Action::Parameters, Parameters))
    {
        UE_LOG(LogMindSand, Warning, TEXT("Prompt encoding: action '%s' has no valid Parameters array."), *Name);
        return Output;
    }

    Output += FString::Printf(TEXT("## %s (%s)\n"), *Name, *Description);

    for (const TSharedPtr<FJsonValue> &ParameterValue : *Parameters)
    {
        if (!ParameterValue.IsValid() || ParameterValue->Type != EJson::Object)
        {
            continue;
        }
        TSharedPtr<FJsonObject> Parameter = ParameterValue->AsObject();
        if (!Parameter.IsValid())
        {
            continue;
        }

        FString ParameterName;
        FString ParameterType;
        FString ParameterDescription;
        if (!Parameter->TryGetStringField(MindSandJsonKeys::ActionParameter::Name, ParameterName) ||
            !Parameter->TryGetStringField(MindSandJsonKeys::ActionParameter::Type, ParameterType) ||
            !Parameter->TryGetStringField(MindSandJsonKeys::ActionParameter::Description, ParameterDescription))
        {
            continue;
        }

        bool bRequired = false;
        if (!Parameter->TryGetBoolField(MindSandJsonKeys::ActionParameter::IsRequired, bRequired))
        {
            UE_LOG(LogMindSand, Warning,
                   TEXT("Prompt encoding: skipping action parameter '%s' without an IsRequired flag."), *ParameterName);
            continue;
        }

        Output += FString::Printf(TEXT("- %s (%s, %s): %s\n"), *ParameterName, *ParameterType,
                                  bRequired ? TEXT("required") : TEXT("optional"), *ParameterDescription);
    }

    return Output;
}

FString EncodeTeammate(const TSharedPtr<FJsonValue> &Value)
{
    FString Output;
    TSharedPtr<FJsonObject> Teammate;
    if (!Value.IsValid() || Value->Type != EJson::Object)
    {
        return Output;
    }
    Teammate = Value->AsObject();
    if (!Teammate.IsValid())
    {
        return Output;
    }

    FString Name;
    FString Personality;
    if (!Teammate->TryGetStringField(MindSandJsonKeys::Teammate::Name, Name) ||
        !Teammate->TryGetStringField(MindSandJsonKeys::Teammate::Personality, Personality))
    {
        return Output;
    }

    const TArray<TSharedPtr<FJsonValue>> *Actions = nullptr;
    if (!Teammate->TryGetArrayField(MindSandJsonKeys::Teammate::AvailableActions, Actions))
    {
        UE_LOG(LogMindSand, Warning, TEXT("Prompt encoding: teammate '%s' has no valid availableActions array."),
               *Name);
        return Output;
    }

    TArray<FString> ActionNames;
    for (const TSharedPtr<FJsonValue> &ActionValue : *Actions)
    {
        if (!ActionValue.IsValid() || ActionValue->Type != EJson::String)
        {
            UE_LOG(LogMindSand, Warning, TEXT("Prompt encoding: skipping malformed available action for '%s'."), *Name);
            continue;
        }
        ActionNames.Add(ActionValue->AsString());
    }

    Output += FString::Printf(TEXT("## %s\n- Personality: %s\n- Actions: %s\n"), *Name, *Personality,
                              *FString::Join(ActionNames, TEXT(", ")));

    const TSharedPtr<FJsonObject> *StateVariables = nullptr;
    if (!Teammate->TryGetObjectField(MindSandJsonKeys::Teammate::StateVariables, StateVariables))
    {
        if (Teammate->HasField(MindSandJsonKeys::Teammate::StateVariables))
        {
            UE_LOG(LogMindSand, Warning, TEXT("Prompt encoding: skipping malformed TeammateStateVariables for '%s'."),
                   *Name);
        }
        return Output;
    }

    TArray<FString> States;
    for (const TPair<FString, TSharedPtr<FJsonValue>> &State : (*StateVariables)->Values)
    {
        if (!State.Value.IsValid() || State.Value->Type != EJson::Boolean)
        {
            UE_LOG(LogMindSand, Warning, TEXT("Prompt encoding: skipping malformed state '%s' for '%s'."), *State.Key,
                   *Name);
            continue;
        }
        States.Add(FString::Printf(TEXT("%s=%s"), *State.Key, State.Value->AsBool() ? TEXT("true") : TEXT("false")));
    }

    if (!States.IsEmpty())
    {
        Output += FString::Printf(TEXT("- State: %s\n"), *FString::Join(States, TEXT(", ")));
    }

    return Output;
}
} // namespace

namespace MindSandPromptEncoding
{
FString EncodeDynamicGameInfo(const TSharedRef<FJsonObject> &DynamicGameInfo)
{
    return Utils::Serialize(DynamicGameInfo);
}

FString EncodeStaticGameInfo(const TSharedRef<FJsonObject> &StaticGameInfo)
{
    FString Output;

    const TArray<TSharedPtr<FJsonValue>> *Actions = nullptr;
    if (StaticGameInfo->TryGetArrayField(MindSandJsonKeys::StaticGameInfo::ActionDescriptions, Actions))
    {
        Output += TEXT("# Actions\n");
        for (const TSharedPtr<FJsonValue> &ActionValue : *Actions)
        {
            if (ActionValue.IsValid() && ActionValue->Type == EJson::Object)
            {
                const TSharedPtr<FJsonObject> ActionDescription = ActionValue->AsObject();
                if (ActionDescription.IsValid())
                {
                    Output += EncodeAction(ActionDescription);
                }
            }
        }
    }

    const TArray<TSharedPtr<FJsonValue>> *Teammates = nullptr;
    if (StaticGameInfo->TryGetArrayField(MindSandJsonKeys::StaticGameInfo::Teammates, Teammates) &&
        !Teammates->IsEmpty())
    {
        if (!Output.IsEmpty())
        {
            Output += TEXT("\n");
        }
        Output += TEXT("# Teammates\n");
        for (const TSharedPtr<FJsonValue> &Teammate : *Teammates)
        {
            Output += EncodeTeammate(Teammate);
        }
    }

    const TArray<TSharedPtr<FJsonValue>> *GlobalEvents = nullptr;
    if (StaticGameInfo->TryGetArrayField(MindSandJsonKeys::StaticGameInfo::GlobalEvents, GlobalEvents) &&
        !GlobalEvents->IsEmpty())
    {
        if (!Output.IsEmpty())
        {
            Output += TEXT("\n");
        }
        Output += TEXT("# Global Events\n");
        for (const TSharedPtr<FJsonValue> &Event : *GlobalEvents)
        {
            if (!Event.IsValid() || Event->Type != EJson::String)
            {
                UE_LOG(LogMindSand, Warning, TEXT("Prompt encoding: skipping malformed global event."));
                continue;
            }
            Output += FString::Printf(TEXT("- %s\n"), *Event->AsString());
        }
    }

    return Output;
}

FString EncodePrompt(const TSharedRef<FJsonObject> &Prompt)
{
    FString Output;

    const TSharedPtr<FJsonObject> *StaticGameInfo = nullptr;
    if (Prompt->TryGetObjectField(MindSandJsonKeys::Prompt::StaticGameInfo, StaticGameInfo) &&
        StaticGameInfo != nullptr && StaticGameInfo->IsValid())
    {
        Output += EncodeStaticGameInfo(StaticGameInfo->ToSharedRef());
    }
    else if (Prompt->HasField(MindSandJsonKeys::Prompt::StaticGameInfo))
    {
        UE_LOG(LogMindSand, Warning, TEXT("Prompt encoding: prompt has no valid staticGameInfo object."));
    }

    const TSharedPtr<FJsonObject> *DynamicGameInfo = nullptr;
    if (Prompt->TryGetObjectField(MindSandJsonKeys::Prompt::DynamicGameInfo, DynamicGameInfo) &&
        DynamicGameInfo != nullptr && DynamicGameInfo->IsValid())
    {
        if (!Output.IsEmpty())
        {
            Output += TEXT("\n");
        }
        Output += TEXT("# Dynamic Game Info\n");
        Output += EncodeDynamicGameInfo(DynamicGameInfo->ToSharedRef());
        Output += TEXT("\n");
    }
    else
    {
        UE_LOG(LogMindSand, Warning, TEXT("Prompt encoding: prompt has no valid dynamicGameInfo object."));
    }

    FString Command;
    if (Prompt->TryGetStringField(MindSandJsonKeys::Prompt::Command, Command))
    {
        if (!Output.IsEmpty())
        {
            Output += TEXT("\n");
        }
        Output += TEXT("# Command\n");
        Output += Command;
        Output += TEXT("\n");
    }
    else
    {
        UE_LOG(LogMindSand, Warning, TEXT("Prompt encoding: prompt has no valid command string."));
    }

    return Output;
}
} // namespace MindSandPromptEncoding
