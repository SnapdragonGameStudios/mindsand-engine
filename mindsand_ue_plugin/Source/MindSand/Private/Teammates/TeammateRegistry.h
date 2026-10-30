// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UObject/WeakObjectPtr.h"

#include "TeammateRegistry.generated.h"

class UTeammateCharacterAgentComponent;

/**
 * Neutral registrar that decouples teammate and planner lifetimes. Teammates register
 * here keyed by squad string; planners pull the current teammate list on demand. Pull-only
 * by design — no broadcasts.
 */
UCLASS()
class MINDSAND_API UTeammateRegistry : public UWorldSubsystem
{
    GENERATED_BODY()

  public:
    static UTeammateRegistry *Get(const UObject *WorldContextObject);

    void RegisterTeammate(const FString &Squad, UTeammateCharacterAgentComponent *Teammate);
    void UnregisterTeammate(UTeammateCharacterAgentComponent *Teammate);
    TArray<UTeammateCharacterAgentComponent *> GetTeammatesForSquad(const FString &Squad);
    TArray<UTeammateCharacterAgentComponent *> GetAllTeammates();
    UTeammateCharacterAgentComponent *GetRandomTeammateForSquad(const FString &Squad);
    UTeammateCharacterAgentComponent *GetRandomTeammate();
    UTeammateCharacterAgentComponent *GetTeammateByName(const FString &TeammateName);
    UTeammateCharacterAgentComponent *GetTeammateById(const FString &TeammateId);
    UTeammateCharacterAgentComponent *GetTeammate(const FString &NameOrId);

  private:
    TMap<FString, TArray<TWeakObjectPtr<UTeammateCharacterAgentComponent>>> TeammatesBySquad;
};
