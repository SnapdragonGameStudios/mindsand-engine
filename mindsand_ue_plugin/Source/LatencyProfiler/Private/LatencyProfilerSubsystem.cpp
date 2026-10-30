// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "LatencyProfilerSubsystem.h"
#include "HAL/PlatformTime.h"
#include "ProfilingDebugging/CsvProfiler.h"

CSV_DEFINE_CATEGORY(LatencyProfiler, true);

// ---------------------------------------------------------------------------
// FLatencyStatAggregate
// ---------------------------------------------------------------------------

FLatencyStatAggregate::FLatencyStatAggregate(int32 InSize)
    : Latest(0.f), Min(0.f), Max(0.f), Average(0.f), BufferSize(FMath::Max(1, InSize)), Index(0), Count(0)
{
    Buffer.SetNumZeroed(BufferSize);
}

void FLatencyStatAggregate::AddSample(float Value)
{
    Buffer[Index] = Value;
    Index = (Index + 1) % BufferSize;
    Count = FMath::Min(Count + 1, BufferSize);
    Latest = Value;

    float Sum = 0.f;
    float NewMin = FLT_MAX;
    float NewMax = -FLT_MAX;

    for (int32 i = 0; i < Count; ++i)
    {
        const float S = Buffer[i];
        Sum += S;
        if (S < NewMin)
            NewMin = S;
        if (S > NewMax)
            NewMax = S;
    }

    Min = NewMin;
    Max = NewMax;
    Average = Sum / static_cast<float>(Count);
}

// ---------------------------------------------------------------------------
// ULatencyProfilerSubsystem
// ---------------------------------------------------------------------------

void ULatencyProfilerSubsystem::Initialize(FSubsystemCollectionBase &Collection)
{
    Super::Initialize(Collection);
}

void ULatencyProfilerSubsystem::Deinitialize()
{
    PendingStartTimes.Empty();
    Aggregates.Empty();
    Super::Deinitialize();
}

void ULatencyProfilerSubsystem::RegisterStat(FName StatName)
{
    if (!Aggregates.Contains(StatName))
    {
        Aggregates.Emplace(StatName, FLatencyStatAggregate());
    }
}

void ULatencyProfilerSubsystem::MarkStart(FName StatName)
{
    PendingStartTimes.FindOrAdd(StatName) = FPlatformTime::Seconds();
}

void ULatencyProfilerSubsystem::MarkEnd(FName StatName)
{
    const double Now = FPlatformTime::Seconds();

    const double *StartTime = PendingStartTimes.Find(StatName);
    if (!StartTime)
    {
        return;
    }

    const float DurationMs = static_cast<float>((Now - *StartTime) * 1000.0);
    PendingStartTimes.Remove(StatName);

#if CSV_PROFILER
    FCsvProfiler::RecordCustomStat(StatName, CSV_CATEGORY_INDEX(LatencyProfiler), DurationMs, ECsvCustomStatOp::Set);
#endif

    RegisterStat(StatName);
    Aggregates[StatName].AddSample(DurationMs);
}

float ULatencyProfilerSubsystem::GetLatest(FName StatName) const
{
    if (const FLatencyStatAggregate *Agg = Aggregates.Find(StatName))
    {
        return Agg->Latest;
    }
    return 0.f;
}

float ULatencyProfilerSubsystem::GetAverage(FName StatName) const
{
    if (const FLatencyStatAggregate *Agg = Aggregates.Find(StatName))
    {
        return Agg->Average;
    }
    return 0.f;
}

float ULatencyProfilerSubsystem::GetMin(FName StatName) const
{
    if (const FLatencyStatAggregate *Agg = Aggregates.Find(StatName))
    {
        return Agg->Min;
    }
    return 0.f;
}

float ULatencyProfilerSubsystem::GetMax(FName StatName) const
{
    if (const FLatencyStatAggregate *Agg = Aggregates.Find(StatName))
    {
        return Agg->Max;
    }
    return 0.f;
}
