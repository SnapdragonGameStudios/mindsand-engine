// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "UObject/NoExportTypes.h"

#include "PerceptionStateGatherer.generated.h"

class UAIPerceptionComponent;
class UAISense;
class AActor;
class FJsonObject;
class FJsonValue;

/**
 * Provides JSON-formatted perception state for any UAIPerceptionComponent.
 *
 * Public API:
 *   - GetKnownActorsJSON: returns only known actors + their stimuli
 *   - GetObjectDescriptions: returns descriptions for collected object types
 *
 * JSON from GetKnownActorsJSON (single component):
 * {
 *   "knownActors": [ "enemy_01", "enemy_02", ... ]           // when stimuli disabled
 * }
 * or (when stimuli enabled):
 * {
 *   "knownActors": [
 *     {
 *       "id": "enemy_01",
 *       "stimuli": [
 *         { "sense": "Sight", "successfullySensed": true, "age": 0.42 }
 *       ]
 *     }
 *   ]
 * }
 *
 * JSON from GetKnownActorsJSON (multi component, keyed by pair name):
 * {
 *   "knownActors": {
 *     "alice": [ "enemy_01", "enemy_02" ],
 *     "bob":   [ "enemy_02", "crate_07" ]
 *   }
 * }
 *
 * JSON from GetObjectDescriptions:
 * {
 *   "objectTypes": [
 *     { "type": "Enemy", "description": "Humanoid hostile NPC" },
 *     { "type": "Crate", "description": "Breakable wooden crate" }
 *   ]
 * }
 */
UCLASS()
class MINDSAND_API UPerceptionStateGatherer : public UObject
{
    GENERATED_BODY()

  public:
    /**
     * Returns a JSON object describing all known actors for this perception component.
     * Does NOT include object descriptions or type fields.
     *
     * OutCollectedTypes:
     *   - Persistent set of TypeIDs
     *   - Updated with any new types discovered during this call
     *
     * SenseToInclude null = no filter (all senses). Non-null = forward only actors
     * known via that sense, using UAIPerceptionComponent::GetKnownPerceivedActors.
     */
    static TSharedPtr<FJsonObject> GetKnownActorsJSON(UAIPerceptionComponent *PerceptionComp,
                                                      TSet<FString> &OutCollectedTypes,
                                                      TSubclassOf<UAISense> SenseToInclude = nullptr);

    /** Serialized string form of GetKnownActorsJSON for a single perception component. */
    UFUNCTION(BlueprintCallable, Category = "AI|Perception")
    static FString GetKnownActors(UAIPerceptionComponent *PerceptionComp, TSet<FString> &OutCollectedTypes,
                                  TSubclassOf<UAISense> SenseToInclude = nullptr);

    /**
     * Multi-component variant. Each pair contributes a top-level field on the returned
     * object, keyed by the pair's name, whose value is that component's known-actor array.
     * A null component yields an empty array under its key. Duplicate keys — last write wins.
     * OutCollectedTypes accumulates TypeIDs across all components.
     */
    static TSharedPtr<FJsonObject> GetKnownActorsJSON(
        const TArray<TPair<FString, UAIPerceptionComponent *>> &NamedComps, TSet<FString> &OutCollectedTypes,
        TSubclassOf<UAISense> SenseToInclude = nullptr);

    /** Serialized string form of the multi-component GetKnownActorsJSON. C++-only. */
    static FString GetKnownActors(const TArray<TPair<FString, UAIPerceptionComponent *>> &NamedComps,
                                  TSet<FString> &OutCollectedTypes, TSubclassOf<UAISense> SenseToInclude = nullptr);

    /**
     * Returns an array of actor ID strings for all known actors in this perception component.
     * OutCollectedTypes is updated with any new types discovered during this call.
     * OutActorStates optionally maps actor IDs to their state strings (empty if no state).
     * SenseToInclude null = no filter (all senses).
     */
    UFUNCTION(BlueprintCallable, Category = "AI|Perception")
    static TArray<FString> GetKnownActorsArray(UAIPerceptionComponent *PerceptionComp, TSet<FString> &OutCollectedTypes,
                                               TMap<FString, FString> &OutActorStates,
                                               TSubclassOf<UAISense> SenseToInclude = nullptr);

  private:
    /**
     * Builds the JSON entry for a single known actor.
     * When bIncludeStimuliInfo is false, returns a bare string (the id).
     * When true, returns an object with "id" and "stimuli" fields.
     */
    static TSharedPtr<FJsonValue> BuildKnownActorEntry(UAIPerceptionComponent *PerceptionComp, AActor *TargetActor,
                                                       TSet<FString> &OutCollectedTypes);

    /** Per-component pipeline shared by both public GetKnownActorsJSON overloads. */
    static TArray<TSharedPtr<FJsonValue>> BuildKnownActorsArray(UAIPerceptionComponent *PerceptionComp,
                                                                TSet<FString> &OutCollectedTypes,
                                                                TSubclassOf<UAISense> SenseToInclude);
};
