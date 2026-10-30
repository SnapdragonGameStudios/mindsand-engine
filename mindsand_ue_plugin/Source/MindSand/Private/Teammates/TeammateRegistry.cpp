// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "TeammateRegistry.h"

#include "ActorRegistrySubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "MindSandLog.h"
#include "Teammates/TeammateCharacterAgentComponent.h"

UTeammateRegistry *UTeammateRegistry::Get(const UObject *WorldContextObject)
{
    if (WorldContextObject == nullptr)
    {
        return nullptr;
    }
    UWorld *World = WorldContextObject->GetWorld();
    return World ? World->GetSubsystem<UTeammateRegistry>() : nullptr;
}

void UTeammateRegistry::RegisterTeammate(const FString &Squad, UTeammateCharacterAgentComponent *Teammate)
{
    if (Teammate == nullptr)
    {
        return;
    }
    UE_LOG(LogMindSand, Log, TEXT("Registering [%s] in Squad [%s]"), *(Teammate->GetCharacterName()), *Squad);
    if (Teammate->GetCharacterName().IsEmpty())
    {
        UE_LOG(LogMindSand, Warning, TEXT("Teammate Name is Empty!!"));
    }
    TeammatesBySquad.FindOrAdd(Squad).AddUnique(Teammate);
}

void UTeammateRegistry::UnregisterTeammate(UTeammateCharacterAgentComponent *Teammate)
{
    if (Teammate == nullptr)
    {
        return;
    }
    for (auto It = TeammatesBySquad.CreateIterator(); It; ++It)
    {
        It.Value().RemoveAll([Teammate](const TWeakObjectPtr<UTeammateCharacterAgentComponent> &Entry) {
            return !Entry.IsValid() || Entry.Get() == Teammate;
        });
        if (It.Value().Num() == 0)
        {
            It.RemoveCurrent();
        }
    }
}

TArray<UTeammateCharacterAgentComponent *> UTeammateRegistry::GetTeammatesForSquad(const FString &Squad)
{
    TArray<UTeammateCharacterAgentComponent *> Result;
    TArray<TWeakObjectPtr<UTeammateCharacterAgentComponent>> *Entries = TeammatesBySquad.Find(Squad);
    if (Entries == nullptr)
    {
        UE_LOG(LogMindSand, Warning, TEXT("No teammates for Squad %s"), *Squad);
        return Result;
    }

    for (int32 Index = Entries->Num() - 1; Index >= 0; --Index)
    {
        UTeammateCharacterAgentComponent *Resolved = (*Entries)[Index].Get();
        if (Resolved == nullptr)
        {
            Entries->RemoveAtSwap(Index);
        }
    }

    Result.Reserve(Entries->Num());
    for (const TWeakObjectPtr<UTeammateCharacterAgentComponent> &Entry : *Entries)
    {
        Result.Add(Entry.Get());
    }

    UE_LOG(LogMindSand, Log, TEXT("GetTeammatesForSquad - %s - %d"), *Squad, Result.Num());
    return Result;
}

UTeammateCharacterAgentComponent *UTeammateRegistry::GetRandomTeammateForSquad(const FString &Squad)
{
    TArray<UTeammateCharacterAgentComponent *> Teammates = GetTeammatesForSquad(Squad);
    if (Teammates.Num() == 0)
    {
        return nullptr;
    }
    return Teammates[FMath::RandRange(0, Teammates.Num() - 1)];
}

TArray<UTeammateCharacterAgentComponent *> UTeammateRegistry::GetAllTeammates()
{
    TArray<UTeammateCharacterAgentComponent *> All;
    for (auto It = TeammatesBySquad.CreateIterator(); It; ++It)
    {
        TArray<TWeakObjectPtr<UTeammateCharacterAgentComponent>> &Entries = It.Value();
        for (int32 Index = Entries.Num() - 1; Index >= 0; --Index)
        {
            if (Entries[Index].Get() == nullptr)
            {
                Entries.RemoveAtSwap(Index);
            }
        }
        All.Reserve(All.Num() + Entries.Num());
        for (const TWeakObjectPtr<UTeammateCharacterAgentComponent> &Entry : Entries)
        {
            All.Add(Entry.Get());
        }
    }
    return All;
}

UTeammateCharacterAgentComponent *UTeammateRegistry::GetRandomTeammate()
{
    TArray<UTeammateCharacterAgentComponent *> All = GetAllTeammates();
    if (All.Num() == 0)
    {
        return nullptr;
    }
    return All[FMath::RandRange(0, All.Num() - 1)];
}

UTeammateCharacterAgentComponent *UTeammateRegistry::GetTeammateByName(const FString &TeammateName)
{
    if (TeammateName.IsEmpty())
    {
        return nullptr;
    }
    for (const TPair<FString, TArray<TWeakObjectPtr<UTeammateCharacterAgentComponent>>> &Pair : TeammatesBySquad)
    {
        for (const TWeakObjectPtr<UTeammateCharacterAgentComponent> &Entry : Pair.Value)
        {
            UTeammateCharacterAgentComponent *Resolved = Entry.Get();
            if (Resolved != nullptr && Resolved->GetCharacterName().Equals(TeammateName))
            {
                UE_LOG(LogMindSand, Log, TEXT("Teamate Found with name [%s]"), *TeammateName);
                return Resolved;
            }
        }
    }

    UE_LOG(LogMindSand, Warning, TEXT("No Teamate Found with name [%s]"), *TeammateName);
    return nullptr;
}

UTeammateCharacterAgentComponent *UTeammateRegistry::GetTeammateById(const FString &TeammateId)
{
    if (TeammateId.IsEmpty())
    {
        return nullptr;
    }

    APawn *Pawn = Cast<APawn>(UActorRegistrySubsystem::Get(GetWorld())->GetActorFromID(TeammateId));
    if (Pawn == nullptr)
    {
        UE_LOG(LogMindSand, Warning, TEXT("GetTeammateById: no pawn for id [%s]"), *TeammateId);
        return nullptr;
    }

    AController *Controller = Pawn->GetController();
    if (Controller == nullptr)
    {
        UE_LOG(LogMindSand, Warning, TEXT("GetTeammateById: pawn [%s] has no controller"), *TeammateId);
        return nullptr;
    }

    UE_LOG(LogMindSand, Log, TEXT("Found Pawn with id [%s]"), *TeammateId);

    return Controller->GetComponentByClass<UTeammateCharacterAgentComponent>();
}

UTeammateCharacterAgentComponent *UTeammateRegistry::GetTeammate(const FString &NameOrId)
{
    UTeammateCharacterAgentComponent *agent = nullptr;
    if (NameOrId.IsEmpty())
    {
        UE_LOG(LogMindSand, Warning, TEXT("Empty teammate id - Using Random!"));
        agent = GetRandomTeammate();
    }
    else
    {
        agent = GetTeammateByName(NameOrId);
        if (agent == nullptr)
        {
            agent = GetTeammateById(NameOrId);
        }
    }

    return agent;
}
