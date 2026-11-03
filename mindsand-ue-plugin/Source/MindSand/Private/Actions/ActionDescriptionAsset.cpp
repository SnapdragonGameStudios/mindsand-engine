// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "Actions/ActionDescriptionAsset.h"
#include "Actions/ActionRegistrySubsystem.h"
#include "Actions/GameplayTask_Action.h"
#include "Core/MindSandJsonKeys.h"
#include "MindSandLog.h"

UActionDescriptionAsset::UActionDescriptionAsset()
{
}

void UActionDescriptionAsset::PostLoad()
{
    Super::PostLoad();

#if WITH_EDITOR
    // ReflectedParameters is only a serialized snapshot of ActionClass's UPROPERTYs, and nothing else refreshes
    // it except repointing ActionClass. Reconciling on load is what makes a C++ change - a property added,
    // removed, renamed or retyped - reach the asset without an author manually re-picking the class. Author-owned
    // fields are preserved because the class itself has not changed here.
    //
    // Skipped when ActionClass is unset or failed to resolve: there is nothing to reconcile against, and rebuilding
    // from a null class would silently blank the schema of an asset whose class reference is merely broken.
    //
    // Deliberately does not mark the asset dirty. The reconciled list is what GetJSON reads immediately, and it
    // persists whenever the asset is next saved for any other reason; dirtying every action asset on each editor
    // start would be worse than a snapshot that lags on disk. Cooking runs WITH_EDITOR, so cooked data is
    // reconciled too.
    if (ActionClass)
    {
        UpdateReflectedParameters(/*bPreserveAuthoredFields=*/true);
    }
#endif
}

UGameplayTask_Action *UActionDescriptionAsset::CreateActionGameplayTask(UObject *owner)
{
    // Get the Default Object of the TSubclassOf class
    if (ActionClass)
    {
        UGameplayTask_Action *_NewTask = NewObject<UGameplayTask_Action>(owner, ActionClass.Get());
        return _NewTask;
    }
    return nullptr;
}

#if WITH_EDITOR
void UActionDescriptionAsset::PostEditChangeProperty(FPropertyChangedEvent &propertyChangedEvent)
{
    // Guard the pointer actually dereferenced: MemberProperty, not Property. They normally refer to the same
    // field, so the previous mismatched check happened to work, but it protected the wrong pointer.
    const FName changedPropertyName =
        (propertyChangedEvent.MemberProperty ? propertyChangedEvent.MemberProperty->GetFName() : NAME_None);

    if (changedPropertyName == GET_MEMBER_NAME_CHECKED(UActionDescriptionAsset, ActionClass))
    {
        // A different action class means author intent written against the old one no longer applies, so the
        // descriptions and optional flags are reset rather than carried over.
        UpdateReflectedParameters(/*bPreserveAuthoredFields=*/false);
    }

    Super::PostEditChangeProperty(propertyChangedEvent);
}

// The parameter type name advertised to the model for Property, or an empty string for a type that cannot be
// expressed as an action parameter (in which case the property is deliberately left out of the schema).
static FString GetActionParameterTypeName(const FProperty *Property)
{
    if (Property->IsA<FIntProperty>())
    {
        return TEXT("Integer");
    }
    if (Property->IsA<FFloatProperty>() || Property->IsA<FDoubleProperty>())
    {
        return TEXT("Float");
    }
    if (Property->IsA<FBoolProperty>())
    {
        return TEXT("Bool");
    }
    if (Property->IsA<FStrProperty>())
    {
        return TEXT("String");
    }
    if (Property->IsA<FObjectProperty>())
    {
        return TEXT("Actor");
    }
    if (const FArrayProperty *ArrayProp = CastField<FArrayProperty>(Property))
    {
        const FProperty *InnerProp = ArrayProp->Inner;
        if (InnerProp->IsA<FIntProperty>())
        {
            return TEXT("IntegersArray");
        }
        if (InnerProp->IsA<FFloatProperty>() || Property->IsA<FDoubleProperty>())
        {
            return TEXT("FloatsArray");
        }
        if (InnerProp->IsA<FBoolProperty>())
        {
            return TEXT("BoolsArray");
        }
        if (InnerProp->IsA<FStrProperty>())
        {
            return TEXT("StringsArray");
        }
        if (InnerProp->IsA<FObjectProperty>())
        {
            return TEXT("ActorsArray");
        }
    }
    return FString();
}

void UActionDescriptionAsset::UpdateReflectedParameters(bool bPreserveAuthoredFields)
{
    // ActionClass owns which parameters exist and what type each one is, so Name and Type are always taken from
    // the property. ParameterDescription and bIsOptional are author-owned, and whether they survive depends on
    // why we are rebuilding - see the declaration for the two cases.
    //
    // Set aside rather than copied into a lookup: actions declare a handful of parameters, so a linear scan
    // beats allocating a map.
    TArray<FNonEditableActionParameters> PreviousParameters;
    if (bPreserveAuthoredFields)
    {
        PreviousParameters = MoveTemp(ReflectedParameters);
    }

    ReflectedParameters.Empty();
    for (TFieldIterator<FProperty> PropertyIt(ActionClass, EFieldIteratorFlags::ExcludeSuper); PropertyIt; ++PropertyIt)
    {
        FProperty *Property = *PropertyIt;

        if (Property->HasMetaData(TEXT("BlueprintPrivate")) || Property->HasMetaData(TEXT("AllowPrivateAccess")))
        {
            continue;
        }

        // Types with no parameter representation are skipped, so they never reach the model or reflection.
        const FString TypeName = GetActionParameterTypeName(Property);
        if (TypeName.IsEmpty())
        {
            continue;
        }

        const FString VariableName = Property->GetFName().ToString();

        // Carry the author's values across when this parameter existed before. Note Type is deliberately taken
        // from the property, not from the previous entry - retyping a UPROPERTY is exactly the drift this is
        // here to correct.
        const FNonEditableActionParameters *Previous = PreviousParameters.FindByPredicate(
            [&VariableName](const FNonEditableActionParameters &Candidate) { return Candidate.Name == VariableName; });
        if (Previous)
        {
            ReflectedParameters.Add({VariableName, TypeName, Previous->ParameterDescription, Previous->bIsOptional});
            continue;
        }

        // Nothing to carry over: either a parameter that did not exist before, or a full regeneration. The
        // description falls back to the property's ToolTip, and the parameter is advertised as required.
        const FString ToolTip =
            Property->HasMetaData(TEXT("ToolTip")) ? Property->GetToolTipText().ToString() : FString();
        ReflectedParameters.Add({VariableName, TypeName, ToolTip});
    }
}
#endif // WITH_EDITOR

TSharedPtr<FJsonValueObject> UActionDescriptionAsset::GetJSON(bool UseConditionsEffectsAndCost) const
{
    TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
    ResultJson->SetStringField(MindSandJsonKeys::Action::Name, ActionName);
    ResultJson->SetStringField(MindSandJsonKeys::Action::Description, ActionDescription);
    if (UseConditionsEffectsAndCost)
    {
        TSharedPtr<FJsonObject> ConditionsJson = MakeShared<FJsonObject>();
        for (auto &condition : Conditions)
        {
            ConditionsJson->SetBoolField(condition.Key, condition.Value);
        }
        ResultJson->SetObjectField(MindSandJsonKeys::Action::Conditions, ConditionsJson);
        TSharedPtr<FJsonObject> EffectsJson = MakeShared<FJsonObject>();
        for (auto &effect : OutEffects)
        {
            EffectsJson->SetBoolField(effect.Key, effect.Value);
        }
        ResultJson->SetObjectField(MindSandJsonKeys::Action::Effects, EffectsJson);
        ResultJson->SetNumberField(MindSandJsonKeys::Action::Cost, Cost);
    }
    // Both lists are advertised together: the model emits one flat key/value object either way. The split
    // only matters on the receiving side, where reflection writes ReflectedParameters and a ProcessParameters
    // override is responsible for AdditionalParameters.
    TArray<TSharedPtr<FJsonValue>> params;
    params.Reserve(ReflectedParameters.Num() + AdditionalParameters.Num());
    // Generic over the element type: FNonEditableActionParameters and FActionParameters differ only in their
    // UPROPERTY specifiers, and both expose Name / Type / ParameterDescription / bIsOptional.
    auto AppendParameters = [&params](const auto &parameters) {
        for (const auto &parameter : parameters)
        {
            TSharedPtr<FJsonObject> paramsJson = MakeShared<FJsonObject>();
            paramsJson->SetStringField(MindSandJsonKeys::ActionParameter::Name, parameter.Name);
            paramsJson->SetStringField(MindSandJsonKeys::ActionParameter::Type, parameter.Type);
            paramsJson->SetStringField(MindSandJsonKeys::ActionParameter::Description, parameter.ParameterDescription);
            // Advisory: tells the model whether it has to supply this. Nothing validates it on the way back in.
            paramsJson->SetBoolField(MindSandJsonKeys::ActionParameter::IsRequired, !parameter.bIsOptional);
            params.Add(MakeShared<FJsonValueObject>(paramsJson));
        }
    };
    AppendParameters(ReflectedParameters);
    AppendParameters(AdditionalParameters);
    ResultJson->SetArrayField(MindSandJsonKeys::Action::Parameters, params);
    return MakeShared<FJsonValueObject>(ResultJson);
}

TArray<TSharedPtr<FJsonValue>> UActionDescriptionAsset::GetJSON(const TArray<UActionDescriptionAsset *> &actions,
                                                                bool UseConditionsEffectsAndCost)
{
    TArray<TSharedPtr<FJsonValue>> Actions;
    for (auto &action : actions)
    {
        Actions.Add(action->GetJSON());
    }

    return Actions;
}

TArray<TSharedPtr<FJsonValue>> UActionDescriptionAsset::GetJSON(const UActionRegistrySubsystem *ActionRegistry,
                                                                const TArray<FString> &ActionNames,
                                                                bool UseConditionsEffectsAndCost)
{
    TArray<TSharedPtr<FJsonValue>> Result;
    if (!ActionRegistry)
    {
        return Result;
    }
    Result.Reserve(ActionNames.Num());
    for (const FString &Name : ActionNames)
    {
        UActionDescriptionAsset *A = ActionRegistry->FindActionByName(Name);
        if (!A)
        {
            UE_LOG(LogMindSand, Warning, TEXT("ActionDescriptionAsset::GetJSON: no action registered with name '%s'."),
                   *Name);
            continue;
        }
        Result.Add(A->GetJSON(UseConditionsEffectsAndCost));
    }
    return Result;
}

TArray<TSharedPtr<FJsonValue>> UActionDescriptionAsset::GetActionNamesJSON(
    const TArray<UActionDescriptionAsset *> &actions)
{
    TArray<TSharedPtr<FJsonValue>> Actions;
    for (auto &action : actions)
    {
        Actions.Add(MakeShareable(new FJsonValueString(action->ActionName)));
    }
    return Actions;
}
