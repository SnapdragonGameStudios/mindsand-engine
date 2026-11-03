// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "Modules/ModuleManager.h"

class FLatencyProfilerModule : public IModuleInterface
{
};

IMPLEMENT_MODULE(FLatencyProfilerModule, LatencyProfiler)
