// Copyright (c) 2024 Electronic Arts. All Rights Reserved.

#include "TextureSetsBlueprintFunctionLibrary.h"

#include "TextureSet.h"
#include "TextureSetDefinition.h"
#include "TextureSetMaterialBindingData.h"
#include "TextureSetsHelpers.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	bool SetTextureOverride(UMaterialInstance* MaterialInstance, const FTextureParameterValue& TextureParameterValue)
	{
		if (UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(MaterialInstance))
		{
			MID->SetTextureParameterValueByInfo(TextureParameterValue.ParameterInfo, TextureParameterValue.ParameterValue);
			return true;
		}

#if WITH_EDITOR
		if (UMaterialInstanceConstant* MIC = Cast<UMaterialInstanceConstant>(MaterialInstance))
		{
			MIC->SetTextureParameterValueEditorOnly(TextureParameterValue.ParameterInfo, TextureParameterValue.ParameterValue);
			return true;
		}
#endif

		return false;
	}

	bool SetVectorOverride(UMaterialInstance* MaterialInstance, const FVectorParameterValue& VectorParameterValue)
	{
		if (UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(MaterialInstance))
		{
			MID->SetVectorParameterValueByInfo(VectorParameterValue.ParameterInfo, VectorParameterValue.ParameterValue);
			return true;
		}

#if WITH_EDITOR
		if (UMaterialInstanceConstant* MIC = Cast<UMaterialInstanceConstant>(MaterialInstance))
		{
			MIC->SetVectorParameterValueEditorOnly(VectorParameterValue.ParameterInfo, VectorParameterValue.ParameterValue);
			return true;
		}
#endif

		return false;
	}
}

void UTextureSetsBlueprintFunctionLibrary::SetTextureSetParameterValue(
	UMaterialInstanceDynamic* MID,
	const FMaterialParameterInfo& ParameterInfo,
	UTextureSet* Value)
{
	if (!IsValid(MID) || !IsValid(Value))
	{
		return;
	}

	ApplyTextureSetParameterToMaterialInstance(MID, ParameterInfo, Value);
}

bool UTextureSetsBlueprintFunctionLibrary::ApplyTextureSetParameterToMaterialInstance(
	UMaterialInstance* MaterialInstance,
	const FMaterialParameterInfo& ParameterInfo,
	UTextureSet* Value,
	bool bUpdateBinding)
{
	if (!IsValid(MaterialInstance) || !IsValid(Value))
	{
		return false;
	}

	const UTextureSet* ExistingTextureSet = TextureSetMaterialBinding::FindBoundTextureSet(MaterialInstance, ParameterInfo, true);
	if (IsValid(ExistingTextureSet) && IsValid(ExistingTextureSet->Definition) && IsValid(Value->Definition) && ExistingTextureSet->Definition != Value->Definition)
	{
		ensureAlwaysMsgf(
			false,
			TEXT("[Set Texture Set Parameter Value] Value = %s (Definition: %s) does not match existing bound definition %s for ParameterInfo = %s on MaterialInstance = %s"),
			*Value->GetName(),
			*Value->Definition->GetName(),
			*ExistingTextureSet->Definition->GetName(),
			*ParameterInfo.ToString(),
			*MaterialInstance->GetName());
		return false;
	}

	if (bUpdateBinding)
	{
		TextureSetMaterialBinding::SetBoundTextureSet(MaterialInstance, ParameterInfo, Value);
	}

	SetDerivedTextureSetParameters(MaterialInstance, ParameterInfo, Value);

#if WITH_EDITOR
	if (UMaterialInstanceConstant* MIC = Cast<UMaterialInstanceConstant>(MaterialInstance))
	{
		MIC->PostEditChange();
	}
#endif

	return true;
}

void UTextureSetsBlueprintFunctionLibrary::SetDerivedTextureSetParameters(
	UMaterialInstance* MaterialInstance,
	const FMaterialParameterInfo& ParameterInfo,
	UTextureSet* NewTextureSetValue)
{
	TArray<FTextureParameterValue> CustomTextureParameterValues;
	TArray<FVectorParameterValue> CustomVectorParameterValues;

	if (NewTextureSetValue != nullptr)
	{
		NewTextureSetValue->BuildMaterialTextureParameters(ParameterInfo, CustomTextureParameterValues);
		NewTextureSetValue->BuildMaterialVectorParameters(ParameterInfo, CustomVectorParameterValues);
	}
	
	for (FTextureParameterValue& TextureParameterValue : CustomTextureParameterValues)
	{
		SetTextureOverride(MaterialInstance, TextureParameterValue);
	}
	
	for (FVectorParameterValue& VectorParameterValue : CustomVectorParameterValues)
	{
		SetVectorOverride(MaterialInstance, VectorParameterValue);
	}
}
