// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "ActorRegistryWorldSettings.h"
#include "GameplayTagDescriptionAsset.h"

UGameplayTagDescriptionAsset *AActorRegistryWorldSettings::GetGameplayTagDescriptionAsset_Implementation() const
{
    return GameplayTagDescriptionAsset.LoadSynchronous();
}
