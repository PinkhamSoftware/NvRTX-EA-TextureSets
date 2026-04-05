#include "TextureSetMaterialBindingData.h"

#include "Materials/MaterialInstance.h"
#include "TextureSet.h"

const UTextureSet* UTextureSetMaterialBindingData::FindTextureSet(const FMaterialParameterInfo& ParameterInfo) const
{
	const FTextureSetMaterialBinding* FoundBinding = Bindings.FindByPredicate(
		[&ParameterInfo](const FTextureSetMaterialBinding& Binding)
		{
			return Binding.ParameterInfo == ParameterInfo;
		});

	return FoundBinding != nullptr ? FoundBinding->TextureSet.Get() : nullptr;
}

bool UTextureSetMaterialBindingData::SetTextureSet(const FMaterialParameterInfo& ParameterInfo, UTextureSet* InTextureSet)
{
	FTextureSetMaterialBinding* ExistingBinding = Bindings.FindByPredicate(
		[&ParameterInfo](const FTextureSetMaterialBinding& Binding)
		{
			return Binding.ParameterInfo == ParameterInfo;
		});

	if (ExistingBinding)
	{
		if (ExistingBinding->TextureSet == InTextureSet)
		{
			return false;
		}

		ExistingBinding->TextureSet = InTextureSet;
		return true;
	}

	FTextureSetMaterialBinding NewBinding;
	NewBinding.ParameterInfo = ParameterInfo;
	NewBinding.TextureSet = InTextureSet;
	Bindings.Add(NewBinding);
	return true;
}

bool UTextureSetMaterialBindingData::RemoveTextureSet(const FMaterialParameterInfo& ParameterInfo)
{
	const int32 RemovedCount = Bindings.RemoveAll(
		[&ParameterInfo](const FTextureSetMaterialBinding& Binding)
		{
			return Binding.ParameterInfo == ParameterInfo;
		});

	return RemovedCount > 0;
}

namespace TextureSetMaterialBinding
{
	static const UMaterialInterface* GetParentMaterial(const UMaterialInterface* Material)
	{
		const UMaterialInstance* MaterialInstance = Cast<UMaterialInstance>(Material);
		return MaterialInstance != nullptr ? MaterialInstance->Parent.Get() : nullptr;
	}

	UTextureSetMaterialBindingData* FindBindings(UMaterialInterface* Material)
	{
		return Material != nullptr
			? Cast<UTextureSetMaterialBindingData>(Material->GetAssetUserDataOfClass(UTextureSetMaterialBindingData::StaticClass()))
			: nullptr;
	}

	const UTextureSetMaterialBindingData* FindBindings(const UMaterialInterface* Material)
	{
		return Material != nullptr
			? Cast<UTextureSetMaterialBindingData>(const_cast<UMaterialInterface*>(Material)->GetAssetUserDataOfClass(UTextureSetMaterialBindingData::StaticClass()))
			: nullptr;
	}

	UTextureSetMaterialBindingData* FindOrAddBindings(UMaterialInterface* Material)
	{
		if (Material == nullptr)
		{
			return nullptr;
		}

		UTextureSetMaterialBindingData* ExistingData = FindBindings(Material);
		if (ExistingData != nullptr)
		{
			return ExistingData;
		}

		UTextureSetMaterialBindingData* NewData = NewObject<UTextureSetMaterialBindingData>(Material);
		Material->AddAssetUserData(NewData);
		return NewData;
	}

	const UTextureSet* FindBoundTextureSet(const UMaterialInterface* Material, const FMaterialParameterInfo& ParameterInfo, bool bSearchParentChain)
	{
		const UMaterialInterface* CurrentMaterial = Material;
		while (CurrentMaterial != nullptr)
		{
			const UTextureSetMaterialBindingData* BindingData = FindBindings(CurrentMaterial);
			if (BindingData != nullptr)
			{
				const UTextureSet* FoundTextureSet = BindingData->FindTextureSet(ParameterInfo);
				if (FoundTextureSet != nullptr)
				{
					return FoundTextureSet;
				}
			}

			if (!bSearchParentChain)
			{
				break;
			}

			CurrentMaterial = GetParentMaterial(CurrentMaterial);
		}

		return nullptr;
	}

	bool SetBoundTextureSet(UMaterialInterface* Material, const FMaterialParameterInfo& ParameterInfo, UTextureSet* TextureSet)
	{
		if (Material == nullptr)
		{
			return false;
		}

		UTextureSetMaterialBindingData* BindingData = FindOrAddBindings(Material);
		if (BindingData == nullptr)
		{
			return false;
		}

#if WITH_EDITOR
		Material->Modify();
		BindingData->Modify();
#endif

		bool bChanged = false;
		if (TextureSet != nullptr)
		{
			bChanged = BindingData->SetTextureSet(ParameterInfo, TextureSet);
		}
		else
		{
			bChanged = BindingData->RemoveTextureSet(ParameterInfo);
		}

		return bChanged;
	}

	TArray<FTextureSetMaterialBinding> GetAllBindings(const UMaterialInterface* Material, bool bSearchParentChain)
	{
		TMap<FMaterialParameterInfo, FTextureSetMaterialBinding> UniqueBindings;

		const UMaterialInterface* CurrentMaterial = Material;
		while (CurrentMaterial != nullptr)
		{
			const UTextureSetMaterialBindingData* BindingData = FindBindings(CurrentMaterial);
			if (BindingData != nullptr)
			{
				for (const FTextureSetMaterialBinding& Binding : BindingData->Bindings)
				{
					if (!UniqueBindings.Contains(Binding.ParameterInfo))
					{
						UniqueBindings.Add(Binding.ParameterInfo, Binding);
					}
				}
			}

			if (!bSearchParentChain)
			{
				break;
			}

			CurrentMaterial = GetParentMaterial(CurrentMaterial);
		}

		TArray<FTextureSetMaterialBinding> Result;
		UniqueBindings.GenerateValueArray(Result);
		return Result;
	}
}
