#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

class UMaterialInstanceConstant;
class UTextureSetDefinition;

class FTextureSetMaterialInstanceCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	static UMaterialInstanceConstant* ResolveMaterialInstance(UObject* Object);
};
