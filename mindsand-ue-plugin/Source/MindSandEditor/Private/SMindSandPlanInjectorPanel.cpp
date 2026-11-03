// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "SMindSandPlanInjectorPanel.h"

#include "Debug/MindSandDebugUtils.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/Text/TextLayout.h"
#include "HAL/FileManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/MessageDialog.h"
#include "Misc/Paths.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SMindSandPlanInjectorPanel"

DEFINE_LOG_CATEGORY_STATIC(LogMindSandEditor, Log, All);

namespace
{
/** Config location for the last active tab. Only UI state lives here; plan text lives in files. */
const TCHAR *const PlanInjectorConfigSection = TEXT("MindSandPlanInjector");
const TCHAR *const ActiveTabConfigKey = TEXT("ActiveTab");

/** Filename stem prefix, so a tab N is planN / planN.json. */
const FString PlanFilePrefix(TEXT("plan"));

/** "planN" - the tab label and the filename stem, kept in one place so the two cannot drift. */
FString PlanName(int32 Number)
{
    return FString::Printf(TEXT("%s%d"), *PlanFilePrefix, Number);
}

FText IdleStatusText()
{
    return LOCTEXT("StatusIdle", "Paste a plan response, then press Execute Plan while playing in editor. "
                                 "Execute also saves the tab.");
}

/** Parses "planN" into N; returns false for any other stem, so unrelated files are ignored. */
bool ParsePlanNumberFromStem(const FString &Stem, int32 &OutNumber)
{
    if (!Stem.StartsWith(PlanFilePrefix, ESearchCase::IgnoreCase))
    {
        return false;
    }

    const FString Digits = Stem.RightChop(PlanFilePrefix.Len());
    if (Digits.IsEmpty() || !Digits.IsNumeric())
    {
        return false;
    }

    OutNumber = FCString::Atoi(*Digits);
    return true;
}

} // namespace

void SMindSandPlanInjectorPanel::Construct(const FArguments &InArgs)
{
    // clang-format off
    ChildSlot
    [
        SNew(SVerticalBox)

        // Plan tabs
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(4.f, 4.f, 4.f, 0.f)
        [
            SNew(SScrollBox)
            .Orientation(Orient_Horizontal)

            + SScrollBox::Slot()
            [
                SAssignNew(TabStripBox, SHorizontalBox)
            ]
        ]

        // Toolbar
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(4.f, 4.f, 4.f, 2.f)
        [
            SNew(SHorizontalBox)

            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(SButton)
                .Text(LOCTEXT("ExecuteButton", "Execute Plan"))
                .ToolTipText(LOCTEXT("ExecuteButtonTooltip",
                    "Save this tab to Saved/MindSand/PlanInjector, then validate the JSON and hand it to "
                    "UMindSandDebugUtils::ExecutePlan in the running Play-In-Editor session."))
                .OnClicked(this, &SMindSandPlanInjectorPanel::OnExecuteClicked)
            ]

            + SHorizontalBox::Slot()
            .FillWidth(1.f)
            [
                SNullWidget::NullWidget
            ]

            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(SButton)
                .Text(LOCTEXT("ClearButton", "Clear"))
                .ToolTipText(LOCTEXT("ClearButtonTooltip", "Empty this tab's text box."))
                .OnClicked(this, &SMindSandPlanInjectorPanel::OnClearClicked)
            ]
        ]

        // Action-plan response: one text box per tab, so each keeps its own undo history and scroll position.
        + SVerticalBox::Slot()
        .FillHeight(1.f)
        .Padding(4.f, 2.f)
        [
            SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
            .Padding(2.f)
            [
                SAssignNew(TextBoxSwitcher, SWidgetSwitcher)
            ]
        ]

        // Status
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(6.f, 2.f, 6.f, 6.f)
        [
            SNew(STextBlock)
            .AutoWrapText(true)
            .Text(this, &SMindSandPlanInjectorPanel::GetStatusText)
            .ColorAndOpacity(this, &SMindSandPlanInjectorPanel::GetStatusColor)
        ]
    ];
    // clang-format on

    LoadTabsFromDisk();
    RebuildTabStrip();

    // The tab set is implied by the files on disk; only which one was open is remembered separately.
    int32 RestoredIndex = 0;
    GConfig->GetInt(PlanInjectorConfigSection, ActiveTabConfigKey, RestoredIndex, GEditorPerProjectIni);
    SetActiveTab(Tabs[FMath::Clamp(RestoredIndex, 0, Tabs.Num() - 1)]);
}

void SMindSandPlanInjectorPanel::LoadTabsFromDisk()
{
    const FString Directory = GetPlanDirectory();

    TArray<FString> FoundFiles;
    IFileManager::Get().FindFiles(FoundFiles, *(Directory / PlanFilePrefix + TEXT("*.json")), true, false);

    // planN.json ordered by N, so the strip does not reshuffle between sessions and plan10 sorts after
    // plan9 (alphabetical order on the filenames would not).
    TArray<int32> Numbers;
    for (const FString &FileName : FoundFiles)
    {
        int32 Number = 0;
        if (ParsePlanNumberFromStem(FPaths::GetBaseFilename(FileName), Number))
        {
            Numbers.AddUnique(Number);
        }
    }
    Numbers.Sort();

    for (const int32 Number : Numbers)
    {
        FString Contents;
        if (!FFileHelper::LoadFileToString(Contents, *GetPlanFilePath(Number)))
        {
            UE_LOG(LogMindSandEditor, Warning, TEXT("MindSand Plan Injector: could not read %s"),
                   *GetPlanFilePath(Number));
            continue;
        }
        AddTab(Number, FText::FromString(Contents));
    }

    if (Tabs.IsEmpty())
    {
        AddTab(0, FText::GetEmpty());
    }
}

TSharedPtr<SMindSandPlanInjectorPanel::FPlanTab> SMindSandPlanInjectorPanel::AddTab(int32 Number,
                                                                                    const FText &InitialText)
{
    TSharedPtr<FPlanTab> Tab = MakeShared<FPlanTab>();
    Tab->Number = Number;
    Tab->StatusText = IdleStatusText();

    // clang-format off
    SAssignNew(Tab->TextBox, SMultiLineEditableTextBox)
    .Font(FCoreStyle::GetDefaultFontStyle("Mono", 9))
    // Wrap to the panel width so a single-line pasted response stays readable. Per-character
    // wrapping because minified JSON can be one long run with no break opportunities.
    .AutoWrapText(true)
    .WrappingPolicy(ETextWrappingPolicy::AllowPerCharacterWrapping)
    .Text(InitialText)
    .HintText(LOCTEXT("PlanHint",
        "{\n"
        "  \"Plans\": [\n"
        "    {\n"
        "      \"teammate\": \"<teammate name>\",\n"
        "      \"acknowledgment\": \"On my way\",\n"
        "      \"Actions\": [\n"
        "        { \"Name\": \"<action>\", \"parameters\": { } }\n"
        "      ]\n"
        "    }\n"
        "  ]\n"
        "}"))
    .OnTextChanged(this, &SMindSandPlanInjectorPanel::OnPlanTextChanged);
    // clang-format on

    TextBoxSwitcher->AddSlot()[Tab->TextBox.ToSharedRef()];
    Tabs.Add(Tab);

    return Tab;
}

void SMindSandPlanInjectorPanel::CloseTab(TSharedPtr<FPlanTab> Tab)
{
    const int32 Index = Tabs.IndexOfByKey(Tab);
    if (Index == INDEX_NONE)
    {
        return;
    }

    const FString FilePath = GetPlanFilePath(Tab->Number);
    const bool bFileExists = IFileManager::Get().FileExists(*FilePath);
    const bool bHasText = Tab->TextBox.IsValid() && !Tab->TextBox->GetText().IsEmptyOrWhitespace();

    // Closing is destructive - the tab's file goes with it - so only an empty, unsaved tab closes silently.
    if (bHasText || bFileExists)
    {
        const FText Name = FText::FromString(PlanName(Tab->Number));
        const FText Prompt =
            bFileExists ? FText::Format(LOCTEXT("ConfirmCloseSaved", "Close {0} and delete {1}?"), Name,
                                        FText::FromString(FilePath))
                        : FText::Format(LOCTEXT("ConfirmCloseUnsaved", "Close {0}? Its text was never saved."), Name);
        if (FMessageDialog::Open(EAppMsgType::YesNo, Prompt) != EAppReturnType::Yes)
        {
            return;
        }
    }

    if (bFileExists && !IFileManager::Get().Delete(*FilePath))
    {
        UE_LOG(LogMindSandEditor, Error, TEXT("MindSand Plan Injector: could not delete %s"), *FilePath);
    }

    if (Tab->TextBox.IsValid())
    {
        TextBoxSwitcher->RemoveSlot(Tab->TextBox.ToSharedRef());
    }
    Tabs.RemoveAt(Index);

    // The panel is never tabless: closing the last one leaves a fresh empty plan0 to type into.
    if (Tabs.IsEmpty())
    {
        AddTab(0, FText::GetEmpty());
    }

    RebuildTabStrip();

    if (ActiveTab == Tab)
    {
        SetActiveTab(Tabs[FMath::Min(Index, Tabs.Num() - 1)]);
    }
    else
    {
        // Removing an earlier tab shifted the active tab's index, so the stored one needs refreshing.
        SaveActiveTabIndexToConfig();
    }
}

void SMindSandPlanInjectorPanel::SetActiveTab(TSharedPtr<FPlanTab> Tab)
{
    if (!Tab.IsValid() || !Tab->TextBox.IsValid())
    {
        return;
    }

    ActiveTab = Tab;
    TextBoxSwitcher->SetActiveWidget(Tab->TextBox.ToSharedRef());
    SaveActiveTabIndexToConfig();
}

void SMindSandPlanInjectorPanel::RebuildTabStrip()
{
    TabStripBox->ClearChildren();

    for (const TSharedPtr<FPlanTab> &Tab : Tabs)
    {
        // clang-format off
        TabStripBox->AddSlot()
        .AutoWidth()
        .Padding(0.f, 0.f, 2.f, 0.f)
        [
            MakeTabWidget(Tab)
        ];
        // clang-format on
    }

    // clang-format off
    TabStripBox->AddSlot()
    .AutoWidth()
    .Padding(4.f, 0.f, 0.f, 0.f)
    [
        SNew(SButton)
        .ButtonStyle(FAppStyle::Get(), "SimpleButton")
        .ToolTipText(LOCTEXT("AddTabTooltip", "Add another plan tab."))
        .OnClicked(this, &SMindSandPlanInjectorPanel::OnAddTabClicked)
        [
            SNew(SImage)
            .Image(FAppStyle::GetBrush("Icons.Plus"))
            .ColorAndOpacity(FSlateColor::UseForeground())
        ]
    ];
    // clang-format on
}

TSharedRef<SWidget> SMindSandPlanInjectorPanel::MakeTabWidget(TSharedPtr<FPlanTab> Tab)
{
    // Weak, so a tab widget left alive by a pending Slate operation cannot keep a closed tab's state
    // alive - and captures the tab's identity rather than its index, which shifts as tabs are closed.
    const TWeakPtr<FPlanTab> WeakTab = Tab;

    // clang-format off
    return SNew(SBorder)
    .BorderImage_Lambda([this, WeakTab]() -> const FSlateBrush *
    {
        const bool bIsActive = WeakTab.IsValid() && ActiveTab == WeakTab.Pin();
        return FAppStyle::GetBrush(bIsActive ? "Brushes.Header" : "Brushes.Recessed");
    })
    .Padding(FMargin(6.f, 2.f))
    [
        SNew(SHorizontalBox)

        + SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        [
            SNew(SButton)
            .ButtonStyle(FAppStyle::Get(), "NoBorder")
            .ContentPadding(FMargin(2.f, 0.f))
            .ToolTipText_Lambda([WeakTab]()
            {
                const TSharedPtr<FPlanTab> PinnedTab = WeakTab.Pin();
                if (!PinnedTab.IsValid())
                {
                    return FText::GetEmpty();
                }
                const FText Name = FText::FromString(PlanName(PinnedTab->Number));
                return PinnedTab->bDirty
                    ? FText::Format(LOCTEXT("TabDirtyTooltip",
                        "{0} - unsaved. Press Execute Plan to write it to disk."), Name)
                    : Name;
            })
            .OnClicked_Lambda([this, WeakTab]()
            {
                SetActiveTab(WeakTab.Pin());
                return FReply::Handled();
            })
            [
                SNew(STextBlock)
                .Text_Lambda([WeakTab]()
                {
                    const TSharedPtr<FPlanTab> PinnedTab = WeakTab.Pin();
                    if (!PinnedTab.IsValid())
                    {
                        return FText::GetEmpty();
                    }
                    // Trailing asterisk is the only unsaved-work cue: nothing is written until Execute.
                    return FText::FromString(PinnedTab->bDirty ? PlanName(PinnedTab->Number) + TEXT("*")
                                                               : PlanName(PinnedTab->Number));
                })
            ]
        ]

        + SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        .Padding(2.f, 0.f, 0.f, 0.f)
        [
            SNew(SButton)
            .ButtonStyle(FAppStyle::Get(), "SimpleButton")
            .ToolTipText(LOCTEXT("CloseTabTooltip", "Close this plan tab and delete its saved file."))
            .OnClicked_Lambda([this, WeakTab]()
            {
                CloseTab(WeakTab.Pin());
                return FReply::Handled();
            })
            [
                SNew(SImage)
                .Image(FAppStyle::GetBrush("Icons.X"))
                .ColorAndOpacity(FSlateColor::UseForeground())
            ]
        ]
    ];
    // clang-format on
}

int32 SMindSandPlanInjectorPanel::FindUnusedTabNumber() const
{
    // Lowest free number, so closing plan1 and adding a tab gives plan1 back rather than climbing.
    for (int32 Candidate = 0;; ++Candidate)
    {
        const bool bTaken =
            Tabs.ContainsByPredicate([Candidate](const TSharedPtr<FPlanTab> &Tab) { return Tab->Number == Candidate; });
        if (!bTaken && !IFileManager::Get().FileExists(*GetPlanFilePath(Candidate)))
        {
            return Candidate;
        }
    }
}

FString SMindSandPlanInjectorPanel::GetPlanDirectory()
{
    return FPaths::ProjectSavedDir() / TEXT("MindSand") / TEXT("PlanInjector");
}

FString SMindSandPlanInjectorPanel::GetPlanFilePath(int32 Number)
{
    return GetPlanDirectory() / PlanName(Number) + TEXT(".json");
}

void SMindSandPlanInjectorPanel::SaveActiveTab()
{
    if (!ActiveTab.IsValid() || !ActiveTab->TextBox.IsValid())
    {
        return;
    }

    const FString FilePath = GetPlanFilePath(ActiveTab->Number);
    IFileManager::Get().MakeDirectory(*GetPlanDirectory(), /*Tree=*/true);

    // Written verbatim - no wrapper object - so plans can be edited or copied outside the editor.
    const FString Contents = ActiveTab->TextBox->GetText().ToString();
    if (FFileHelper::SaveStringToFile(Contents, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        ActiveTab->bDirty = false;
    }
    else
    {
        // Leave bDirty set: the tab keeps its asterisk, which is the user-visible signal.
        UE_LOG(LogMindSandEditor, Error, TEXT("MindSand Plan Injector: could not write %s"), *FilePath);
    }
}

void SMindSandPlanInjectorPanel::SaveActiveTabIndexToConfig() const
{
    const int32 Index = Tabs.IndexOfByKey(ActiveTab);
    if (Index != INDEX_NONE)
    {
        GConfig->SetInt(PlanInjectorConfigSection, ActiveTabConfigKey, Index, GEditorPerProjectIni);
    }
}

FReply SMindSandPlanInjectorPanel::OnExecuteClicked()
{
    // Save first, unconditionally: Execute is the only save point, so a broken plan or a missing PIE
    // session must not cost the user their text.
    SaveActiveTab();

    const FString PlanResponse =
        (ActiveTab.IsValid() && ActiveTab->TextBox.IsValid()) ? ActiveTab->TextBox->GetText().ToString() : FString();

    UWorld *TargetWorld = GetTargetWorld();
    if (TargetWorld == nullptr)
    {
        SetStatus(EPlanInjectorStatus::Warning,
                  LOCTEXT("NoGameWorld", "Saved. No game world though - start a Play-In-Editor session to run it. "
                                         "The plan is dispatched through the game instance's "
                                         "UAITeammateOrchestrator."));
        return FReply::Handled();
    }

    if (UMindSandDebugUtils::ExecutePlan(TargetWorld, PlanResponse))
    {
        SetStatus(EPlanInjectorStatus::Success, LOCTEXT("PlanDispatched", "Dispatched action plan."));
        UE_LOG(LogMindSandEditor, Log, TEXT("MindSand Plan Injector: dispatched action plan."));
    }
    else
    {
        SetStatus(EPlanInjectorStatus::Error,
                  LOCTEXT("PlanDispatchFailed",
                          "Dispatch failed - a named teammate is not registered, or the orchestrator is "
                          "unavailable. Plans ahead of the failing one were already dispatched. "
                          "See the Output Log."));
        UE_LOG(LogMindSandEditor, Error, TEXT("MindSand Plan Injector: ExecutePlan returned false."));
    }

    return FReply::Handled();
}

FReply SMindSandPlanInjectorPanel::OnClearClicked()
{
    if (!ActiveTab.IsValid())
    {
        return FReply::Handled();
    }

    if (ActiveTab->TextBox.IsValid())
    {
        // SetText does not raise OnTextChanged, so mark the tab dirty here.
        ActiveTab->TextBox->SetText(FText::GetEmpty());
        ActiveTab->bDirty = true;
    }
    SetStatus(EPlanInjectorStatus::Idle, IdleStatusText());

    return FReply::Handled();
}

FReply SMindSandPlanInjectorPanel::OnAddTabClicked()
{
    const TSharedPtr<FPlanTab> NewTab = AddTab(FindUnusedTabNumber(), FText::GetEmpty());
    RebuildTabStrip();
    SetActiveTab(NewTab);

    return FReply::Handled();
}

void SMindSandPlanInjectorPanel::OnPlanTextChanged(const FText &NewText)
{
    if (!ActiveTab.IsValid())
    {
        return;
    }

    ActiveTab->bDirty = true;

    // A stale pass/fail line next to edited text is misleading, so drop it on the first keystroke.
    if (ActiveTab->Status != EPlanInjectorStatus::Idle)
    {
        SetStatus(EPlanInjectorStatus::Idle, LOCTEXT("StatusEdited", "Edited - press Execute Plan to save and run "
                                                                     "it."));
    }
}

FText SMindSandPlanInjectorPanel::GetStatusText() const
{
    return ActiveTab.IsValid() ? ActiveTab->StatusText : IdleStatusText();
}

FSlateColor SMindSandPlanInjectorPanel::GetStatusColor() const
{
    switch (ActiveTab.IsValid() ? ActiveTab->Status : EPlanInjectorStatus::Idle)
    {
    case EPlanInjectorStatus::Success:
        return FSlateColor(FLinearColor(0.2f, 0.8f, 0.2f));
    case EPlanInjectorStatus::Warning:
        return FSlateColor(FLinearColor(0.9f, 0.7f, 0.1f));
    case EPlanInjectorStatus::Error:
        return FSlateColor(FLinearColor(0.9f, 0.25f, 0.25f));
    default:
        return FSlateColor::UseSubduedForeground();
    }
}

void SMindSandPlanInjectorPanel::SetStatus(EPlanInjectorStatus NewStatus, const FText &Message)
{
    if (!ActiveTab.IsValid())
    {
        return;
    }

    ActiveTab->Status = NewStatus;
    ActiveTab->StatusText = Message;
}

UWorld *SMindSandPlanInjectorPanel::GetTargetWorld()
{
    if (GEditor != nullptr && GEditor->PlayWorld != nullptr)
    {
        return GEditor->PlayWorld;
    }

    if (GEngine == nullptr)
    {
        return nullptr;
    }

    // Covers -game / standalone-in-process cases where PlayWorld is not set.
    for (const FWorldContext &Context : GEngine->GetWorldContexts())
    {
        if (Context.WorldType != EWorldType::PIE && Context.WorldType != EWorldType::Game)
        {
            continue;
        }
        UWorld *World = Context.World();
        if (World != nullptr && World->GetGameInstance() != nullptr)
        {
            return World;
        }
    }

    return nullptr;
}

#undef LOCTEXT_NAMESPACE
