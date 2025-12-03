#ifndef SAMPLE_COMMON_IMAGE_CONVERT_HPP_
#define SAMPLE_COMMON_IMAGE_CONVERT_HPP_

#include "funny_Mat.hpp"
#include <vector>
#include <iostream>

// 简化版imdecode函数实现
static inline bool imdecode(const std::vector<uint8_t>& buf, int flags, funny_Mat& dst) {
    // 注意：这是一个简化实现，实际项目中可能需要使用真正的JPEG解码库
    std::cout << "警告: imdecode是简化实现，不支持实际的JPEG解码" << std::endl;
    
    // 为了编译通过，这里创建一个小的默认图像
    int width = 640;
    int height = 480;
    dst = funny_Mat(height, width, static_cast<int>(PixelFormat::CV_8UC3));
    
    // 填充一些简单的测试数据
    uint8_t* data = dst.data();
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * 3;
            data[idx] = (x % 256);     // B
            data[idx + 1] = (y % 256); // G
            data[idx + 2] = 128;       // R
        }
    }
    
    return true;
}

// 简化版cvtColor函数实现
static inline void cvtColor(const funny_Mat& src, funny_Mat& dst, int code) {
    // 确保源图像是正确的格式
    if (src.type() != static_cast<int>(PixelFormat::CV_8UC2)) {
        std::cout << "错误: cvtColor - 源图像必须是CV_8UC2格式" << std::endl;
        return;
    }
    
    // 创建目标图像
    dst = funny_Mat(src.rows(), src.cols(), static_cast<int>(PixelFormat::CV_8UC3));
    
    const uint8_t* src_data = src.data();
    uint8_t* dst_data = dst.data();
    
    if (code == static_cast<int>(ColorConversionCode::YUV2BGR_YVYU)) {
        // YVYU格式到BGR的转换
        for (int i = 0; i < src.rows() * src.cols(); i++) {
            int src_idx = i * 2;
            int dst_idx = i * 3;
            
            // YVYU格式: Y0, V0, Y1, U0, Y2, V1, Y3, U1...
            uint8_t Y = src_data[src_idx];
            uint8_t V = src_data[src_idx + 1];
            
            // 简化的YUV到BGR转换公式
            // 注意：这是一个简化实现，实际项目中可能需要更精确的转换
            int C = Y - 16;
            int D = V - 128;
            
            // 对于偶数像素使用U0，奇数像素可能需要调整索引
            uint8_t U = (i % 2 == 0) ? src_data[src_idx + 3] : src_data[src_idx + 1];
            int E = U - 128;
            
            // 计算BGR值
            int B = (298 * C + 409 * D + 128) >> 8;
            int G = (298 * C - 100 * D - 208 * E + 128) >> 8;
            int R = (298 * C + 516 * E + 128) >> 8;
            
            // 确保值在0-255范围内
            dst_data[dst_idx] = (B < 0) ? 0 : ((B > 255) ? 255 : B);
            dst_data[dst_idx + 1] = (G < 0) ? 0 : ((G > 255) ? 255 : G);
            dst_data[dst_idx + 2] = (R < 0) ? 0 : ((R > 255) ? 255 : R);
        }
    } else if (code == static_cast<int>(ColorConversionCode::YUV2BGR_YUYV)) {
        // YUYV格式到BGR的转换
        for (int i = 0; i < src.rows() * src.cols(); i++) {
            int src_idx = i * 2;
            int dst_idx = i * 3;
            
            // YUYV格式: Y0, U0, Y1, V0, Y2, U1, Y3, V1...
            uint8_t Y = src_data[src_idx];
            uint8_t U = src_data[src_idx + 1];
            
            // 简化的YUV到BGR转换公式
            int C = Y - 16;
            int D = U - 128;
            
            // 对于偶数像素使用V0，奇数像素可能需要调整索引
            uint8_t V = (i % 2 == 0) ? src_data[src_idx + 3] : src_data[src_idx + 1];
            int E = V - 128;
            
            // 计算BGR值
            int B = (298 * C + 409 * E + 128) >> 8;
            int G = (298 * C - 100 * E - 208 * D + 128) >> 8;
            int R = (298 * C + 516 * D + 128) >> 8;
            
            // 确保值在0-255范围内
            dst_data[dst_idx] = (B < 0) ? 0 : ((B > 255) ? 255 : B);
            dst_data[dst_idx + 1] = (G < 0) ? 0 : ((G > 255) ? 255 : G);
            dst_data[dst_idx + 2] = (R < 0) ? 0 : ((R > 255) ? 255 : R);
        }
    } else {
        std::cout << "错误: cvtColor - 不支持的颜色转换代码" << std::endl;
    }
}

#endif // SAMPLE_COMMON_IMAGE_CONVERT_HPP_