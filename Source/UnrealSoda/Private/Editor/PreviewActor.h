// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PreviewActor.generated.h"

class UMaterialInterface;
class FPrimitiveDrawInterface;
class UClass;

UCLASS(MinimalAPI)
class APreviewActor : public AActor
{
	GENERATED_UCLASS_BODY()

public:

	virtual void RenderTarget(FPrimitiveDrawInterface* PDI);
	virtual bool SetPreviwActorClass(const UClass* Class) { return false; } // TODO

protected:
	UMaterialInterface* TransparentMaterial;
	UMaterialInterface* GridMaterial;

	float RenderScale = 1.0;
};



