#ifndef FUNNY_RESIZE_HPP_
#define FUNNY_RESIZE_HPP_

#include <stdint.h>
#include <vector>

/**
 * 图像插值方法枚举
 */
enum InterpolationMethod {
    INTER_NEAREST,  // 最近邻插值
    INTER_LINEAR    // 双线性插值
};

/**
 * @brief 图像resize函数，替代OpenCV的cv::resize
 * @param src_width 源图像宽度 
 * @param src_height 源图像高度
 * @param src_data 源图像数据
 * @param src_channels 源图像通道数
 * @param dst_width 目标图像宽度
 * @param dst_height 目标图像高度
 * @param dst_data 目标图像数据（需要预先分配足够空间）
 * @param interpolation 插值方法
 */
void funny_resize(
    int src_width, int src_height, const uint8_t* src_data, int src_channels,
    int dst_width, int dst_height, uint8_t* dst_data, 
    InterpolationMethod interpolation = INTER_LINEAR
);

/**
 * @brief 16位图像resize函数，用于深度图等16位图像
 * @param src_width 源图像宽度
 * @param src_height 源图像高度
 * @param src_data 源图像数据
 * @param dst_width 目标图像宽度
 * @param dst_height 目标图像高度
 * @param dst_data 目标图像数据（需要预先分配足够空间）
 * @param interpolation 插值方法
 */
void funny_resize_16bit(
    int src_width, int src_height, const uint16_t* src_data,
    int dst_width, int dst_height, uint16_t* dst_data, 
    InterpolationMethod interpolation = INTER_NEAREST
);

#endif // FUNNY_RESIZE_HPP_