// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SHorizontalBox;
class SMultiLineEditableTextBox;
class SWidgetSwitcher;

/**
 * Plan Injector panel: a strip of plan tabs over a large text box for a hand-written (or copy-pasted) LLM
 * plan response, plus an Execute button that feeds the active tab to UMindSandDebugUtils::ExecutePlan.
 * Dispatch requires a running Play-In-Editor session, because the orchestrator that consumes the plan is
 * a UGameInstanceSubsystem.
 *
 * Each tab is backed by <Project>/Saved/MindSand/PlanInjector/planN.json, written on Execute, so plans
 * survive editor restarts and can be authored or copied with an external editor.
 */
class SMindSandPlanInjectorPanel : public SCompoundWidget
{
  public:
    SLATE_BEGIN_ARGS(SMindSandPlanInjectorPanel)
    {
    }
    SLATE_END_ARGS()

    void Construct(const FArguments &InArgs);

  private:
    enum class EPlanInjectorStatus : uint8
    {
        Idle,
        Success,
        Warning,
        Error
    };

    /** One plan tab. Held by shared pointer so callbacks can capture an identity, not an array index. */
    struct FPlanTab
    {
        /** N in "planN" / "planN.json"; also the tab label. */
        int32 Number = 0;
        TSharedPtr<SMultiLineEditableTextBox> TextBox;
        FText StatusText;
        EPlanInjectorStatus Status = EPlanInjectorStatus::Idle;
        /** Text differs from what is on disk (or was never written). */
        bool bDirty = false;
    };

    // -- Tabs ------------------------------------------------------------------------------------

    void LoadTabsFromDisk();
    /** Appends a tab (lowest unused number) and makes it active. */
    TSharedPtr<FPlanTab> AddTab(int32 Number, const FText &InitialText);
    void CloseTab(TSharedPtr<FPlanTab> Tab);
    void SetActiveTab(TSharedPtr<FPlanTab> Tab);
    void RebuildTabStrip();
    TSharedRef<SWidget> MakeTabWidget(TSharedPtr<FPlanTab> Tab);
    int32 FindUnusedTabNumber() const;

    // -- Persistence -----------------------------------------------------------------------------

    static FString GetPlanDirectory();
    static FString GetPlanFilePath(int32 Number);
    void SaveActiveTab();
    void SaveActiveTabIndexToConfig() const;

    // -- Toolbar / status ------------------------------------------------------------------------

    FReply OnExecuteClicked();
    FReply OnClearClicked();
    FReply OnAddTabClicked();
    void OnPlanTextChanged(const FText &NewText);

    FText GetStatusText() const;
    FSlateColor GetStatusColor() const;
    void SetStatus(EPlanInjectorStatus NewStatus, const FText &Message);

    /** The world to inject into: the PIE/game world, or null when none is running. */
    static UWorld *GetTargetWorld();

    TArray<TSharedPtr<FPlanTab>> Tabs;
    TSharedPtr<FPlanTab> ActiveTab;

    TSharedPtr<SHorizontalBox> TabStripBox;
    TSharedPtr<SWidgetSwitcher> TextBoxSwitcher;
};
