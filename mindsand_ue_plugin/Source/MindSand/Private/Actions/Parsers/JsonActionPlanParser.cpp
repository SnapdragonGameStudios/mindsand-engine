// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "JsonActionPlanParser.h"
#include "Core/MindSandJsonKeys.h"
#include "Core/Utils.h"
#include "Json.h"

bool FJsonActionPlanParser::Parse(const FString &Response, TArray<FParsedPlan> &OutPlans) const
{
    const size_t StartOfJson = Response.Find(TEXT("{"), ESearchCase::CaseSensitive, ESearchDir::FromStart);
    const size_t EndOfJson = Response.Find(TEXT("}"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
    const FString JsonString = Response.Mid(StartOfJson, EndOfJson - StartOfJson + 1);

    TSharedPtr<FJsonObject> JsonResponseParsed;
    const TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(JsonString);
    if (!FJsonSerializer::Deserialize(JsonReader, JsonResponseParsed) || !JsonResponseParsed.IsValid())
    {
        return false;
    }

    const TArray<TSharedPtr<FJsonValue>> *PlansArrayPtr = nullptr;
    if (!JsonResponseParsed->TryGetArrayField(MindSandJsonKeys::ActionPlanResponse::Plans, PlansArrayPtr) ||
        PlansArrayPtr == nullptr)
    {
        return false;
    }

    for (const TSharedPtr<FJsonValue> &PlanValue : *PlansArrayPtr)
    {
        const TSharedPtr<FJsonObject> PlanObject = PlanValue->AsObject();
        if (!PlanObject.IsValid())
        {
            continue;
        }

        FParsedPlan Plan;
        PlanObject->TryGetStringField(MindSandJsonKeys::ActionPlanResponse::Teammate, Plan.TeammateId);
        PlanObject->TryGetStringField(MindSandJsonKeys::ActionPlanResponse::Acknowledgment, Plan.Acknowledgment);

        bool bPlanValid = true;
        const TArray<TSharedPtr<FJsonValue>> *ActionsArrayPtr = nullptr;
        if (PlanObject->TryGetArrayField(MindSandJsonKeys::ActionPlanResponse::Actions, ActionsArrayPtr) &&
            ActionsArrayPtr != nullptr)
        {
            for (const TSharedPtr<FJsonValue> &ActionValue : *ActionsArrayPtr)
            {
                const TSharedPtr<FJsonObject> ActionObject = ActionValue->AsObject();
                if (!ActionObject.IsValid())
                {
                    bPlanValid = false;
                    break;
                }

                FParsedAction Action;
                if (!ActionObject->TryGetStringField(MindSandJsonKeys::Action::Name, Action.Name) ||
                    Action.Name.IsEmpty())
                {
                    bPlanValid = false;
                    break;
                }
                ActionObject->TryGetBoolField(MindSandJsonKeys::ActionPlanResponse::SignalEndTaskToTeammates,
                                              Action.bSignalEndTaskToTeammates);

                const TSharedPtr<FJsonObject> *ParametersObjectPtr = nullptr;
                if (ActionObject->TryGetObjectField(MindSandJsonKeys::ActionPlanResponse::Parameters,
                                                    ParametersObjectPtr) &&
                    ParametersObjectPtr != nullptr && ParametersObjectPtr->IsValid())
                {
                    Action.ParametersJson = Utils::Serialize(ParametersObjectPtr->ToSharedRef());
                }

                Plan.Actions.Add(MoveTemp(Action));
            }
        }

        if (bPlanValid)
        {
            OutPlans.Add(MoveTemp(Plan));
        }
    }

    return !OutPlans.IsEmpty();
}
