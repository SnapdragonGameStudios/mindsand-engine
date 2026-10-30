// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"

#include "TeammateCharacterAgentDescriptor.generated.h"

/**
 * The authored description of a teammate: who they are and what they can do out of the box.
 *
 * Doubles as the controller-side cache: UTeammateCharacterAgentComponent::Desc is a copy of this
 * struct, refreshed on every RegisterTeammate(). On that side Actions is the live action set, so it
 * is rebuilt element-by-element through RegisterAction (which validates each name against the action
 * registry and registers the action's plan state variables) rather than left as the raw copy.
 */
USTRUCT(BlueprintType)
struct MINDSAND_API FTeammateDesc
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MindSand|Teammate")
    FString Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MindSand|Teammate")
    FString Squad;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MindSand|Teammate")
    FString Personality;

    // Action names seeded into UTeammateCharacterAgentComponent::Desc.Actions on registration.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MindSand|Teammate")
    TArray<FString> Actions;
};

/**
 * Pawn-side authoring component that describes a teammate. Add it to a Character (or any Pawn)
 * and fill in the fields; the controller-side UTeammateCharacterAgentComponent reads them at
 * possession time via its RegisterTeammate() function and caches them.
 *
 * Data only -- this component never talks to UTeammateRegistry itself. Registration stays the
 * controller side's job so the registry keeps holding controller-side components.
 */
UCLASS(Blueprintable, BlueprintType, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent),
       hidecategories = (Object, LOD, Lighting, Transform, Sockets, TextureStreaming))
class MINDSAND_API UTeammateCharacterAgentDescriptor : public UActorComponent
{
    GENERATED_BODY()

  public:
    UTeammateCharacterAgentDescriptor();

    // ShowOnlyInnerProperties flattens the struct in the details panel, so authors edit Name/Squad/
    // Personality/Actions directly instead of expanding a nested "Desc" row.
    //
    // BlueprintReadOnly rather than BlueprintReadWrite: Desc.Name is mirrored into the owning actor's
    // UActorDescriptorComponent::UniqueID, and a Blueprint assigning the struct wholesale would desync
    // the two. Blueprints write through the setters below -- SetDesc for a wholesale replacement. The
    // details panel still edits it directly, and PostEditChangeProperty keeps the mirror honest there.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MindSand|Teammate", meta = (ShowOnlyInnerProperties))
    FTeammateDesc Desc;

    // When true, the controller-side UTeammateCharacterAgentComponent registers this teammate with
    // UTeammateRegistry as soon as the owning pawn is possessed. When false, nothing is cached and
    // nothing is registered until something calls
    // UTeammateCharacterAgentComponent::RegisterTeammate().
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MindSand|Teammate")
    bool bRegisterOnPossess = true;

    // BlueprintPure rather than BlueprintCallable: these are side-effect-free reads, so they need no exec
    // pins. Matches UActorDescriptorComponent::GetUniqueID, the component this class is modelled on.
    // (UTeammateCharacterAgentComponent's own getters are BlueprintCallable -- older code, not a pattern
    // to copy.)
    //
    // All four return by value, like the controller-side getters, so the struct's layout stays an
    // implementation detail.

    // Also the owning actor's UActorDescriptorComponent::UniqueID -- see SetTeammateName.
    UFUNCTION(BlueprintPure, Category = "MindSand|Teammate")
    FString GetTeammateName() const;

    // The squad the controller-side component will register under on its next RegisterTeammate(), which
    // is not necessarily the key it currently sits at in UTeammateRegistry. For the live key, ask the
    // controller-side UTeammateCharacterAgentComponent::GetCharacterSquad().
    UFUNCTION(BlueprintPure, Category = "MindSand|Teammate")
    FString GetSquad() const;

    UFUNCTION(BlueprintPure, Category = "MindSand|Teammate")
    FString GetPersonality() const;

    // The *authored* action set. The live set is the controller-side
    // UTeammateCharacterAgentComponent::GetAvailableActions(), which starts as a validated copy of this
    // and then diverges as RegisterActions / UnRegisterActions mutate it.
    UFUNCTION(BlueprintPure, Category = "MindSand|Teammate")
    TArray<FString> GetActions() const;

    /**
     * Sets the teammate's name and mirrors it into the owning actor's UActorDescriptorComponent as its
     * UniqueID, so the id the ActorRegistry mints (<IDTag base name>_<UniqueID>) always derives from the
     * same string the LLM is shown as the teammate's name.
     *
     * Returns whether the UniqueID was written. Desc.Name is updated either way; false means the mirror
     * did not happen -- the name is empty, the owner has no UActorDescriptorComponent, or the
     * ActorRegistry has already minted an id for the owner (UActorDescriptorComponent::SetUniqueID only
     * takes effect before the first mint, and warns when called after).
     *
     * Does NOT touch an already-registered teammate: the controller-side cache and the UTeammateRegistry
     * squad key are only refreshed by UTeammateCharacterAgentComponent::RegisterTeammate().
     */
    UFUNCTION(BlueprintCallable, Category = "MindSand|Teammate")
    bool SetTeammateName(const FString &NewName);

    // Takes effect on the next UTeammateCharacterAgentComponent::RegisterTeammate() -- on its own this
    // does not move an already-registered teammate between squads.
    UFUNCTION(BlueprintCallable, Category = "MindSand|Teammate")
    void SetSquad(const FString &NewSquad);

    UFUNCTION(BlueprintCallable, Category = "MindSand|Teammate")
    void SetPersonality(const FString &NewPersonality);

    // Replaces the authored action set. Same caveat as SetSquad: the controller-side live action set is
    // only reseeded from here by RegisterTeammate(). To grant an action to a running teammate, use
    // UTeammateCharacterAgentComponent::RegisterActions instead (and expect a later RegisterTeammate()
    // to discard it).
    UFUNCTION(BlueprintCallable, Category = "MindSand|Teammate")
    void SetActions(const TArray<FString> &NewActions);

    /**
     * Replaces the whole authored description in one go -- the sanctioned way to assign the struct, and
     * the reason Desc itself is only BlueprintReadOnly.
     *
     * Returns the same thing SetTeammateName does: whether NewDesc.Name reached the owning actor's
     * UActorDescriptorComponent as its UniqueID. Desc is replaced either way, so a NewDesc with an empty
     * Name leaves whatever UniqueID was previously mirrored in place rather than clearing it.
     *
     * Squad / Personality / Actions carry the same caveat as their individual setters: they only describe
     * what the next UTeammateCharacterAgentComponent::RegisterTeammate() will pick up. Replacing the
     * struct does not move an already-registered teammate between squads or change its live action set.
     */
    UFUNCTION(BlueprintCallable, Category = "MindSand|Teammate")
    bool SetDesc(const FTeammateDesc &NewDesc);

  protected:
    // Applies the authored Desc.Name to the owner's UActorDescriptorComponent. Runtime is the authority
    // for the mirror: PostEditChangeProperty below only reaches a level-placed instance, never a
    // component template being edited inside the Blueprint editor.
    virtual void BeginPlay() override;

#if WITH_EDITOR
    // Re-mirrors Desc.Name into the owner's UActorDescriptorComponent when it is edited in the details
    // panel of a placed actor.
    virtual void PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent) override;
#endif

  private:
    // Pushes Desc.Name into the owning actor's UActorDescriptorComponent::UniqueID. Returns false and
    // leaves UniqueID untouched when Desc.Name is empty (clearing the name deliberately does not clear
    // an authored UniqueID), when there is no owning actor, or when the owner carries no
    // UActorDescriptorComponent.
    bool SyncUniqueIDToActorDescriptor();
};
