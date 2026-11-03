// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "ActorRegistryDataProvider.h"
#include "CoreMinimal.h"
#include "GameFramework/WorldSettings.h"

#include "ActorRegistryWorldSettings.generated.h"

class UGameplayTagDescriptionAsset;

UCLASS(Blueprintable)
class ACTORREGISTRY_API AActorRegistryWorldSettings : public AWorldSettings, public IActorRegistryDataProvider
{
    GENERATED_BODY()

  public:
    UPROPERTY(EditAnywhere, Category = "Actor Registry")
    TSoftObjectPtr<UGameplayTagDescriptionAsset> GameplayTagDescriptionAsset;

    // IActorRegistryDataProvider: synchronously loads and returns the asset assigned in the editor.
    virtual UGameplayTagDescriptionAsset *GetGameplayTagDescriptionAsset_Implementation() const override;
};
