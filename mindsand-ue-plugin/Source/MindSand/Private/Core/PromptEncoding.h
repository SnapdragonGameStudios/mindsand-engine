// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "CoreMinimal.h"

class FJsonObject;

/** Functions for encoding game data into the LLM prompt's Markdown context. */
namespace MindSandPromptEncoding
{
/** Encodes the complete static/dynamic/command prompt assembled by the orchestrator. */
FString EncodePrompt(const TSharedRef<FJsonObject> &Prompt);
} // namespace MindSandPromptEncoding
