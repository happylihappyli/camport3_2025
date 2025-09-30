#include "funny_resize.hpp"
#include <algorithm>

/**
 * @brief 最近邻插值 - 8位图像
 */
static void resize_nearest_neighbor(
    int src_width, int src_height, const uint8_t* src_data, int src_channels,
    int dst_width, int dst_height, uint8_t* dst_data
) {
    float x_ratio = static_cast<float>(src_width) / dst_width;
    float y_ratio = static_cast<float>(src_height) / dst_height;
    
    for (int y = 0; y < dst_height; y++) {
        for (int x = 0; x < dst_width; x++) {
            // 计算源图像中的对应位置
            int src_x = static_cast<int>(x * x_ratio + 0.5f);
            int src_y = static_cast<int>(y * y_ratio + 0.5f);
            
            // 确保不越界
            src_x = std::min(src_width - 1, std::max(0, src_x));
            src_y = std::min(src_height - 1, std::max(0, src_y));
            
            // 复制像素值到目标图像
            const uint8_t* src_pixel = src_data + (src_y * src_width + src_x) * src_channels;
            uint8_t* dst_pixel = dst_data + (y * dst_width + x) * src_channels;
            
            for (int c = 0; c < src_channels; c++) {
                dst_pixel[c] = src_pixel[c];
            }
        }
    }
}

/**
 * @brief 双线性插值 - 8位图像
 */
static void resize_bilinear(
    int src_width, int src_height, const uint8_t* src_data, int src_channels,
    int dst_width, int dst_height, uint8_t* dst_data
) {
    float x_ratio = static_cast<float>(src_width - 1) / dst_width;
    float y_ratio = static_cast<float>(src_height - 1) / dst_height;
    
    for (int y = 0; y < dst_height; y++) {
        for (int x = 0; x < dst_width; x++) {
            // 计算源图像中的浮点坐标
            float src_x = x * x_ratio;
            float src_y = y * y_ratio;
            
            // 找到四个相邻点
            int x0 = static_cast<int>(src_x);
            int y0 = static_cast<int>(src_y);
            int x1 = std::min(src_width - 1, x0 + 1);
            int y1 = std::min(src_height - 1, y0 + 1);
            
            // 计算权重
            float fx = src_x - x0;
            float fy = src_y - y0;
            float fx1 = 1.0f - fx;
            float fy1 = 1.0f - fy;
            
            // 双线性插值
            uint8_t* dst_pixel = dst_data + (y * dst_width + x) * src_channels;
            
            for (int c = 0; c < src_channels; c++) {
                const uint8_t* p00 = src_data + (y0 * src_width + x0) * src_channels + c;
                const uint8_t* p01 = src_data + (y0 * src_width + x1) * src_channels + c;
                const uint8_t* p10 = src_data + (y1 * src_width + x0) * src_channels + c;
                const uint8_t* p11 = src_data + (y1 * src_width + x1) * src_channels + c;
                
                float val = (*p00) * fx1 * fy1 + (*p01) * fx * fy1 + (*p10) * fx1 * fy + (*p11) * fx * fy;
                dst_pixel[c] = static_cast<uint8_t>(std::min(255.0f, std::max(0.0f, val)));
            }
        }
    }
}

/**
 * @brief 8位图像resize函数实现
 */
void funny_resize(
    int src_width, int src_height, const uint8_t* src_data, int src_channels,
    int dst_width, int dst_height, uint8_t* dst_data, 
    InterpolationMethod interpolation
) {
    if (!src_data || !dst_data || src_width <= 0 || src_height <= 0 || 
        dst_width <= 0 || dst_height <= 0 || src_channels <= 0) {
        return; // 参数无效
    }
    
    if (interpolation == INTER_NEAREST) {
        resize_nearest_neighbor(src_width, src_height, src_data, src_channels, 
                               dst_width, dst_height, dst_data);
    } else {
        // 默认使用双线性插值
        resize_bilinear(src_width, src_height, src_data, src_channels, 
                       dst_width, dst_height, dst_data);
    }
}

/**
 * @brief 16位图像resize函数实现（主要用于深度图）
 */
void funny_resize_16bit(
    int src_width, int src_height, const uint16_t* src_data,
    int dst_width, int dst_height, uint16_t* dst_data, 
    InterpolationMethod interpolation
) {
    if (!src_data || !dst_data || src_width <= 0 || src_height <= 0 || 
        dst_width <= 0 || dst_height <= 0) {
        return; // 参数无效
    }
    
    float x_ratio = static_cast<float>(src_width - 1) / dst_width;
    float y_ratio = static_cast<float>(src_height - 1) / dst_height;
    
    if (interpolation == INTER_NEAREST) {
        // 最近邻插值
        for (int y = 0; y < dst_height; y++) {
            for (int x = 0; x < dst_width; x++) {
                int src_x = static_cast<int>(x * x_ratio + 0.5f);
                int src_y = static_cast<int>(y * y_ratio + 0.5f);
                
                src_x = std::min(src_width - 1, std::max(0, src_x));
                src_y = std::min(src_height - 1, std::max(0, src_y));
                
                dst_data[y * dst_width + x] = src_data[src_y * src_width + src_x];
            }
        }
    } else {
        // 双线性插值
        for (int y = 0; y < dst_height; y++) {
            for (int x = 0; x < dst_width; x++) {
                float src_x = x * x_ratio;
                float src_y = y * y_ratio;
                
                int x0 = static_cast<int>(src_x);
                int y0 = static_cast<int>(src_y);
                int x1 = std::min(src_width - 1, x0 + 1);
                int y1 = std::min(src_height - 1, y0 + 1);
                
                float fx = src_x - x0;
                float fy = src_y - y0;
                float fx1 = 1.0f - fx;
                float fy1 = 1.0f - fy;
                
                uint16_t p00 = src_data[y0 * src_width + x0];
                uint16_t p01 = src_data[y0 * src_width + x1];
                uint16_t p10 = src_data[y1 * src_width + x0];
                uint16_t p11 = src_data[y1 * src_width + x1];
                
                float val = p00 * fx1 * fy1 + p01 * fx * fy1 + p10 * fx1 * fy + p11 * fx * fy;
                dst_data[y * dst_width + x] = static_cast<uint16_t>(val);
            }
        }
    }
}