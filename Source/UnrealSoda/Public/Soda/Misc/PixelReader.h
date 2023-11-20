// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Math/Color.h"


class UTextureRenderTarget2D;
class FRHICommandListImmediate;
class FRHITexture;

 /**
  * FCameraPixelReader
  */
class UNREALSODA_API FCameraPixelReader
{
public:
	void BeginRead(
		UTextureRenderTarget2D& RenderTarget,
		FRHICommandListImmediate& InRHICmdList,
		TArrayView<FColor>& OutPixels, uint32& OutWidth);

	void EndRead();

	static void ReadPixels(
		UTextureRenderTarget2D& RenderTarget,
		FRHICommandListImmediate& InRHICmdList,
		TArray<FColor>& OutPixels,
		uint32& OutWidth);

private:
	TArray<FColor> Pixels;
	FRHITexture* Texture;
};