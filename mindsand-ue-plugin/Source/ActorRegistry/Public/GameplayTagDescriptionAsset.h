// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "GameplayTagDescriptionAsset.generated.h"

USTRUCT(BlueprintType)
struct ACTORREGISTRY_API FGameplayTagDescription
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    FGameplayTag Tag;

    UPROPERTY(EditAnywhere)
    FString Description;
};

UCLASS(BlueprintType)
class ACTORREGISTRY_API UGameplayTagDescriptionAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

  public:
    UPROPERTY(EditAnywhere)
    TArray<FGameplayTagDescription> TagDescriptions;

    // Primary asset id used by the asset manager; type is "GameplayTagDescription".
    virtual FPrimaryAssetId GetPrimaryAssetId() const override;

    // Rebuilds the internal tag -> description lookup map after the asset is loaded.
    virtual void PostLoad() override;
#if WITH_EDITOR
    // Rebuilds the lookup map when the asset is edited in the editor.
    virtual void PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent) override;
#endif

    // Returns the description for InTag, concatenating descriptions of every ancestor tag
    // (root -> immediate parent) found in this asset, separated by newlines. Empty if no match.
    const FString FindDescription(FGameplayTag InTag) const;

  private:
    void BuildLookupMap();

    TMap<FGameplayTag, FString> LookupMap;
};
