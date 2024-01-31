#pragma once

#include <cstdint>

namespace soda 
{
namespace sim
{
namespace proto_v1 
{

enum class ImageCodecFormat : std::int32_t { Raw = 0, JPEG };

/**
 * Image tensor message header for serialization / deserialization.
 * One image package contain the tensor header structure and raw image data.
 * The raw image data size = width * height * channels * channel_data_type_size.
 * Size of one package = size_of_hedaer + size_of_raw_image_data
 * Package sends via ZMQ protocol
 */
struct TensorMsgHeader 
{
    union TenserUnion 
    {
        std::int32_t data[4];
        struct 
        {
            std::int32_t depth; /* alwais = 1 */
            std::int32_t height;
            std::int32_t width;
            std::int32_t channels; /* count of image channels (for RGB image channels = 3) */
        } shape;
    } tenser;
    std::int32_t dtype; /* channel data type from OpenCV (CV_8U=0, CV_8S=1, CV_16U=2, CV_16S=3, CV_32S=4, CV_32F=5, CV_64F=6) */
    std::int64_t timestamp; /* [ms] */
    std::int64_t index;
};

static_assert(sizeof(TensorMsgHeader) == 10*4, "TensorMsgHeader has unexpected size");

/**
 * Encoded image tensor header for serialization / deserialization (not supported).
 */
struct TensorEncodedMsgHeader 
{
    union TenserUnion 
    {
        std::int32_t data[4];
        struct 
        {
            std::int32_t depth;
            std::int32_t height;
            std::int32_t width;
            std::int32_t channels;
        } shape;
    } tenser;
    std::int32_t dtype;
    std::int64_t timestamp;
    std::int64_t index;
    ImageCodecFormat codec;
    std::uint32_t data_size;
};

} // namespace sim
} // namespace proto_v1
} // namespace soda
