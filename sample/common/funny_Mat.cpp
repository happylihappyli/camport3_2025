// funny_Mat.cpp - 自定义矩阵类的实现

#include "funny_Mat.hpp"
#include <cstring>
#include <iostream>

// 颜色转换函数实现
void cvtColor(const funny_Mat& src, funny_Mat& dst, int code) {
    // 检查输入有效性
    if (src.empty()) {
        std::cerr << "错误: 源图像为空" << std::endl;
        return;
    }
    
    // 根据转换代码执行不同的转换
    switch (code) {
        case COLOR_YUV2BGR_YVYU: {
            // 确保源图像是8UC2类型
            if (src.type() != CV_8UC2) {
                std::cerr << "错误: YVYU转换需要8UC2类型的源图像" << std::endl;
                return;
            }
            
            // 创建目标图像
            if (dst.rows() != src.rows() || dst.cols() != src.cols() || dst.type() != CV_8UC3) {
                dst = funny_Mat(src.rows(), src.cols(), CV_8UC3);
            }
            
            // YVYU到BGR的转换
            const uint8_t* yvyu_data = src.data();
            uint8_t* bgr_data = dst.data();
            
            for (int i = 0; i < src.rows(); ++i) {
                for (int j = 0; j < src.cols(); ++j) {
                    // YVYU格式是: Y0 V0 Y1 U0 Y2 V1 Y3 U1 ...
                    int idx = i * src.cols() + j;
                    int yvyu_idx = idx * 2;
                    int bgr_idx = idx * 3;
                    
                    // 提取YUV分量
                    uint8_t Y, U, V;
                    if (j % 2 == 0) {
                        Y = yvyu_data[yvyu_idx];
                        V = yvyu_data[yvyu_idx + 1];
                        // 对于偶数列，U来自下一个像素对的U分量
                        if (j + 1 < src.cols()) {
                            U = yvyu_data[yvyu_idx + 3];
                        } else {
                            U = 128; // 默认值
                        }
                    } else {
                        Y = yvyu_data[yvyu_idx];
                        U = yvyu_data[yvyu_idx + 1];
                        // 对于奇数列，V来自上一个像素对的V分量
                        V = yvyu_data[yvyu_idx - 1];
                    }
                    
                    // YUV到BGR的转换
                    // 公式参考: https://en.wikipedia.org/wiki/YUV
                    int C = Y - 16;
                    int D = U - 128;
                    int E = V - 128;
                    
                    int R = (298 * C + 409 * E + 128) >> 8;
                    int G = (298 * C - 100 * D - 208 * E + 128) >> 8;
                    int B = (298 * C + 516 * D + 128) >> 8;
                    
                    // 确保值在0-255范围内
                    R = (R < 0) ? 0 : (R > 255) ? 255 : R;
                    G = (G < 0) ? 0 : (G > 255) ? 255 : G;
                    B = (B < 0) ? 0 : (B > 255) ? 255 : B;
                    
                    // 存储BGR值
                    bgr_data[bgr_idx] = static_cast<uint8_t>(B);
                    bgr_data[bgr_idx + 1] = static_cast<uint8_t>(G);
                    bgr_data[bgr_idx + 2] = static_cast<uint8_t>(R);
                }
            }
            break;
        }
        
        case COLOR_YUV2BGR_YUYV: {
            // 确保源图像是8UC2类型
            if (src.type() != CV_8UC2) {
                std::cerr << "错误: YUYV转换需要8UC2类型的源图像" << std::endl;
                return;
            }
            
            // 创建目标图像
            if (dst.rows() != src.rows() || dst.cols() != src.cols() || dst.type() != CV_8UC3) {
                dst = funny_Mat(src.rows(), src.cols(), CV_8UC3);
            }
            
            // YUYV到BGR的转换
            const uint8_t* yuyv_data = src.data();
            uint8_t* bgr_data = dst.data();
            
            for (int i = 0; i < src.rows(); ++i) {
                for (int j = 0; j < src.cols(); ++j) {
                    // YUYV格式是: Y0 U0 Y1 V0 Y2 U1 Y3 V1 ...
                    int idx = i * src.cols() + j;
                    int yuyv_idx = idx * 2;
                    int bgr_idx = idx * 3;
                    
                    // 提取YUV分量
                    uint8_t Y, U, V;
                    if (j % 2 == 0) {
                        Y = yuyv_data[yuyv_idx];
                        U = yuyv_data[yuyv_idx + 1];
                        // 对于偶数列，V来自下一个像素对的V分量
                        if (j + 1 < src.cols()) {
                            V = yuyv_data[yuyv_idx + 3];
                        } else {
                            V = 128; // 默认值
                        }
                    } else {
                        Y = yuyv_data[yuyv_idx];
                        V = yuyv_data[yuyv_idx + 1];
                        // 对于奇数列，U来自上一个像素对的U分量
                        U = yuyv_data[yuyv_idx - 1];
                    }
                    
                    // YUV到BGR的转换
                    int C = Y - 16;
                    int D = U - 128;
                    int E = V - 128;
                    
                    int R = (298 * C + 409 * E + 128) >> 8;
                    int G = (298 * C - 100 * D - 208 * E + 128) >> 8;
                    int B = (298 * C + 516 * D + 128) >> 8;
                    
                    // 确保值在0-255范围内
                    R = (R < 0) ? 0 : (R > 255) ? 255 : R;
                    G = (G < 0) ? 0 : (G > 255) ? 255 : G;
                    B = (B < 0) ? 0 : (B > 255) ? 255 : B;
                    
                    // 存储BGR值
                    bgr_data[bgr_idx] = static_cast<uint8_t>(B);
                    bgr_data[bgr_idx + 1] = static_cast<uint8_t>(G);
                    bgr_data[bgr_idx + 2] = static_cast<uint8_t>(R);
                }
            }
            break;
        }
        
        default:
            std::cerr << "错误: 不支持的颜色转换代码" << std::endl;
            return;
    }
}

// 图像解码函数实现
bool imdecode(const std::vector<uint8_t>& buf, int flags, funny_Mat& dst) {
    // 注意：这是一个简化实现，实际项目中可能需要使用第三方库或系统API来解码JPEG图像
    // 这里我们假设输入是JPEG格式，并模拟解码过程
    
    if (buf.empty()) {
        std::cerr << "错误: 输入缓冲区为空" << std::endl;
        return false;
    }
    
    // 检查是否为JPEG格式（简化检查）
    if (buf.size() < 2 || buf[0] != 0xFF || buf[1] != 0xD8) {
        std::cerr << "错误: 不是有效的JPEG图像" << std::endl;
        return false;
    }
    
    // 注意：在实际应用中，这里需要使用真正的JPEG解码库
    // 这里我们只是创建一个模拟的RGB图像
    // 实际项目中，建议使用libjpeg、stb_image等轻量级库
    
    // 为了演示，我们创建一个固定大小的RGB图像
    // 注意：这只是一个占位符实现
    int width = 640;  // 假设宽度
    int height = 480; // 假设高度
    
    dst = funny_Mat(height, width, CV_8UC3);
    
    // 填充一些测试数据（渐变）
    uint8_t* data = dst.data();
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            int idx = i * width * 3 + j * 3;
            data[idx] = static_cast<uint8_t>((i * 255) / height);     // B
            data[idx + 1] = static_cast<uint8_t>((j * 255) / width);  // G
            data[idx + 2] = static_cast<uint8_t>(((i + j) * 255) / (height + width)); // R
        }
    }
    
    std::cout << "警告: imdecode是简化实现，仅用于演示。实际项目中需要使用真正的JPEG解码库。" << std::endl;
    return true;
}