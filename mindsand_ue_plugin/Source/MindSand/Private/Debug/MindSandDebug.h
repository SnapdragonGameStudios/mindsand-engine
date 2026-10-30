// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "CoreMinimal.h"

// Debug dump of LLM request/response text, gated by the MindSand.DumpJson cvar (default off).
// When enabled, writes <Date>_<TurnId>_<Kind>.json under Saved/Logs/MindSand/<Date>/, where <Date>
// is generated at call time. Kind is expected to be "request" or "response".
void DumpMindSandJson(const FString &TurnId, const TCHAR *Kind, const FString &Json);
