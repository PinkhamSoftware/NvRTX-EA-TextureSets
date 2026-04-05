#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetUserData.h"
#include "Materials/MaterialParameters.h"
#include "TextureSetMaterialBindingData.generated.h"

class UMaterialInterface;
class UTextureSet;

USTRUCT()
struct FTextureSetMaterialBinding
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Texture Sets")
	FMaterialParameterInfo ParameterInfo;

	UPROPERTY(EditAnywhere, Category = "Texture Sets")
	TObjectPtr<UTextureSet> TextureSet = nullptr;
};

UCLASS(BlueprintType)
class TEXTURESETS_API UTextureSetMaterialBindingData : public UAssetUserData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Texture Sets")
	TArray<FTextureSetMaterialBinding> Bindings;

	const UTextureSet* FindTextureSet(const FMaterialParameterInfo& ParameterInfo) const;
	bool SetTextureSet(const FMaterialParameterInfo& ParameterInfo, UTextureSet* InTextureSet);
	bool RemoveTextureSet(const FMaterialParameterInfo& ParameterInfo);
};

namespace TextureSetMaterialBinding
{
	TEXTURESETS_API UTextureSetMaterialBindingData* FindBindings(UMaterialInterface* Material);
	TEXTURESETS_API const UTextureSetMaterialBindingData* FindBindings(const UMaterialInterface* Material);
	TEXTURESETS_API UTextureSetMaterialBindingData* FindOrAddBindings(UMaterialInterface* Material);
	TEXTURESETS_API const UTextureSet* FindBoundTextureSet(const UMaterialInterface* Material, const FMaterialParameterInfo& ParameterInfo, bool bSearchParentChain = true);
	TEXTURESETS_API bool SetBoundTextureSet(UMaterialInterface* Material, const FMaterialParameterInfo& ParameterInfo, UTextureSet* TextureSet);
	TEXTURESETS_API TArray<FTextureSetMaterialBinding> GetAllBindings(const UMaterialInterface* Material, bool bSearchParentChain = true);
}
