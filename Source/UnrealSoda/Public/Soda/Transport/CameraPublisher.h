// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/SodaTypes.h"
#include "Soda/Misc/Time.h"
#include "CameraPublisher.generated.h"

/** 
 * OpenCV compatible data types 
 */
enum class EDataType : uint8
{
	CV_8U = 0,
	CV_8S,
	CV_16U,
	CV_16S,
	CV_32S,
	CV_32F,
	CV_64F,
};

/**
 * ECameraSensorShader indicates which PostProcess shader (material) is applied to USceneCaptureComponent
 */
UENUM(BlueprintType)
enum class ECameraSensorShader : uint8
{
	ColorBGR8 = 0,
	Depth8 = 2,
	DepthFloat32 = 3,
	Depth16 = 4,
	SegmBGR8 = 5,
	Segm8 = 6,
	HdrRGB8 = 7,

	/** Color Filter Array.See https ://en.wikipedia.org/wiki/Color_filter_array */
	CFA = 8, 
};

/**
 *	Descriptor for the texture (frame) captured from a USceneCaptureComponent
 */
struct UNREALSODA_API FCameraFrame
{
	FCameraFrame();
	FCameraFrame(ECameraSensorShader InShader);

	uint32 Height = 0;
	uint32 Width = 0;

	/**  Valid only if Shader == DepthFloat32, [m] */
	float MaxDepthDistance = 0;
	int64 Index = 0;
	TTimestamp Timestamp{};

	/** Original texture width in pixels */
	uint32 ImageStride = 0;

	void SetShader(ECameraSensorShader Shader);
	ECameraSensorShader GetShader() const { return Shader; }
	EDataType GetDataType() const { return DType; }
	uint8 GetChannels() const { return Channels; }

	uint32 ComputeRawBufferSize() const;

protected:
	ECameraSensorShader Shader;
	EDataType DType;
	uint8 Channels;

};

namespace soda
{
	/** [in bytes] */
	UNREALSODA_API uint8 GetDataTypeSize(EDataType CVType);

	/** Be sure size of DstBuf >= FCameraFrame::ComputeRawBufferSize(Frame) */
	UNREALSODA_API void ColorToRawBuffer(const TArray<FColor>& Color, const FCameraFrame& Frame, uint8* DstBuf);
}

/**
 *	UCameraPublisher
 */
UCLASS(abstract, ClassGroup = Soda)
class UNREALSODA_API UCameraPublisher : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	virtual bool Advertise() { return false; }
	virtual void Shutdown() {}
	virtual void Publish(const void* ImgBGRA8Ptr, const FCameraFrame& CameraFrame) {}
	virtual bool IsInitializing() const { return false; }
	virtual bool IsOk() const { return false; }
	virtual TArray<FColor>& LockBuffer() { static TArray<FColor> Dummy; return Dummy; }
	virtual void UnlockBuffer(const FCameraFrame& CameraFrame) {}
	virtual ~UCameraPublisher() {}
};
