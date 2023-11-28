// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Transport/CameraPublisher.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Async/ParallelFor.h"
#include <errno.h>

namespace soda
{
	uint8 GetDataTypeSize(EDataType CVType)
	{
		static const uint8 CV_2_SIZE[7] = { 1, 1, 2, 2, 4, 4, 8 };
		return CV_2_SIZE[uint8(CVType)];
	}


	void ColorToRawBuffer(const TArray<FColor>& ColorBuf, const FCameraFrame& Frame, uint8* DstBuf)
	{
		switch (Frame.GetShader())
		{
		case ECameraSensorShader::Segm8:
			ParallelFor(Frame.Height, [&](int32 i)
			{
				for (uint32 j = 0; j < Frame.Width; j++)
				{
					DstBuf[i * Frame.Width + j] = ColorBuf[i * Frame.ImageStride + j].R;  //Only R cnannel
				}
			});
			break;

		case ECameraSensorShader::ColorBGR8:
		case ECameraSensorShader::SegmBGR8:
		case ECameraSensorShader::HdrRGB8:
		case ECameraSensorShader::CFA:
			ParallelFor(Frame.Height, [&](int32 i)
			{
				for (uint32 j = 0; j < Frame.Width; j++)
				{
					uint32 out_ind = (i * Frame.Width + j) * 3;
					uint32 in_ind = (i * Frame.ImageStride + j);
					*(uint32*)&DstBuf[out_ind + 0] = *(uint32*)&ColorBuf[in_ind + 0];
				}
			});
			break;


		case ECameraSensorShader::DepthFloat32:
			ParallelFor(Frame.Height, [&](int32 i)
			{
				for (uint32 j = 0; j < Frame.Width; j++)
				{
					((float*)DstBuf)[i * Frame.Width + j] = Rgba2Float(ColorBuf[Frame.ImageStride * i + j]) * Frame.MaxDepthDistance;
				}
			});
			break;


		case ECameraSensorShader::Depth16:
			ParallelFor(Frame.Height, [&](int32 i)
			{
				for (uint32 j = 0; j < Frame.Width; j++)
				{
					((uint16_t*)DstBuf)[i * Frame.Width + j] = static_cast<uint16_t>(Rgba2Float(ColorBuf[Frame.ImageStride * i + j]) * 0xFFFF + 0.5f);
				}
			});
			break;

		case ECameraSensorShader::Depth8:
			ParallelFor(Frame.Height, [&](int32 i)
			{
				for (uint32 j = 0; j < Frame.Width; j++)
				{
					DstBuf[i * Frame.Width + j] = static_cast<uint8_t>(Rgba2Float(ColorBuf[Frame.ImageStride * i + j]) * 0xFF + 0.5f);
				}
			});
			break;
		}
	}
}


FCameraFrame::FCameraFrame()
{
	SetShader(ECameraSensorShader::ColorBGR8);
}

FCameraFrame::FCameraFrame(ECameraSensorShader InShader)
{
	SetShader(InShader);
}

void FCameraFrame::SetShader(ECameraSensorShader InShader)
{
	switch (Shader)
	{
	case ECameraSensorShader::SegmBGR8:
	case ECameraSensorShader::ColorBGR8:
	case ECameraSensorShader::HdrRGB8:
	case ECameraSensorShader::CFA:
		DType = EDataType::CV_8U;
		Channels = 3;
		break;

	case ECameraSensorShader::Segm8:
	case ECameraSensorShader::Depth8:
		DType = EDataType::CV_8U;
		Channels = 1;
		break;

	case ECameraSensorShader::DepthFloat32:
		DType = EDataType::CV_32F;
		Channels = 1;
		break;

	case ECameraSensorShader::Depth16:
		DType = EDataType::CV_16U;
		Channels = 1;
		break;

	default:
		check(0);
	}
}

uint32 FCameraFrame::ComputeRawBufferSize() const
{
	return soda::GetDataTypeSize(GetDataType()) * GetChannels() * Height * Width;
}

UCameraPublisher::UCameraPublisher(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{}
