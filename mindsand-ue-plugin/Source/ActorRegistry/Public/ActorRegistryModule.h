// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

class ACTORREGISTRY_API IActorRegistryModule : public IModuleInterface
{
  public:
    // Loads the ActorRegistry module if needed and returns its singleton interface.
    static inline IActorRegistryModule &Get()
    {
        return FModuleManager::LoadModuleChecked<IActorRegistryModule>("ActorRegistry");
    }

    // Returns true if the ActorRegistry module is currently loaded.
    static inline bool IsAvailable()
    {
        return FModuleManager::Get().IsModuleLoaded("ActorRegistry");
    }
};
