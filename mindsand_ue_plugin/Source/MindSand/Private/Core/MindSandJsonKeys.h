// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "CoreMinimal.h"

/** Canonical wire-format keys used by MindSand JSON producers and consumers. */
namespace MindSandJsonKeys
{
namespace Prompt
{
inline constexpr TCHAR StaticGameInfo[] = TEXT("staticGameInfo");
inline constexpr TCHAR DynamicGameInfo[] = TEXT("dynamicGameInfo");
inline constexpr TCHAR Command[] = TEXT("command");
} // namespace Prompt
namespace StaticGameInfo
{
inline constexpr TCHAR ActionDescriptions[] = TEXT("action Descriptions");
inline constexpr TCHAR Teammates[] = TEXT("teammates");
inline constexpr TCHAR GlobalEvents[] = TEXT("GlobalEvents");
} // namespace StaticGameInfo
namespace DynamicGameInfo
{
inline constexpr TCHAR ActorDescriptions[] = TEXT("actorDescriptions");
inline constexpr TCHAR KnownActors[] = TEXT("knownActors");
inline constexpr TCHAR ObjectStates[] = TEXT("objectStates");
} // namespace DynamicGameInfo
namespace Action
{
inline constexpr TCHAR Name[] = TEXT("Name");
inline constexpr TCHAR Description[] = TEXT("Description");
inline constexpr TCHAR Conditions[] = TEXT("Conditions");
inline constexpr TCHAR Effects[] = TEXT("Effects");
inline constexpr TCHAR Cost[] = TEXT("Cost");
inline constexpr TCHAR Parameters[] = TEXT("Parameters");
} // namespace Action
namespace ActionParameter
{
inline constexpr TCHAR Name[] = TEXT("Name");
inline constexpr TCHAR Type[] = TEXT("Type");
inline constexpr TCHAR Description[] = TEXT("Parameter Description");
inline constexpr TCHAR IsRequired[] = TEXT("IsRequired");
} // namespace ActionParameter
namespace Teammate
{
inline constexpr TCHAR Name[] = TEXT("name");
inline constexpr TCHAR Personality[] = TEXT("personality");
inline constexpr TCHAR AvailableActions[] = TEXT("availableActions");
inline constexpr TCHAR StateVariables[] = TEXT("TeammateStateVariables");
} // namespace Teammate
namespace Perception
{
inline constexpr TCHAR ActorId[] = TEXT("id");
inline constexpr TCHAR State[] = TEXT("state");
inline constexpr TCHAR Sense[] = TEXT("sense");
inline constexpr TCHAR SuccessfullySensed[] = TEXT("successfullySensed");
inline constexpr TCHAR Age[] = TEXT("age");
inline constexpr TCHAR Stimuli[] = TEXT("stimuli");
} // namespace Perception
namespace ActionPlanResponse
{
inline constexpr TCHAR Plans[] = TEXT("Plans");
inline constexpr TCHAR Teammate[] = TEXT("teammate");
inline constexpr TCHAR Acknowledgment[] = TEXT("acknowledgment");
inline constexpr TCHAR Actions[] = TEXT("Actions");
inline constexpr TCHAR SignalEndTaskToTeammates[] = TEXT("SignalEndTaskToTeammates");
inline constexpr TCHAR Parameters[] = TEXT("parameters");
} // namespace ActionPlanResponse
} // namespace MindSandJsonKeys
