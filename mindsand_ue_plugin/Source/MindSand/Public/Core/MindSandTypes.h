// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "CoreMinimal.h"
#include <vector>

#define MAX_ACTION_CHAR_PARAMETER 128

UENUM(BlueprintType)
enum class ActionParameterType : uint8
{
    APT_BOOL UMETA(DisplayName = "Bool"),
    APT_FLOAT UMETA(DisplayName = "Float"),
    APT_INTEGER UMETA(DisplayName = "Integer"),
    APT_STRING UMETA(DisplayName = "String"),
    APT_ACTOR UMETA(DisplayName = "Actor"),
};

union ParameterValue {
    AActor *ActorValue;
    char CharValue[MAX_ACTION_CHAR_PARAMETER];
    float FloatValue;
    int IntValue;
    bool BoolValue;
};
