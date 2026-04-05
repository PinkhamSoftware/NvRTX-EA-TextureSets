#include "TextureSetMaterialInstanceCustomization.h"

#include "AssetRegistry/AssetData.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "MaterialEditor/MaterialEditorInstanceConstant.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "MaterialExpressionTextureSetSampleParameter.h"
#include "PropertyCustomizationHelpers.h"
#include "TextureSet.h"
#include "TextureSetDefinition.h"
#include "TextureSetMaterialBindingData.h"
#include "TextureSetsBlueprintFunctionLibrary.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<IDetailCustomization> FTextureSetMaterialInstanceCustomization::MakeInstance()
{
	return MakeShared<FTextureSetMaterialInstanceCustomization>();
}

UMaterialInstanceConstant* FTextureSetMaterialInstanceCustomization::ResolveMaterialInstance(UObject* Object)
{
	if (UMaterialInstanceConstant* MIC = Cast<UMaterialInstanceConstant>(Object))
	{
		return MIC;
	}

	if (UMaterialEditorInstanceConstant* EditorMIC = Cast<UMaterialEditorInstanceConstant>(Object))
	{
		return Cast<UMaterialInstanceConstant>(EditorMIC->GetMaterialInterface());
	}

	return nullptr;
}

void FTextureSetMaterialInstanceCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> CustomizedObjects;
	DetailBuilder.GetObjectsBeingCustomized(CustomizedObjects);
	if (CustomizedObjects.Num() != 1 || !CustomizedObjects[0].IsValid())
	{
		return;
	}

	UMaterialInstanceConstant* MaterialInstance = ResolveMaterialInstance(CustomizedObjects[0].Get());
	if (!IsValid(MaterialInstance))
	{
		return;
	}

	UMaterial* BaseMaterial = MaterialInstance->GetMaterial();
	if (!IsValid(BaseMaterial))
	{
		return;
	}

	TArray<UMaterialExpressionTextureSetSampleParameter*> TextureSetParameters;
	BaseMaterial->GetAllExpressionsInMaterialAndFunctionsOfType(TextureSetParameters);

	if (TextureSetParameters.IsEmpty())
	{
		return;
	}

	IDetailCategoryBuilder& TextureSetCategory = DetailBuilder.EditCategory("Texture Sets");

	for (UMaterialExpressionTextureSetSampleParameter* Expression : TextureSetParameters)
	{
		if (!IsValid(Expression))
		{
			continue;
		}

		const FMaterialParameterInfo ParameterInfo(Expression->ParameterName, EMaterialParameterAssociation::GlobalParameter, INDEX_NONE);
		const FText ParameterNameText = FText::FromName(Expression->ParameterName);
		TWeakObjectPtr<UMaterialInstanceConstant> WeakMaterialInstance = MaterialInstance;
		TWeakObjectPtr<UTextureSetDefinition> WeakDefinition = Expression->Definition;

		TextureSetCategory.AddCustomRow(ParameterNameText)
		.NameContent()
		[
			SNew(STextBlock)
			.Text(ParameterNameText)
		]
		.ValueContent()
		.MinDesiredWidth(250.0f)
		[
			SNew(SObjectPropertyEntryBox)
			.AllowedClass(UTextureSet::StaticClass())
			.ObjectPath_Lambda([WeakMaterialInstance, ParameterInfo]() -> FString
			{
				if (!WeakMaterialInstance.IsValid())
				{
					return FString();
				}

				const UTextureSet* BoundTextureSet = TextureSetMaterialBinding::FindBoundTextureSet(WeakMaterialInstance.Get(), ParameterInfo, true);
				return IsValid(BoundTextureSet) ? BoundTextureSet->GetPathName() : FString();
			})
			.OnShouldFilterAsset_Lambda([WeakDefinition](const FAssetData& AssetData) -> bool
			{
				if (!WeakDefinition.IsValid())
				{
					return false;
				}

				const FAssetTagValueRef DefinitionIdValue = AssetData.TagsAndValues.FindTag("TextureSetDefinitionID");
				if (DefinitionIdValue.IsSet())
				{
					return DefinitionIdValue.AsString() != WeakDefinition->GetGuid().ToString();
				}

				const UTextureSet* TextureSetAsset = Cast<UTextureSet>(AssetData.GetAsset());
				return !IsValid(TextureSetAsset) || TextureSetAsset->Definition != WeakDefinition.Get();
			})
			.OnObjectChanged_Lambda([WeakMaterialInstance, WeakDefinition, ParameterInfo](const FAssetData& AssetData)
			{
				if (!WeakMaterialInstance.IsValid())
				{
					return;
				}

				UTextureSet* TextureSetAsset = Cast<UTextureSet>(AssetData.GetAsset());
				if (!IsValid(TextureSetAsset))
				{
					TextureSetMaterialBinding::SetBoundTextureSet(WeakMaterialInstance.Get(), ParameterInfo, nullptr);
					return;
				}

				if (WeakDefinition.IsValid() && TextureSetAsset->Definition != WeakDefinition.Get())
				{
					return;
				}

				if (UTextureSetsBlueprintFunctionLibrary::ApplyTextureSetParameterToMaterialInstance(WeakMaterialInstance.Get(), ParameterInfo, TextureSetAsset))
				{
					WeakMaterialInstance->MarkPackageDirty();
				}
			})
		];
	}
}
