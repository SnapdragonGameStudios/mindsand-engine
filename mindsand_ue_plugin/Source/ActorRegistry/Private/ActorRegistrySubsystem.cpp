// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "ActorRegistrySubsystem.h"
#include "ActorDescriptorComponent.h"
#include "ActorRegistryDataProvider.h"
#include "ActorRegistrySettings.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/WorldSettings.h"
#include "GameplayTagAssetInterface.h"
#include "GameplayTagDescriptionAsset.h"
#include "GameplayTagsManager.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogActorRegistry, Log, All);

UActorRegistrySubsystem *UActorRegistrySubsystem::Get(const UObject *WorldContextObject)
{
    if (!WorldContextObject)
        return nullptr;
    const UWorld *World =
        GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    return World ? World->GetSubsystem<UActorRegistrySubsystem>() : nullptr;
}

namespace
{
namespace Constants
{
constexpr const char kSeparator = '#';
}
namespace Utils
{
bool CollectGameplayTagsFromComponents(AActor *Actor, FGameplayTag &OutTag)
{
    if (!Actor)
        return false;

    const FGameplayTag IDParent = FGameplayTag::RequestGameplayTag(FName("ID"), false);
    static const UClass *TagInterfaceClass = UGameplayTagAssetInterface::StaticClass();

    for (UActorComponent *Component : Actor->GetComponents())
    {
        if (!Component)
            continue;

        if (Component->GetClass()->ImplementsInterface(TagInterfaceClass))
        {
            IGameplayTagAssetInterface *TagInterface = Cast<IGameplayTagAssetInterface>(Component);
            if (TagInterface)
            {
                if (IDParent.IsValid())
                {
                    FGameplayTagContainer OwnedTags;
                    TagInterface->GetOwnedGameplayTags(OwnedTags);

                    for (const FGameplayTag &Tag : OwnedTags)
                    {
                        if (Tag.MatchesTag(IDParent))
                        {
                            OutTag = Tag;
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

} // namespace Utils
} // namespace
AActor *UActorRegistrySubsystem::GetActorFromID(const FString &InID) const
{
    const TWeakObjectPtr<AActor> *WeakPtr = PrimaryMap.Find(InID);
    if (WeakPtr && WeakPtr->IsValid())
    {
        return WeakPtr->Get();
    }
    return nullptr;
}

bool UActorRegistrySubsystem::IsActorRegistered(const AActor *InActor) const
{
    return IsValid(InActor) && InverseMap.Contains(FObjectKey(InActor));
}

FString UActorRegistrySubsystem::GetIDFromActor(AActor *InActor, ERegistryLookup LookupMode)
{
    FString Result = TEXT("");

    if (!IsValid(InActor))
    {
        return Result;
    }

    const FObjectKey Key(InActor);
    const FString *ExistingID = InverseMap.Find(Key);

    if (ExistingID)
    {
        Result = *ExistingID;
    }
    else if (LookupMode == ERegistryLookup::RegisterIfMissing)
    {
        Result = GenerateID(InActor);
        if (Result.IsEmpty())
        {
            return Result;
        }
        PrimaryMap.Add(Result, TWeakObjectPtr<AActor>(InActor));
        InverseMap.Add(Key, Result);
        InActor->OnDestroyed.AddDynamic(this, &UActorRegistrySubsystem::OnActorDestroyed);
    }

    return Result;
}

void UActorRegistrySubsystem::GetInfoFromActor(AActor *InActor, FString &ID, FString &Type, FString &State,
                                               ERegistryLookup LookupMode)
{
    ID = GetIDFromActor(InActor, LookupMode);
    int32 Index;
    if (ID.FindLastChar(Constants::kSeparator, Index))
    {
        Type = ID.Left(Index);
    }
    else
    {
        Type = ID;
    }

    State = ID.IsEmpty() ? FString() : GetStateForActor(InActor);
}

bool UActorRegistrySubsystem::RegisterActor(AActor *InActor, const FString &InID)
{
    if (!IsValid(InActor))
    {
        UE_LOG(LogActorRegistry, Warning, TEXT("ActorRegistry: RegisterActor called with invalid actor."));
        return false;
    }
    if (InID.IsEmpty())
    {
        UE_LOG(LogActorRegistry, Warning, TEXT("ActorRegistry: RegisterActor called with empty ID on actor '%s'."),
               *GetNameSafe(InActor));
        return false;
    }
    if (InID.Contains(FString::Chr(Constants::kSeparator)))
    {
        UE_LOG(LogActorRegistry, Warning,
               TEXT("ActorRegistry: RegisterActor ID '%s' contains reserved separator '%c'; rejected."), *InID,
               Constants::kSeparator);
        return false;
    }

    const FObjectKey Key(InActor);
    if (const FString *ExistingID = InverseMap.Find(Key))
    {
        if (*ExistingID == InID)
        {
            return true; // idempotent: same actor, same ID
        }
        UE_LOG(LogActorRegistry, Warning,
               TEXT("ActorRegistry: actor '%s' is already registered as '%s'; cannot re-register as '%s'."),
               *GetNameSafe(InActor), **ExistingID, *InID);
        return false;
    }

    // Evict prior holder of InID (e.g. previously-possessed pawn) so the ID moves cleanly.
    if (const TWeakObjectPtr<AActor> *PriorWeak = PrimaryMap.Find(InID))
    {
        if (AActor *PriorActor = PriorWeak->Get())
        {
            PriorActor->OnDestroyed.RemoveDynamic(this, &UActorRegistrySubsystem::OnActorDestroyed);
            InverseMap.Remove(FObjectKey(PriorActor));
        }
        PrimaryMap.Remove(InID);
    }

    PrimaryMap.Add(InID, TWeakObjectPtr<AActor>(InActor));
    InverseMap.Add(Key, InID);
    InActor->OnDestroyed.AddDynamic(this, &UActorRegistrySubsystem::OnActorDestroyed);
    return true;
}

void UActorRegistrySubsystem::OnActorDestroyed(AActor *DestroyedActor)
{
    if (!DestroyedActor)
    {
        return;
    }

    const FObjectKey Key(DestroyedActor);
    if (const FString *ID = InverseMap.Find(Key))
    {
        PrimaryMap.Remove(*ID);
        InverseMap.Remove(Key);
    }
}

FString UActorRegistrySubsystem::ResolveBaseName(AActor *InActor)
{
    FString TagString;

    { // 1. IGameplayTagAssetInterface: find first tag under "ID.*"
        FGameplayTag Tag;
        if (Utils::CollectGameplayTagsFromComponents(InActor, Tag))
        {
            TagString = Tag.ToString();
        }
    }

    if (TagString.IsEmpty() && !GetDefault<UActorRegistrySettings>()->bRequireGameplayTagComponent)
    { // 2. AActor::Tags: find first tag prefixed with "ID."
        for (const FName &Tag : InActor->Tags)
        {
            const FString Candidate = Tag.ToString();
            if (Candidate.StartsWith(TEXT("ID.")))
            {
                TagString = Candidate;
                break;
            }
        }
    }

    if (TagString.IsEmpty())
    {
        return FString(); // no ID.* tag → actor is not registry-eligible
    }

    FString BaseName;
    if (!TagString.Split(TEXT("ID."), nullptr, &BaseName, ESearchCase::IgnoreCase, ESearchDir::FromStart))
    {
        BaseName = TagString;
    }
    return BaseName;
}

FString UActorRegistrySubsystem::GenerateID(AActor *InActor)
{
    const FString BaseName = ResolveBaseName(InActor);
    if (BaseName.IsEmpty())
    {
        return FString(); // opt-out: actor has no ID.* tag
    }

    FString UniquePart;
    if (const UActorDescriptorComponent *Descriptor = InActor->FindComponentByClass<UActorDescriptorComponent>())
    {
        UniquePart = Descriptor->GetUniqueID();
    }

    const bool bHasOverride = !UniquePart.IsEmpty();
    if (!bHasOverride)
    {
        int32 &Counter = NameCounters.FindOrAdd(BaseName, 0);
        Counter++;
        UniquePart = FString::Printf(TEXT("%02d"), Counter);
    }

    const FString Candidate = FString::Printf(TEXT("%s%c%s"), *BaseName, Constants::kSeparator, *UniquePart);

    if (bHasOverride && PrimaryMap.Contains(Candidate))
    {
        UE_LOG(LogActorRegistry, Warning,
               TEXT("ActorRegistry: override ID '%s' already registered; rejecting duplicate on actor '%s'."),
               *Candidate, *GetNameSafe(InActor));
        return FString();
    }
    return Candidate;
}

UGameplayTagDescriptionAsset *UActorRegistrySubsystem::LoadDescriptionAsset() const
{
    if (CachedDescriptionAsset != nullptr)
    {
        return CachedDescriptionAsset;
    }

    if (const UWorld *World = GetWorld())
    {
        if (AWorldSettings *WorldSettings = World->GetWorldSettings())
        {
            if (WorldSettings->GetClass()->ImplementsInterface(UActorRegistryDataProvider::StaticClass()))
            {
                if (UGameplayTagDescriptionAsset *Asset =
                        IActorRegistryDataProvider::Execute_GetGameplayTagDescriptionAsset(WorldSettings))
                {
                    CachedDescriptionAsset = Asset;
                    return CachedDescriptionAsset;
                }
            }
        }
    }

    const UActorRegistrySettings *Settings = GetDefault<UActorRegistrySettings>();

    CachedDescriptionAsset = Settings->GameplayTagDescriptionAsset.LoadSynchronous();

    return CachedDescriptionAsset;
}

FString UActorRegistrySubsystem::GetDescriptionForTag(const FString &InTag) const
{
    const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*(TEXT("ID.") + InTag)), false);
    if (!Tag.IsValid())
    {
        return FString();
    }

    UGameplayTagDescriptionAsset *Asset = LoadDescriptionAsset();
    if (!Asset)
    {
        return FString();
    }

    return Asset->FindDescription(Tag);
}

FString UActorRegistrySubsystem::GetDescriptionForActor(AActor *InActor) const
{
    if (!IsValid(InActor))
    {
        return FString();
    }

    return GetDescriptionForTag(ResolveBaseName(InActor));
}

FString UActorRegistrySubsystem::GetStateForActor(AActor *InActor) const
{
    if (!IsValid(InActor))
    {
        return FString();
    }

    if (const UActorDescriptorComponent *Descriptor = InActor->FindComponentByClass<UActorDescriptorComponent>())
    {
        return Descriptor->GetState();
    }

    return FString();
}

TArray<AActor *> UActorRegistrySubsystem::GetActorsByCategory(const FString &Category) const
{
    TArray<AActor *> Result;
    FString BaseName = Category;
    if (BaseName.StartsWith(TEXT("ID.")))
        BaseName = BaseName.Mid(3);
    for (const auto &Pair : PrimaryMap)
    {
        if (Pair.Key.StartsWith(BaseName))
            if (AActor *Actor = Pair.Value.Get())
                Result.Add(Actor);
    }
    return Result;
}

FString UActorRegistrySubsystem::GetActorDescriptions(const TSet<FString> &CollectedTypes) const
{
    FString Output = TEXT("{}");

    {
        // Create array to hold entries
        TArray<TSharedPtr<FJsonValue>> TypesArray;

        for (const FString &TypeID : CollectedTypes)
        {
            const FString Description = GetDescriptionForTag(TypeID);
            if (!Description.IsEmpty())
            {
                // Build JSON object for each entry
                TSharedPtr<FJsonObject> EntryObj = MakeShared<FJsonObject>();
                EntryObj->SetStringField(TEXT("type"), TypeID);
                EntryObj->SetStringField(TEXT("description"), Description);

                // Add to array
                TypesArray.Add(MakeShared<FJsonValueObject>(EntryObj));
            }
        }

        // Wrap in root object
        TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
        Root->SetArrayField(TEXT("objectTypes"), TypesArray);

        // Serialize
        TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
            TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Output);
        FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
    }

    return Output;
}
