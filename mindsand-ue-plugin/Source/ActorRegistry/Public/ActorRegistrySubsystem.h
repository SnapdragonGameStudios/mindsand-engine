// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UObject/ObjectKey.h"

#include "ActorRegistrySubsystem.generated.h"

class UGameplayTagDescriptionAsset;

UENUM(BlueprintType)
enum class ERegistryLookup : uint8
{
    LookupOnly UMETA(DisplayName = "Lookup Only"),
    RegisterIfMissing UMETA(DisplayName = "Register If Missing"),
};

UCLASS()
class ACTORREGISTRY_API UActorRegistrySubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

  public:
    // Returns the ActorRegistry world subsystem for the world associated with WorldContextObject.
    // Returns nullptr if the object has no world or the subsystem is unavailable.
    static UActorRegistrySubsystem *Get(const UObject *WorldContextObject);

    // Action Decoder path: resolves a previously-minted ID back to its live actor.
    // Returns nullptr if the ID is unknown or the actor has been destroyed.
    UFUNCTION(BlueprintPure, Category = "ActorRegistry")
    AActor *GetActorFromID(const FString &InID) const;

    // Returns true if InActor has already been registered (has a minted ID).
    bool IsActorRegistered(const AActor *InActor) const;

    // State Encoder path: returns the ID for InActor. With RegisterIfMissing, mints and stores
    // a new ID on first sight; with LookupOnly, returns empty if unknown. When the actor
    // has a UActorDescriptorComponent with a non-empty UniqueID, the ID is
    // "<BaseName>_<UniqueID>" (e.g. "NPC_Alice"); otherwise it is auto-numbered
    // "<BaseName>_NN". Returns empty when the UniqueID collides with an existing ID.
    UFUNCTION(BlueprintCallable, Category = "ActorRegistry")
    FString GetIDFromActor(AActor *InActor, ERegistryLookup LookupMode = ERegistryLookup::LookupOnly);

    // Registers InActor under the caller-supplied InID, bypassing tag resolution and the
    // auto-numbering path. Intended for singletons like the player where a bare, stable ID
    // (e.g. "Player") is desired. If InID is already held by a different actor, that prior
    // holder is evicted (its OnDestroyed binding removed, dropped from both maps) — this
    // supports flows like player pawn possession/unpossession where the same ID moves between
    // pawns. Returns false (and logs) when InActor is invalid, InID is empty, InID contains
    // the '#' separator reserved for auto-minted IDs, or InActor is already registered under
    // a different ID. Returns true (no-op) when InActor is already registered under InID.
    // Note: GetDescriptionForActor still routes through the actor's "ID.*" tag, so
    // descriptions require the tag regardless of whether the actor was registered via this
    // path.
    UFUNCTION(BlueprintCallable, Category = "ActorRegistry")
    bool RegisterActor(AActor *InActor, const FString &InID);

    // Convenience wrapper around GetIDFromActor that also returns the actor's base name
    // (the resolved "ID.*" tag with the "ID." prefix stripped, or the class-name fallback)
    // as Type, plus the actor's dynamic state as maintained on its UActorDescriptorComponent.
    // State is empty when the actor has no descriptor component or when the actor is not
    // registry-eligible (ID is empty).
    UFUNCTION(BlueprintCallable, Category = "ActorRegistry")
    void GetInfoFromActor(AActor *InActor, FString &ID, FString &Type, FString &State,
                          ERegistryLookup LookupMode = ERegistryLookup::LookupOnly);

    // Returns the description for the actor's resolved gameplay tag from the active description asset.
    // Returns an empty string if no matching tag is found.
    UFUNCTION(BlueprintPure, Category = "ActorRegistry")
    FString GetDescriptionForActor(AActor *InActor) const;

    // Returns the actor's dynamic state as maintained on its UActorDescriptorComponent.
    // Empty when the actor has no component or has not set any state.
    UFUNCTION(BlueprintPure, Category = "ActorRegistry")
    FString GetStateForActor(AActor *InActor) const;

    // Returns the description for the given gameplay tag string from the active description asset.
    // Returns an empty string if the tag is not found or the asset is not loaded.
    UFUNCTION(BlueprintPure, Category = "ActorRegistry")
    FString GetDescriptionForTag(const FString &InTag) const;

    // Returns a serialized JSON string of descriptions for the given set of gameplay tag string from the active
    // description asset. Returns an empty string if the tag is not found or the asset is not loaded.
    UFUNCTION(BlueprintPure, Category = "ActorRegistry")
    FString GetActorDescriptions(const TSet<FString> &types) const;

    // Returns all live actors whose registry ID begins with the given category base name.
    // Category may be a full tag string ("ID.NPC") or bare base name ("NPC").
    UFUNCTION(BlueprintPure, Category = "ActorRegistry")
    TArray<AActor *> GetActorsByCategory(const FString &Category) const;

  private:
    // Forward lookup: ID → Actor
    TMap<FString, TWeakObjectPtr<AActor>> PrimaryMap;

    // Inverse lookup: Actor identity → ID
    TMap<FObjectKey, FString> InverseMap;

    // Per-name counters for generated IDs (e.g. Crate_01, PrimaryTerminal_01)
    TMap<FString, int32> NameCounters;

    UFUNCTION()
    void OnActorDestroyed(AActor *DestroyedActor);

    FString GenerateID(AActor *InActor);

    UGameplayTagDescriptionAsset *LoadDescriptionAsset() const;
    static FString ResolveBaseName(AActor *InActor);

    UPROPERTY()
    mutable UGameplayTagDescriptionAsset *CachedDescriptionAsset = nullptr;
};
