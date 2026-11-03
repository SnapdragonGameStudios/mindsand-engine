// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "ActionParameterReflection.h"
#include "Actions/ActionDescriptionAsset.h"
#include "Json.h"
#include "MindSandLog.h"

void ReflectParametersJson(UObject *Object, const TArray<FActionParameters> &ActionParameters,
                           const FString &ParametersJson, const FActionParameterValueTranslator &ValueTranslator)
{
    // Plans may omit parameters; empty input leaves the action's defaults unchanged.
    if (!Object || ParametersJson.IsEmpty())
    {
        return;
    }

    TSharedPtr<FJsonObject> Parsed;
    const TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ParametersJson);
    if (!FJsonSerializer::Deserialize(JsonReader, Parsed) || !Parsed.IsValid())
    {
        UE_LOG(LogMindSand, Warning, TEXT("ActionParameterReflection: %s parameters are not a JSON object: %s"),
               *Object->GetName(), *ParametersJson);
        return;
    }

    // Process supplied keys only; omitted parameters keep their compiled-in defaults.
    for (const TPair<FString, TSharedPtr<FJsonValue>> &Entry : Parsed->Values)
    {
        // This allowlist contains only the action class's own reflected parameters, keeping inherited properties
        // such as UGameplayTask::Priority out of reach. FString comparison is case-insensitive, matching the
        // FName lookup below.
        const bool bIsDeclaredParameter = ActionParameters.ContainsByPredicate(
            [&Entry](const FActionParameters &Declared) { return Declared.Name == Entry.Key; });

        // AdditionalParameters are handled by ProcessParameters overrides, not reflection.
        if (!bIsDeclaredParameter || !Entry.Value.IsValid() || Entry.Value->IsNull())
        {
            continue;
        }

        // Use the current property type for conversion; the asset's serialized Type is only the model-facing
        // schema and may be stale after a C++ property change.
        FProperty *Property = Object->GetClass()->FindPropertyByName(FName(*Entry.Key));
        if (!Property)
        {
            UE_LOG(LogMindSand, Warning,
                   TEXT("ActionParameterReflection: %s declares parameter '%s' but has no property of that name; the "
                        "action asset is out of date."),
                   *Object->GetClass()->GetName(), *Entry.Key);
            continue;
        }

        // Let the converter handle scalar and array values so the translator also applies to array elements.
        // Keep strict mode and property flags disabled: the allowlist is the write boundary, while model output
        // may contain unrelated keys or variable-length arrays.
        FText FailReason;
        if (!FJsonObjectConverter::JsonValueToUProperty(Entry.Value, Property,
                                                        Property->ContainerPtrToValuePtr<void>(Object), 0, 0, false,
                                                        &FailReason, &ValueTranslator))
        {
            UE_LOG(LogMindSand, Warning, TEXT("ActionParameterReflection: could not set parameter '%s' on %s: %s"),
                   *Entry.Key, *Object->GetName(), *FailReason.ToString());
        }
    }
}
