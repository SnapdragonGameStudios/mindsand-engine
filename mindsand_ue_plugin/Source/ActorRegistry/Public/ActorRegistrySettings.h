// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "ActorRegistrySettings.generated.h"

UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Actor Registry"))
class ACTORREGISTRY_API UActorRegistrySettings : public UDeveloperSettings
{
    GENERATED_BODY()

  public:
    // Name of the UGameplayTagDescriptionAsset data asset to search for.
    // Configurable in DefaultGame.ini under [/Script/ActorRegistry.ActorRegistrySettings].
    UPROPERTY(config, EditAnywhere, Category = "Gameplay Tag Descriptions")
    TSoftObjectPtr<UGameplayTagDescriptionAsset> GameplayTagDescriptionAsset =
        TSoftObjectPtr<UGameplayTagDescriptionAsset>(
            FSoftObjectPath(TEXT("/Game/GameplayTagDescriptions.GameplayTagDescriptions")));

    // When true, the ResolveBaseName lookup only considers component-provided tags
    // (via IGameplayTagAssetInterface). The AActor::Tags "ID.*" fallback is skipped,
    // so only actors with a UActorDescriptorComponent (or another IGameplayTagAssetInterface
    // component that exposes an ID.* tag) are eligible for registration.
    UPROPERTY(config, EditAnywhere, Category = "Registration")
    bool bRequireGameplayTagComponent = false;
};
