// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Misc/PixelReader.h"
#include "Engine/TextureRenderTarget2D.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "TextureResource.h"

void FCameraPixelReader::BeginRead(
	UTextureRenderTarget2D& RenderTarget,
	FRHICommandListImmediate& InRHICmdList,
	TArrayView<FColor>& OutPixels, uint32& OutWidth)
{
	check(IsInRenderingThread());

	Texture = RenderTarget.GetRenderTargetResource()->GetRenderTargetTexture();
	checkf(Texture != nullptr, TEXT("FPixelReader: UTextureRenderTarget2D missing render target texture"));
	const FTextureRenderTarget2DResource* RenderResource = static_cast<const FTextureRenderTarget2DResource*>(RenderTarget.GetResource());

	if (IsVulkanPlatform(GMaxRHIShaderPlatform))
	{
		InRHICmdList.ReadSurfaceData(
			Texture,
			FIntRect(0, 0, RenderResource->GetSizeXY().X, RenderResource->GetSizeXY().Y),
			Pixels,
			FReadSurfaceDataFlags(RCM_UNorm, CubeFace_MAX));
		OutPixels = TArrayView<FColor>(Pixels.GetData(), Pixels.Num());
		OutWidth = RenderResource->GetSizeXY().X;
	}
	else
	{
		uint32 DestStride = 0;
		FColor* Data = (FColor*)RHILockTexture2D(Texture, 0, RLM_ReadOnly, DestStride, false);
		OutPixels = TArrayView<FColor>(Data, DestStride / 4 * RenderResource->GetSizeXY().Y);
		OutWidth = DestStride / 4;
	}
}

void FCameraPixelReader::EndRead()
{
	if (!IsVulkanPlatform(GMaxRHIShaderPlatform))
	{
		RHIUnlockTexture2D(Texture, 0, false);
	}
}

void FCameraPixelReader::ReadPixels(UTextureRenderTarget2D& RenderTarget, FRHICommandListImmediate& InRHICmdList, TArray<FColor>& Pixels, uint32& Width)
{
	check(IsInRenderingThread());

	FRHITexture2D* Texture = RenderTarget.GetRenderTargetResource()->GetRenderTargetTexture();
	const FTextureRenderTarget2DResource* RenderResource = static_cast<const FTextureRenderTarget2DResource*>(RenderTarget.GetResource());
	checkf(Texture != nullptr, TEXT("ReadPixels(). UTextureRenderTarget2D missing render target texture"));

	if (IsVulkanPlatform(GMaxRHIShaderPlatform))
	{
		InRHICmdList.ReadSurfaceData(
			Texture,
			FIntRect(0, 0, RenderResource->GetSizeXY().X, RenderResource->GetSizeXY().Y),
			Pixels,
			FReadSurfaceDataFlags(RCM_UNorm, CubeFace_MAX));
		Width = RenderResource->GetSizeXY().X;
	}
	else
	{
		uint32 DestStride = 0;
		void* CpuDataPtr = (void*)RHILockTexture2D(Texture, 0, RLM_ReadOnly, DestStride, false);
		Pixels.SetNum(DestStride * RenderResource->GetSizeXY().Y / 4);
		FMemory::BigBlockMemcpy(Pixels.GetData(), CpuDataPtr, Pixels.Num() * 4);
		RHIUnlockTexture2D(Texture, 0, false);
		Width = DestStride / 4;
	}
}
