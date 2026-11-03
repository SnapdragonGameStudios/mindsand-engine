// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "MindSandSettings.generated.h"

UENUM(BlueprintType)
enum class EActionPlanFormat : uint8
{
    DSL UMETA(DisplayName = "DSL"),
    Json UMETA(DisplayName = "JSON")
};

/**
 * MindSand AI developer settings.
 * Accessible via: Edit > Project Settings > Plugins > MindSand Settings
 *
 * Values are stored in Config/DefaultGame.ini under [/Script/MindSand.MindSandSettings].
 */
UCLASS(Config = Game, defaultconfig, meta = (DisplayName = "MindSand Settings"))
class MINDSAND_API UMindSandSettings : public UDeveloperSettings
{
    GENERATED_BODY()

  public:
    UMindSandSettings();

    /**
     * LLM Provider
     */
    UPROPERTY(Config, EditAnywhere, Category = "MindSand Settings | LLM",
              meta = (DisplayName = "LLM Provider", ToolTip = "Which provider to use. Http, Genie, etc."))
    FString LLMProvider = TEXT("HttpLLMProvider");

    UPROPERTY(Config, EditAnywhere, Category = "MindSand Settings | LLM",
              meta = (DisplayName = "System Prompt File Path", ToolTip = "Path to the System Prompt file"))
    FString SystemPromptPath;

    UPROPERTY(Config, EditAnywhere, Category = "MindSand Settings | LLM",
              meta = (DisplayName = "Use JSON",
                      ToolTip = "Send static game information as JSON instead of encoded Markdown"))
    bool bUseJson = false;

    UPROPERTY(Config, EditAnywhere, Category = "MindSand Settings | LLM",
              meta = (DisplayName = "Action Plan Format", ToolTip = "Format expected from LLM action plans"))
    EActionPlanFormat ActionPlanFormat = EActionPlanFormat::DSL;

    UPROPERTY(Config, EditAnywhere, Category = "MindSand Settings",
              meta = (DisplayName = "Use Unified Perception for Teammates",
                      ToolTip = "Should the teammates and player use unified perception"))
    bool bUseUnifiedPerception = true;

    /** Category shown in the Project Settings sidebar. */
    virtual FName GetCategoryName() const override
    {
        return FName("Plugins");
    }

#if WITH_EDITOR
    /** Section name within the category. */
    virtual FText GetSectionText() const override
    {
        return NSLOCTEXT("MindSandSettings", "SectionText", "MindSand Settings");
    }

    virtual FText GetSectionDescription() const override
    {
        return NSLOCTEXT("MindSandSettings", "SectionDesc",
                         "Configure the LLM Provider"
                         "used by the MindSand AI teammate system.");
    }
#endif
};
