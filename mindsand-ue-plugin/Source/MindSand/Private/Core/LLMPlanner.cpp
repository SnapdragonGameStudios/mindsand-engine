// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "Core/LLMPlanner.h"

#include "Actions/Parsers/BufferedActionPlanTokenProcessor.h"
#include "Actions/Parsers/JsonActionPlanParser.h"
#include "Actions/Parsers/StreamingActionPlanTokenProcessor.h"
#include "Actions/Parsers/StreamingDSLActionPlanParser.h"
#include "Core/MindSandJsonKeys.h"
#include "Core/PromptEncoding.h"
#include "Debug/MindSandDebug.h"
#include "Framework/LLMPipelinesTypes.h"
#include "Json.h"
#include "LatencyProfilerSubsystem.h"
#include "MindSandLog.h"
#include "MindSandSettings.h"
#include "PromptContext/PromptContextGatherer.h"
#include "Utils.h"

static TAutoConsoleVariable<bool> CVarMindSandDisableLLM(TEXT("MindSand.DisableLLM"), 0,
                                                         TEXT("0: Enable the feature\n") TEXT("1: Disable the feature"),
                                                         ECVF_Cheat);

static TAutoConsoleVariable<bool> CVarMindSandResetOnQuery(TEXT("MindSand.ResetOnQuery"), 0,
                                                           TEXT("0: Preserve conversation history (static info once)\n")
                                                               TEXT("1: Reset conversation on each query (legacy)"),
                                                           ECVF_Cheat);

void ULLMPlanner::Initialize()
{
    const UMindSandSettings *Settings = GetDefault<UMindSandSettings>();
    FOnOperationComplete LLMInitDelegate;
    LLMInitDelegate.BindDynamic(this, &ULLMPlanner::OnLLMInit);
    LLMModel = NewObject<ULLMModel>(this);
    UE_LOG(LogMindSand, Log, TEXT("Using Provider - %s"), *(Settings->LLMProvider));
    LLMModel->Initialize({}, LLMInitDelegate, Settings->LLMProvider);

    if (Settings && Settings->ActionPlanFormat == EActionPlanFormat::DSL)
        TokenProcessor = MakeUnique<FStreamingActionPlanTokenProcessor>(MakeUnique<FStreamingDSLActionPlanParser>());
    else
        TokenProcessor = MakeUnique<FBufferedActionPlanTokenProcessor>(MakeUnique<FJsonActionPlanParser>());
}

void ULLMPlanner::Deinitialize()
{
    if (TokenProcessor != nullptr)
    {
        TokenProcessor->Stop();
        TokenProcessor.Reset();
    }
    ActionsCallback = nullptr;
    PlanFinishedCallback = nullptr;
    bFirstQuery = true;
}

void ULLMPlanner::SetCallbacks(FOnActionsParsed InActionsCallback, FOnPlanFinished InPlanFinishedCallback)
{
    ActionsCallback = MoveTemp(InActionsCallback);
    PlanFinishedCallback = MoveTemp(InPlanFinishedCallback);
}

void ULLMPlanner::RequestPlan(const FString &Command, const FString &TeamName)
{
    TSharedRef<FJsonObject> Prompt = MakeShared<FJsonObject>();

    if (!CVarMindSandResetOnQuery.GetValueOnGameThread())
    {
        if (bFirstQuery)
        {
            Prompt->SetObjectField(MindSandJsonKeys::Prompt::StaticGameInfo,
                                   FPromptContextGatherer::GetStaticGameInfo(GetWorld(), TeamName));
            bFirstQuery = false;
        }
    }
    else
    {
        Prompt->SetObjectField(MindSandJsonKeys::Prompt::StaticGameInfo,
                               FPromptContextGatherer::GetStaticGameInfo(GetWorld(), TeamName));
    }

    Prompt->SetObjectField(MindSandJsonKeys::Prompt::DynamicGameInfo,
                           FPromptContextGatherer::GetDynamicGameInfo(GetWorld(), TeamName));
    Prompt->SetStringField(MindSandJsonKeys::Prompt::Command, Command);

    const UMindSandSettings *Settings = GetDefault<UMindSandSettings>();
    const FString Request =
        Settings && Settings->bUseJson ? Utils::Serialize(Prompt) : MindSandPromptEncoding::EncodePrompt(Prompt);

    SendQuery(Request);
}

void ULLMPlanner::SendQuery(const FString &Text)
{
    const FString QueryText = Text.TrimStartAndEnd();
    if (QueryText.IsEmpty())
    {
        UE_LOG(LogMindSand, Warning, TEXT("[LLM][ENGINE] Ignoring empty LLM query"));
        return;
    }

    if (!IsValid(ConversationEngine))
    {
        UE_LOG(LogMindSand, Error, TEXT("[LLM][ENGINE] Cannot send LLM query: conversation engine is invalid"));
        return;
    }

    GetWorld()->GetGameInstance()->GetSubsystem<ULatencyProfilerSubsystem>()->MarkStart("LLM");

    if (CVarMindSandResetOnQuery.GetValueOnGameThread())
    {
        ConversationEngine->ResetSync();
    }
    const uint64 QueryId = BeginTokenProcessing();
    CurrentQueryKey =
        FString::Printf(TEXT("QUERY_%llu_"), static_cast<unsigned long long>(QueryId)) + FGuid::NewGuid().ToString();
    CurrentResponse.Empty();
    DumpMindSandJson(CurrentQueryKey, TEXT("request"), QueryText);

    if (!CVarMindSandDisableLLM.GetValueOnGameThread())
    {
        ConversationEngine->SendMessage(
            QueryText,
            FOnLLMTokenGeneratedNative::CreateWeakLambda(
                this, [this](ELLMSentenceCode Code, FString Token) { OnConvEngineToken(Code, MoveTemp(Token)); }),
            FOnLLMResponseCompleteNative());
    }
}

bool ULLMPlanner::InjectResponse(const FString &Response)
{
    if (TokenProcessor == nullptr)
    {
        UE_LOG(LogMindSand, Error, TEXT("[LLM][TEST] Cannot process response: token processor is invalid."));
        return false;
    }

    const uint64 QueryId = BeginTokenProcessing();
    UE_LOG(LogMindSand, Log, TEXT("[Query %llu] Processing supplied complete response."),
           static_cast<unsigned long long>(QueryId));
    return TokenProcessor->ProcessTokens(ELLMSentenceCode::Complete, Response);
}

uint64 ULLMPlanner::BeginTokenProcessing()
{
    if (TokenProcessor == nullptr)
    {
        UE_LOG(LogMindSand, Warning, TEXT("[LLM][ENGINE] Cannot begin token processing: token processor is invalid."));
        return 0;
    }

    return TokenProcessor->Begin(ActionsCallback, PlanFinishedCallback);
}

void ULLMPlanner::OnLLMInit(bool bSuccess)
{
    UE_LOG(LogMindSand, Log, TEXT("LLM model init success? %d"), bSuccess ? 1 : 0);
    if (!bSuccess)
    {
        return;
    }

    const UMindSandSettings *Settings = GetDefault<UMindSandSettings>();
    FString SystemPromptText = FString();
    {
        FString FilePath = FPaths::ProjectContentDir() + Settings->SystemPromptPath;
        FFileHelper::LoadFileToString(SystemPromptText, *FilePath);
        UE_LOG(LogMindSand, Log, TEXT("SystemPromptPath = [%s]"), *(Settings->SystemPromptPath));
        UE_LOG(LogMindSand, Log, TEXT("SystemPrompt FilePath = [%s]"), *FilePath);
        ensureMsgf(!SystemPromptText.IsEmpty(), TEXT("**** System Prompt is Empty!"));
    }

    ConversationEngine = NewObject<UConversationEngine>(this);
    FConversationEngineConfig ConvEngineConfig;
    {
        ConvEngineConfig.Model = LLMModel;
        ConvEngineConfig.SystemPrompt = SystemPromptText;
    }

    ConversationEngine->Initialize(ConvEngineConfig, FOnOperationCompleteNative::CreateWeakLambda(
                                                         this, [this](bool bSuccess) { OnConvEngineInit(bSuccess); }));
}

void ULLMPlanner::OnConvEngineInit(bool bSuccess)
{
    UE_LOG(LogMindSand, Log, TEXT("Conv engine init success? %d"), bSuccess ? 1 : 0);
}

void ULLMPlanner::OnConvEngineToken(ELLMSentenceCode Code, FString Token)
{
    UE_LOG(LogMindSand, Verbose, TEXT("Conversation Engine Token Received [code %d]"), Code);

    CurrentResponse += Token;

    if (TokenProcessor != nullptr)
    {
        TokenProcessor->ProcessTokens(Code, Token);
    }

    if (Code != ELLMSentenceCode::End && Code != ELLMSentenceCode::Complete)
    {
        return;
    }

    GetWorld()->GetGameInstance()->GetSubsystem<ULatencyProfilerSubsystem>()->MarkEnd("LLM");
    DumpMindSandJson(CurrentQueryKey, TEXT("response"), CurrentResponse);
}
