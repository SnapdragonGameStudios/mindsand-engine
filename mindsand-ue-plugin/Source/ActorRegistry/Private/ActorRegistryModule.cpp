// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "ActorRegistryModule.h"

class FActorRegistryModule : public IActorRegistryModule
{
  public:
    virtual void StartupModule() override
    {
    }
    virtual void ShutdownModule() override
    {
    }
};

IMPLEMENT_MODULE(FActorRegistryModule, ActorRegistry)
