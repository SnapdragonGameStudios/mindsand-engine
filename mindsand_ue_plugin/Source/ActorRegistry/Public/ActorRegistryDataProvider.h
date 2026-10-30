// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "ActorRegistryDataProvider.generated.h"

class UGameplayTagDescriptionAsset;

UINTERFACE(BlueprintType, MinimalAPI)
class UActorRegistryDataProvider : public UInterface
{
    GENERATED_BODY()
};

class ACTORREGISTRY_API IActorRegistryDataProvider
{
    GENERATED_BODY()

  public:
    // Return the description asset to use in this world.
    // Return nullptr to defer to the project-wide UActorRegistrySettings fallback.
    UFUNCTION(BlueprintNativeEvent, Category = "Actor Registry")
    UGameplayTagDescriptionAsset *GetGameplayTagDescriptionAsset() const;
};
