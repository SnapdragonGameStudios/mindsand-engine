// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "CoreMinimal.h"
#include "JsonObjectConverter.h"

struct FActionParameters;

// Consulted before every property at every recursion depth - array and set elements, map keys and values,
// and struct members all pass through it. Return true to claim a value (FJsonObjectConverter then skips its
// own handling), false to let the converter import it. May be unbound.
using FActionParameterValueTranslator = FJsonObjectConverter::CustomImportCallback;

// Imports the values in ParametersJson onto Object's UPROPERTYs, writing only properties named by
// ActionParameters. All type dispatch belongs to FJsonObjectConverter; ValueTranslator exists for value
// representations it cannot resolve on its own.
void ReflectParametersJson(UObject *Object, const TArray<FActionParameters> &ActionParameters,
                           const FString &ParametersJson, const FActionParameterValueTranslator &ValueTranslator);
