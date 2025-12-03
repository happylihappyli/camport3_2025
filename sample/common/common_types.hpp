#pragma once

// 只包含最基本的头文件
#include <cstdint>
#include <vector>

// SDK头文件 - 只包含TYDefs.h
#include "TYDefs.h"

// funny_Mat前向声明
namespace percipio_layer {
    class funny_Mat;
}

namespace percipio_layer {

// 函数前向声明
int parseIrFrame(const TY_IMAGE_DATA* img, funny_Mat* pIr);
int parseColorFrame(const TY_IMAGE_DATA* img, funny_Mat* pColor, TY_ISP_HANDLE color_isp_handle);
int parseImage(const TY_IMAGE_DATA* img, funny_Mat* image, TY_ISP_HANDLE color_isp_handle);
int parseDepthFrame(const TY_IMAGE_DATA* img, funny_Mat* pDepth);
int parsePoints(const TY_VECT_3F& pts, int pointSize, funny_Mat* point3f);

// 辅助函数声明
int parseBayer8Frame(const TY_IMAGE_DATA* img, funny_Mat* pColor, TY_ISP_HANDLE color_isp_handle);
int parseBayer10Frame(const TY_IMAGE_DATA* img, funny_Mat* pColor);
int parseBayer12Frame(const TY_IMAGE_DATA* img, funny_Mat* pColor);
void parseCsiRaw8(uint8_t* src, funny_Mat& dst, int width, int height);
int parseCsiRaw10(uint8_t* src, funny_Mat& dst, int width, int height);
int parseCsiRaw12(uint8_t* src, funny_Mat& dst, int width, int height);
bool imdecode(const std::vector<uint8_t>& buffer, int flags, funny_Mat& dst);
void cvtColor(const funny_Mat& src, funny_Mat& dst, int code);

} // namespace percipio_layer