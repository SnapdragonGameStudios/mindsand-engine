// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "CoreMinimal.h"

class FJsonObject;
class UWorld;

/** Collects the static and dynamic game state used in an LLM prompt. */
class FPromptContextGatherer
{
  public:
    static TSharedRef<FJsonObject> GetStaticGameInfo(UWorld *World, const FString &TeamName);
    static TSharedRef<FJsonObject> GetDynamicGameInfo(UWorld *World, const FString &TeamName);
};
