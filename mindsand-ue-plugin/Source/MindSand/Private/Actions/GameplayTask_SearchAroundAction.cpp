// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "Actions/GameplayTask_SearchAroundAction.h"
#include "MindSandLog.h"
#include "Teammates/TeammateCharacterAgentComponent.h"
#include "VisualLogger/VisualLogger.h"
#include <AIController.h>

void UGameplayTask_SearchAroundAction::Activate()
{
    Super::Activate();

    AAIController *AIController = Cast<AAIController>(GetOwnerActor());

    if (RelativeDirection.Compare("Left") == 0)
    {
        SearchDirection = SD_LEFT;
    }
    else if (RelativeDirection.Compare("Right") == 0)
    {
        SearchDirection = SD_RIGHT;
    }
    else if (RelativeDirection.Compare("Front") == 0)
    {
        SearchDirection = SD_FRONT;
    }
    else if (RelativeDirection.Compare("Back") == 0)
    {
        SearchDirection = SD_BACK;
    }
    else if (RelativeDirection.Compare("Leftmost") == 0)
    {
        SearchDirection = SD_LEFTMOST;
    }
    else if (RelativeDirection.Compare("Rightmost") == 0)
    {
        SearchDirection = SD_RIGHTMOST;
    }
    else if (RelativeDirection.Compare("Farthest") == 0)
    {
        SearchDirection = SD_FARTHEST;
    }
    else if (RelativeDirection.Compare("Nearest") == 0)
    {
        SearchDirection = SD_NEAREST;
    }
    else
    {
        SearchDirection = SD_AROUND;
    }

    float BestDistance = FLT_MAX;
    float SearchAngleDot = 1.0f;
    float AngleLimit = 0.7f;
    float BestAngleDot = 0.3f;

    FVector ReferenceLocation;

    if (ReferenceActor == nullptr)
    {
        EndAction(false);
        return;
    }

    ReferenceLocation = ReferenceActor->GetActorLocation();

    APawn *Player = AIController->GetWorld()->GetFirstPlayerController()->GetPawn();
    FVector ToReference = ReferenceLocation - Player->GetActorLocation();
    if (ToReference.Length() < FLT_MIN)
    {
        ToReference = -Player->GetActorForwardVector();
    }
    ToReference.Z = 0;
    ToReference.Normalize();
    FVector Right = FVector::CrossProduct(FVector::UpVector, ToReference);
    FVector TestVector = FVector::Zero();
    UE_VLOG(AIController, LogMindSand, Log, TEXT("SearchAround: direction=%s  Right=(%.2f, %.2f, %.2f)"),
            *RelativeDirection, Right.X, Right.Y, Right.Z);
    switch (SearchDirection)
    {
    case SD_LEFTMOST:
    case SD_LEFT: {
        TestVector = -Right;
    }
    break;
    case SD_RIGHTMOST:
    case SD_RIGHT: {
        TestVector = Right;
    }
    break;
    case SD_FRONT: {
        TestVector = -ToReference;
    }
    break;
    case SD_BACK: {
        TestVector = ToReference;
    }
    break;
    }

    AActor *BestCandidate = nullptr;
    UE_VLOG(AIController, LogMindSand, Log, TEXT("SearchAround: TestVector=(%.2f, %.2f, %.2f)"), TestVector.X,
            TestVector.Y, TestVector.Z);

    // Draw the desired search direction from the reference point
    if (!TestVector.IsNearlyZero())
    {
        UE_VLOG_ARROW(AIController, LogMindSand, Log, ReferenceLocation, ReferenceLocation + TestVector * 200.f,
                      FColor::Yellow, TEXT("SearchDir: %s"), *RelativeDirection);
    }

    if (SearchDirection == SD_AROUND || SearchDirection == SD_NEAREST || SearchDirection == SD_FARTHEST)
    {
        for (auto &actor : ActorsToLookFor)
        {
            FVector ToSubject = actor->GetActorLocation() - ReferenceLocation;
            ToSubject.Z = 0;
            float distance = ToSubject.Length();

            UE_VLOG_ARROW(AIController, LogMindSand, Log, ReferenceLocation, actor->GetActorLocation(), FColor::White,
                          TEXT("%.0f"), distance);

            if (SearchDirection != SD_FARTHEST)
            {
                if (distance < BestDistance)
                {
                    BestDistance = distance;
                    BestCandidate = actor;
                }
            }
            else
            {
                if (distance > BestDistance)
                {
                    BestDistance = distance;
                    BestCandidate = actor;
                }
            }
        }
    }
    else
    {
        for (auto &actor : ActorsToLookFor)
        {
            FVector ToSubject = actor->GetActorLocation() - ReferenceLocation;
            ToSubject.Z = 0;
            float distance = ToSubject.Length() / 200;
            distance = FMath::Max(1.0, distance);
            ToSubject.Normalize();
            float angle = FVector::DotProduct(TestVector, ToSubject);

            bool bConsidered = false;
            if (SearchDirection != SD_LEFTMOST || SearchDirection != SD_RIGHTMOST)
            {
                if (angle > (1 - (AngleLimit / distance)))
                {
                    bConsidered = true;
                    if (distance < BestDistance)
                    {
                        BestDistance = distance;
                        BestCandidate = actor;
                    }
                }
            }
            else
            {
                if (angle > AngleLimit)
                {
                    bConsidered = true;
                    if (distance > BestDistance)
                    {
                        BestDistance = distance;
                        BestCandidate = actor;
                    }
                }
            }

            UE_VLOG_ARROW(AIController, LogMindSand, Log, ReferenceLocation, actor->GetActorLocation(),
                          bConsidered ? FColor::Cyan : FColor::Red, TEXT("dot=%.2f"), angle);
        }
    }

    UE_CVLOG_SPHERE(BestCandidate != nullptr, AIController, LogMindSand, Log, BestCandidate->GetActorLocation(), 50.f,
                    FColor::Green, TEXT("Best: %s"), *BestCandidate->GetName());
    UE_CVLOG(BestCandidate != nullptr, AIController, LogMindSand, Log, TEXT("SearchAround: selected '%s'"),
             *BestCandidate->GetName());
    UE_CVLOG(BestCandidate == nullptr, AIController, LogMindSand, Warning,
             TEXT("SearchAround: no candidate found for direction '%s'"), *RelativeDirection);

    bool success = false;
    if (BestCandidate != nullptr)
    {
        if (UTeammateCharacterAgentComponent *planner =
                AIController->GetComponentByClass<UTeammateCharacterAgentComponent>())
        {
            planner->GetActionPlanEngine()->SetTargetedActorForActions(BestCandidate);
            success = true;
        }
    }

    EndAction(success);
}
