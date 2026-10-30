// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "GameplayTagDescriptionAsset.h"
#include "GameplayTagsManager.h"

FPrimaryAssetId UGameplayTagDescriptionAsset::GetPrimaryAssetId() const
{
    return FPrimaryAssetId("GameplayTagDescription", GetFName());
}

void UGameplayTagDescriptionAsset::PostLoad()
{
    Super::PostLoad();
    BuildLookupMap();
}

#if WITH_EDITOR
void UGameplayTagDescriptionAsset::PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    BuildLookupMap();
}
#endif

const FString UGameplayTagDescriptionAsset::FindDescription(FGameplayTag InTag) const
{
    const UGameplayTagsManager &Manager = UGameplayTagsManager::Get();

    // Collect parents (root -> immediate parent)
    TArray<FGameplayTag> Parents;
    FGameplayTagContainer TagContainer = Manager.RequestGameplayTagParents(InTag);

    TagContainer.GetGameplayTagArray(Parents);

    FString Result;

    for (const FGameplayTag &Tag : Parents)
    {
        if (const FString *Desc = LookupMap.Find(Tag))
        {
            if (!Result.IsEmpty())
            {
                Result += TEXT("\n");
            }
            Result += *Desc;
        }
    }

    return Result;
}

void UGameplayTagDescriptionAsset::BuildLookupMap()
{
    LookupMap.Reset();
    LookupMap.Reserve(TagDescriptions.Num());
    for (const FGameplayTagDescription &Entry : TagDescriptions)
    {
        if (Entry.Tag.IsValid())
        {
            LookupMap.Add(Entry.Tag, Entry.Description);
        }
    }
}
