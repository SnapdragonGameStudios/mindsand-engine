// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "MindSandDebugUtils.generated.h"

UCLASS()
class MINDSAND_API UMindSandDebugUtils : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

  public:
    // Feeds ResponseJson into the orchestrator as if the LLM had produced it, exercising the parser
    // and plan-dispatch path without a live model. Returns false when the JSON is malformed, contains
    // no valid plans, or references a teammate that is not registered.
    UFUNCTION(BlueprintCallable, Category = "MindSand|Debug")
    static bool ExecutePlan(const UObject *WorldContextObject, const FString &ResponseJson);

    // Walks every registered teammate's perception component and serializes its known actors,
    // mirroring the snapshot the orchestrator takes when building an LLM prompt. Runs for side
    // effects only (registers newly-observed actors in the ActorRegistrySubsystem and collects
    // their type descriptions) -- the JSON output is discarded.
    UFUNCTION(BlueprintCallable, Category = "MindSand|Debug")
    static void UpdatePerception(const UObject *WorldContextObject);
};
