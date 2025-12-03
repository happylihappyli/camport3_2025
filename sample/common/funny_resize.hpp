#ifndef FUNNY_RESIZE_HPP_
#define FUNNY_RESIZE_HPP_

#include <stdint.h>

enum class InterpolationMethod {
    NEAREST,
    LINEAR
};

void funny_resize(
    int src_width, int src_height, const uint8_t* src_data, int src_channels,
    int dst_width, int dst_height, uint8_t* dst_data, 
    InterpolationMethod interpolation = InterpolationMethod::LINEAR
);

void funny_resize_16bit(
    int src_width, int src_height, const uint16_t* src_data,
    int dst_width, int dst_height, uint16_t* dst_data, 
    InterpolationMethod interpolation = InterpolationMethod::NEAREST
);

#endif