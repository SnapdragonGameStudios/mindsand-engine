// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#pragma once

#include "CoreMinimal.h"

#include "ActionDescriptionAsset.generated.h"

// A fully author-owned parameter: used for AdditionalParameters, where the name and type are declared by hand
// rather than derived from a UPROPERTY.
USTRUCT(BlueprintType, Blueprintable)
struct MINDSAND_API FActionParameters
{
    GENERATED_BODY()
  public:
    UPROPERTY(EditAnywhere)
    FString Name;
    UPROPERTY(EditAnywhere)
    FString Type;
    UPROPERTY(EditAnywhere)
    FString ParameterDescription = "";
    // Told to the model so it knows whether the parameter has to be supplied. Deliberately NOT enforced when
    // parsing the response: an omitted parameter simply leaves the property at its compiled-in default.
    UPROPERTY(EditAnywhere)
    bool bIsOptional = false;
};

// A reflected parameter. Name and Type are derived from a UPROPERTY on the action class and must stay in step
// with it, so they are read-only - editing them would advertise a schema to the model that does not match the
// property. ParameterDescription and bIsOptional are author-owned: the asset is their only source of truth, so
// there is no C++ declaration for an edit here to contradict. That mix of permissions is the reason this struct
// exists separately from FActionParameters, since UPROPERTY specifiers are fixed per declaration and cannot
// vary per usage site.
//
// Both author-owned fields last until ActionClass changes, at which point UpdateReflectedParameters
// regenerates the whole array: the description reverts to the property's ToolTip and bIsOptional to false.
USTRUCT(BlueprintType, Blueprintable)
struct MINDSAND_API FNonEditableActionParameters
{
    GENERATED_BODY()
  public:
    UPROPERTY(VisibleDefaultsOnly)
    FString Name;
    UPROPERTY(VisibleDefaultsOnly)
    FString Type;
    UPROPERTY(EditAnywhere)
    FString ParameterDescription = "";
    // Told to the model so it knows whether the parameter has to be supplied. Deliberately NOT enforced when
    // parsing the response: an omitted parameter simply leaves the property at its compiled-in default.
    UPROPERTY(EditAnywhere)
    bool bIsOptional = false;
};

UCLASS(Blueprintable)
class MINDSAND_API UActionDescriptionAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()
  public:
    UActionDescriptionAsset();

    UPROPERTY(EditAnywhere)
    FString ActionName;
    UPROPERTY(EditAnywhere)
    TMap<FString, bool> OutEffects;
    UPROPERTY(EditAnywhere)
    TMap<FString, bool> Conditions;
    UPROPERTY(EditAnywhere)
    float Cost = 0.0f;
    UPROPERTY(EditAnywhere)
    FString ActionDescription;
    // Derived from the action class's own UPROPERTYs and rewritten wholesale whenever ActionClass changes.
    // Also the reflection allowlist: only names appearing here may be written by ReflectParametersJson.
    // EditFixedSize because the set of parameters is the class's to decide, not an author's.
    UPROPERTY(EditDefaultsOnly, EditFixedSize, Category = "Action Parameters")
    TArray<FNonEditableActionParameters> ReflectedParameters;

    UPROPERTY(EditAnywhere)
    TSubclassOf<class UGameplayTask_Action> ActionClass;

    // Human-owned: advertised to the model, but deliberately never reflected onto a UPROPERTY. A
    // ProcessParameters override is responsible for pulling these out of the raw parameters JSON itself.
    UPROPERTY(EditAnywhere, Category = "Action Parameters")
    TArray<FActionParameters> AdditionalParameters;

  public:
    FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId("ActionDescription", GetFName());
    };
    UGameplayTask_Action *CreateActionGameplayTask(UObject *owner);

    void PostLoad() override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(struct FPropertyChangedEvent &propertyChangedEvent) override;
#endif

  protected:
#if WITH_EDITOR
    // Editor-only: rebuilds ReflectedParameters by walking ActionClass's own UPROPERTYs. Reads property
    // metadata, which is stripped outside the editor.
    //
    // bPreserveAuthoredFields keeps ParameterDescription and bIsOptional for every parameter whose name
    // survives the rebuild. PostLoad passes true - it is reconciling the same class against possibly-changed
    // C++, so author intent still applies. Repointing ActionClass passes false, because intent written against
    // one action class does not carry over to a different one.
    void UpdateReflectedParameters(bool bPreserveAuthoredFields);
#endif

  public:
    TSharedPtr<FJsonValueObject> GetJSON(bool UseConditionsEffectsAndCost = false) const;
    static TArray<TSharedPtr<FJsonValue>> GetJSON(const TArray<UActionDescriptionAsset *> &actions,
                                                  bool UseConditionsEffectsAndCost = false);
    static TArray<TSharedPtr<FJsonValue>> GetJSON(const class UActionRegistrySubsystem *ActionRegistry,
                                                  const TArray<FString> &ActionNames,
                                                  bool UseConditionsEffectsAndCost = false);
    static TArray<TSharedPtr<FJsonValue>> GetActionNamesJSON(const TArray<UActionDescriptionAsset *> &actions);
};
