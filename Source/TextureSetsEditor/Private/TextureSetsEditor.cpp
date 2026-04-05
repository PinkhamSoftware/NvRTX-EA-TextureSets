// Copyright (c) 2024 Electronic Arts. All Rights Reserved.

#include "TextureSetsEditor.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "AssetTypeActions/AssetTypeActions_TextureSet.h"
#include "AssetTypeActions/AssetTypeActions_TextureSetDefinition.h"
#include "Editor.h"
#include "MaterialEditor/MaterialEditorInstanceConstant.h"
#include "Materials/MaterialInstanceConstant.h"
#include "TextureSet.h"
#include "TextureSetAssetParamsCollectionCustomization.h"
#include "TextureSetMaterialInstanceCustomization.h"
#include "TextureSetSourceTextureReferenceCustomization.h"
#include "TextureSetThumbnailRenderer.h"
#include "TextureSetsHelpers.h"
#include "UObject/Object.h"
#include "UObject/AssetRegistryTagsContext.h"
#include "PropertyEditorModule.h"
#include "Subsystems/ImportSubsystem.h"

#define LOCTEXT_NAMESPACE "FTextureSetsModule"

void FTextureSetsEditorModule::StartupModule()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("TextureSets")), LOCTEXT("TextureSetsAssetCategory", "Texture Sets") );

	RegisterAssetTypeAction(AssetTools, MakeShareable(new FAssetTypeActions_TextureSet));
	RegisterAssetTypeAction(AssetTools, MakeShareable(new FAssetTypeActions_TextureSetDefinition));

	RegisterCustomizations();

	OnGetExtraObjectTagsDelegateHandle = UObject::FAssetRegistryTag::OnGetExtraObjectTagsWithContext.AddStatic(&FTextureSetsEditorModule::OnGetExtraObjectTagsWithContext);

	// Register delegates during PostEngineInit as GEditor is not valid yet
	OnPostEngineInitDelegateHandle = FCoreDelegates::OnPostEngineInit.AddLambda([this]()
	{
		if (GEditor)
		{
			OnAssetPostImportDelegateHandle = GEditor->GetEditorSubsystem<UImportSubsystem>()->OnAssetPostImport.AddStatic(&FTextureSetsEditorModule::OnAssetPostImport);
		}
	});

	UThumbnailManager::Get().RegisterCustomRenderer(UTextureSet::StaticClass(), UTextureSetThumbnailRenderer::StaticClass());
}

void FTextureSetsEditorModule::ShutdownModule()
{
	UnregisterAssetTypeActions();
	UnregisterCustomizations();

	UObject::FAssetRegistryTag::OnGetExtraObjectTagsWithContext.Remove(OnGetExtraObjectTagsDelegateHandle);
	
	FCoreDelegates::OnPostEngineInit.Remove(OnPostEngineInitDelegateHandle);

	if (GEditor)
	{
		GEditor->GetEditorSubsystem<UImportSubsystem>()->OnAssetPostImport.Remove(OnAssetPostImportDelegateHandle);
	}

	if (UObjectInitialized())
	{
		UThumbnailManager::Get().UnregisterCustomRenderer(UTextureSet::StaticClass());
	}
}

void FTextureSetsEditorModule::RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action)
{
	AssetTools.RegisterAssetTypeActions(Action);
	RegisteredAssetTypeActions.Add(Action);
}

void FTextureSetsEditorModule::UnregisterAssetTypeActions()
{
	FAssetToolsModule* AssetToolsModule = FModuleManager::GetModulePtr<FAssetToolsModule>("AssetTools");

	if (AssetToolsModule != nullptr)
	{
		IAssetTools& AssetTools = AssetToolsModule->Get();

		for (const TSharedRef<IAssetTypeActions>& Action : RegisteredAssetTypeActions)
		{
			AssetTools.UnregisterAssetTypeActions(Action);
		}

		RegisteredAssetTypeActions.Empty();
	}
}

void FTextureSetsEditorModule::RegisterCustomizations()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	PropertyModule.RegisterCustomPropertyTypeLayout(FTextureSetAssetParamsCollectionCustomization::GetPropertyTypeName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTextureSetAssetParamsCollectionCustomization::MakeInstance));
	PropertyModule.RegisterCustomPropertyTypeLayout(FTextureSetSourceTextureReferenceCustomization::GetPropertyTypeName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTextureSetSourceTextureReferenceCustomization::MakeInstance));
	PropertyModule.RegisterCustomClassLayout(
		UMaterialInstanceConstant::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FTextureSetMaterialInstanceCustomization::MakeInstance));
	PropertyModule.RegisterCustomClassLayout(
		UMaterialEditorInstanceConstant::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FTextureSetMaterialInstanceCustomization::MakeInstance));
}

void FTextureSetsEditorModule::UnregisterCustomizations()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	PropertyModule.UnregisterCustomPropertyTypeLayout(FTextureSetAssetParamsCollectionCustomization::GetPropertyTypeName());
	PropertyModule.UnregisterCustomPropertyTypeLayout(FTextureSetSourceTextureReferenceCustomization::GetPropertyTypeName());
	PropertyModule.UnregisterCustomClassLayout(UMaterialInstanceConstant::StaticClass()->GetFName());
	PropertyModule.UnregisterCustomClassLayout(UMaterialEditorInstanceConstant::StaticClass()->GetFName());
}

void FTextureSetsEditorModule::OnGetExtraObjectTagsWithContext(FAssetRegistryTagsContext Context)
{
	if (const UTexture* Texture = Cast<UTexture>(Context.GetObject()))
	{
		// Add a string with the ID of our source texture to the asset data, so it can be checked for change without having to deserialize the whole asset.
		FString IdString;
		if (TextureSetsHelpers::GetSourceDataIdAsString(Texture, IdString))
		{
			Context.AddTag(UObject::FAssetRegistryTag(TextureSetsHelpers::TextureBulkDataIdAssetTagName, IdString, UObject::FAssetRegistryTag::TT_Hidden));
		}
	}
}

void FTextureSetsEditorModule::OnAssetPostImport(UFactory* ImportFactory, UObject* InObject)
{
	if (IsValid(InObject) && InObject->IsA<UTexture>())
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));
		IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

		TArray<FAssetDependency> Referencers;
		AssetRegistry.GetReferencers(InObject->GetPackage()->GetFName(), Referencers);

		for (const FAssetDependency& Dep : Referencers)
		{
			TArray<FAssetData> AssetsInPackage;
			AssetRegistry.GetAssetsByPackageName(Dep.AssetId.PackageName, AssetsInPackage, false);

			for (const FAssetData& AssetData : AssetsInPackage)
			{
				if (AssetData.IsAssetLoaded() && AssetData.IsInstanceOf(UTextureSet::StaticClass()))
				{
					UTextureSet* TS = Cast<UTextureSet>(AssetData.GetAsset());
					TS->UpdateDerivedData(true);
				}
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FTextureSetsEditorModule, TextureSetsEditor)
