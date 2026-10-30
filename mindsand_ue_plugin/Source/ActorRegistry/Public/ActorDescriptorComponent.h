// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "GameplayTagAssetInterface.h"
#include "GameplayTagContainer.h"

#include "ActorDescriptorComponent.generated.h"

UCLASS(ClassGroup = (ActorRegistry), meta = (BlueprintSpawnableComponent),
       hidecategories = (Object, LOD, Lighting, Transform, Sockets, TextureStreaming))
class ACTORREGISTRY_API UActorDescriptorComponent : public UActorComponent, public IGameplayTagAssetInterface
{
    GENERATED_BODY()

  public:
    UActorDescriptorComponent();

    // The gameplay tag identifying this actor. Authors set a tag under the "ID.*" root
    // (the parent the registry resolver looks for). The tag picker is filtered to the ID subtree.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actor Registry", meta = (Categories = "ID"))
    FGameplayTag IDTag;

    // Optional caller-supplied unique part. When non-empty, the registry uses
    // "<BaseName>_<UniqueID>" as the ID (e.g. "NPC_Alice") instead of the
    // auto-numbered "<BaseName>_NN". Empty (default) = auto behavior.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actor Registry")
    FString UniqueID;

    // Sets the UniqueID for this actor. Must be called before the registry mints an ID
    // for the owner (i.e. before the first GetIDFromActor(..., RegisterIfMissing)).
    // Returns false and logs a warning when the owner is already registered — in that
    // case the field is not written and the stored ID is unchanged.
    UFUNCTION(BlueprintCallable, Category = "Actor Registry")
    bool SetUniqueID(const FString &InUniqueID);

    UFUNCTION(BlueprintPure, Category = "Actor Registry")
    const FString &GetUniqueID() const
    {
        return UniqueID;
    }

    UFUNCTION(BlueprintCallable, Category = "Actor Registry")
    void SetState(const FString &InState);

    UFUNCTION(BlueprintPure, Category = "Actor Registry")
    const FString &GetState() const
    {
        return State;
    }

    // IGameplayTagAssetInterface: yields the configured tag (if valid) so the registry's
    // existing component-walk in ResolveActorTag picks it up.
    virtual void GetOwnedGameplayTags(FGameplayTagContainer &TagContainer) const override;

  private:
    // Per-instance dynamic state. Transient by design: state is runtime-only and
    // should not bake into the level asset.
    UPROPERTY(Transient)
    FString State;
};
