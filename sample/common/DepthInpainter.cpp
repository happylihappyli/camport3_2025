#include "DepthInpainter.hpp"
#include <stdint.h>
#include "DepthInpainter.hpp"
#include "funny_Mat.hpp"
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <iostream>

// 定义常量
#define INSIDE 0
#define BAND 1
#define KNOWN 2
#define CHANGE 3
#define FLT_MAX 3.402823466e+38F
#define FLT_MIN 1.175494351e-38F

// 简化版的深度图像修复实现
// 注意：由于移除了OpenCV依赖，此实现功能有所简化

void DepthInpainter::inpaint(const funny_Mat& inputDepth, funny_Mat& out, const funny_Mat& mask)
{
    // 检查输入参数
    if (funny_Mat::getChannels(inputDepth.type()) != 1 || funny_Mat::getChannels(mask.type()) != 1) {
        // 创建与输入相同大小的输出
        out.create(inputDepth.rows(), inputDepth.cols(), inputDepth.type());
        // 简单复制输入
        // TODO: 实现funny_Mat的复制功能
        return;
    }

    // 生成有效深度的掩码
    funny_Mat validMask = genValidMask(inputDepth);
    funny_Mat validDepth;
    validDepth.create(inputDepth.rows(), inputDepth.cols(), inputDepth.type());
    
    // 复制有效深度值
    // 注意：这里实现的是简化版，仅处理16位无符号深度图像
    if (inputDepth.type() == CV_16UC1 && validMask.type() == CV_8UC1) {
        uint16_t* depthData = reinterpret_cast<uint16_t*>(const_cast<uint8_t*>(inputDepth.data()));
        uint16_t* validDepthData = reinterpret_cast<uint16_t*>(validDepth.data());
        uint8_t* maskData = reinterpret_cast<uint8_t*>(validMask.data());
        int totalPixels = inputDepth.rows() * inputDepth.cols();
        
        for (int i = 0; i < totalPixels; ++i) {
            if (maskData[i] > 0) {
                validDepthData[i] = depthData[i];
            } else {
                validDepthData[i] = 0;
            }
        }
    }

    // 创建输出图像
    out.create(inputDepth.rows(), inputDepth.cols(), inputDepth.type());
    
    // 简化的修复逻辑：将周围的有效像素值填充到无效区域
    // 注意：这是一个非常简化的实现，真实的修复算法需要更复杂的计算
    
    // 简单复制有效深度到输出
    if (validDepth.type() == CV_16UC1 && out.type() == CV_16UC1) {
        uint16_t* validDepthData = reinterpret_cast<uint16_t*>(validDepth.data());
        uint16_t* outData = reinterpret_cast<uint16_t*>(out.data());
        int totalPixels = validDepth.rows() * validDepth.cols();
        
        for (int i = 0; i < totalPixels; ++i) {
            outData[i] = validDepthData[i];
        }
    }

    // 实现简单的邻域填充修复
    // 注意：这是一个简化的实现，仅使用8邻域的平均值填充
    if (out.type() == CV_16UC1) {
        uint16_t* outData = reinterpret_cast<uint16_t*>(out.data());
        int rows = out.rows();
        int cols = out.cols();
        
        // 创建临时图像用于存储修复结果
        funny_Mat temp = out.clone();
        uint16_t* tempData = reinterpret_cast<uint16_t*>(temp.data());
        
        // 简单的邻域平均修复
        for (int i = 1; i < rows - 1; ++i) {
            for (int j = 1; j < cols - 1; ++j) {
                int index = i * cols + j;
                if (outData[index] == 0) {  // 无效像素
                    int sum = 0;
                    int count = 0;
                    
                    // 检查8邻域
                    for (int di = -1; di <= 1; ++di) {
                        for (int dj = -1; dj <= 1; ++dj) {
                            if (di == 0 && dj == 0) continue;
                            int ni = i + di;
                            int nj = j + dj;
                            int nindex = ni * cols + nj;
                            if (ni >= 0 && ni < rows && nj >= 0 && nj < cols && outData[nindex] > 0) {
                                sum += outData[nindex];
                                count++;
                            }
                        }
                    }
                    
                    // 如果有有效邻域像素，使用平均值
                    if (count > 0) {
                        tempData[index] = static_cast<uint16_t>(sum / count);
                    }
                }
            }
        }
        
        // 将临时结果复制回输出
        memcpy(outData, tempData, rows * cols * sizeof(uint16_t));
    }
}

funny_Mat DepthInpainter::genValidMask(const funny_Mat& depth)
{
    funny_Mat validMask;
    validMask.create(depth.rows(), depth.cols(), CV_8UC1);
    
    // 生成有效深度值的掩码
    if (depth.type() == CV_16UC1 && validMask.type() == CV_8UC1) {
        uint16_t* depthData = reinterpret_cast<uint16_t*>(const_cast<uint8_t*>(depth.data()));
        uint8_t* maskData = reinterpret_cast<uint8_t*>(validMask.data());
        int totalPixels = depth.rows() * depth.cols();
        
        for (int i = 0; i < totalPixels; ++i) {
            maskData[i] = (depthData[i] > 0) ? 255 : 0;
        }
    }

    // 实现简单的膨胀操作，填充小的空洞
    if (validMask.type() == CV_8UC1) {
        uint8_t* maskData = reinterpret_cast<uint8_t*>(validMask.data());
        int rows = validMask.rows();
        int cols = validMask.cols();
        
        // 创建临时图像用于存储膨胀结果
        funny_Mat temp = validMask.clone();
        uint8_t* tempData = reinterpret_cast<uint8_t*>(temp.data());
        
        // 简单的膨胀操作（3x3邻域）
        for (int i = 1; i < rows - 1; ++i) {
            for (int j = 1; j < cols - 1; ++j) {
                int index = i * cols + j;
                if (tempData[index] == 0) {  // 无效像素
                    // 检查8邻域
                    for (int di = -1; di <= 1; ++di) {
                        for (int dj = -1; dj <= 1; ++dj) {
                            if (di == 0 && dj == 0) continue;
                            int ni = i + di;
                            int nj = j + dj;
                            int nindex = ni * cols + nj;
                            if (ni >= 0 && ni < rows && nj >= 0 && nj < cols && tempData[nindex] > 0) {
                                maskData[index] = 255;  // 设置为有效
                                break;
                            }
                        }
                        if (maskData[index] > 0) break;
                    }
                }
            }
        }
    }

    return validMask;
}

// 辅助函数：最小值
float min4(float a, float b, float c, float d) {
    float min_val = a;
    if (b < min_val) min_val = b;
    if (c < min_val) min_val = c;
    if (d < min_val) min_val = d;
    return min_val;
}

// 辅助函数：向量点积
float VectorScalMult(float x1, float y1, float x2, float y2) {
    return x1 * x2 + y1 * y2;
}

// 辅助函数：向量长度平方
float VectorLength(float x, float y) {
    return x * x + y * y;
}