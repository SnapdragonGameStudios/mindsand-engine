// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "CoreMinimal.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "LatencyProfilerSubsystem.generated.h"

CSV_DECLARE_CATEGORY_EXTERN(LatencyProfiler);

USTRUCT(BlueprintType)
struct LATENCYPROFILER_API FLatencyStatAggregate
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Latency")
    float Latest = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Latency")
    float Min = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Latency")
    float Max = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Latency")
    float Average = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Latency")
    int32 BufferSize = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Latency")
    int32 Index = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Latency")
    int32 Count = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Latency")
    TArray<float> Buffer;

    FLatencyStatAggregate(int32 InSize = 16);
    void AddSample(float Value);
};

UCLASS()
class LATENCYPROFILER_API ULatencyProfilerSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

  public:
    // USubsystem interface
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Latency Profiler")
    void RegisterStat(FName StatName);

    UFUNCTION(BlueprintCallable, Category = "Latency Profiler")
    void MarkStart(FName StatName);

    UFUNCTION(BlueprintCallable, Category = "Latency Profiler")
    void MarkEnd(FName StatName);

    UFUNCTION(BlueprintCallable, Category = "Latency Profiler")
    float GetLatest(FName StatName) const;

    UFUNCTION(BlueprintCallable, Category = "Latency Profiler")
    float GetAverage(FName StatName) const;

    UFUNCTION(BlueprintCallable, Category = "Latency Profiler")
    float GetMin(FName StatName) const;

    UFUNCTION(BlueprintCallable, Category = "Latency Profiler")
    float GetMax(FName StatName) const;

  private:
    TMap<FName, double> PendingStartTimes;
    TMap<FName, FLatencyStatAggregate> Aggregates;
};
