#pragma once

// 根据是否使用完整SDK头文件来决定包含方式
#ifdef USE_FULL_SDK_HEADERS
// 使用完整SDK头文件的项目（如cloud项目）
#include "TYApi.h"
#include "TyIsp.h"
#include "TYDefs.h"
#include "funny_Mat.hpp"
#else
// 使用头文件隔离的项目（如sample项目）- 使用前向声明避免重复包含
#include <cstdint>
#include <vector>
#include <iostream>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <memory>
#include <functional>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <algorithm>
#include <sys/timeb.h>
#include <thread>
#include <atomic>

// funny_Mat头文件
#include "funny_Mat.hpp"
// 包含SDK类型定义
#include "TYApi.h"

// SDK类型完整定义 - 避免重复包含
// 注意：TY_IMAGE_DATA已在TYApi.h中定义，不需要重复定义
// struct TY_IMAGE_DATA {
//     uint32_t width;
//     uint32_t height;
//     uint32_t pixelFormat;
//     uint32_t componentID;
//     void* buffer;
//     uint32_t size;
//     int32_t status;
//     uint64_t timestamp;
//     uint32_t imageIndex;
// };

// 注意：TY_FRAME_DATA和TY_VECT_3F已在TYApi.h中定义，不需要重复定义

typedef void* TY_DEV_HANDLE;
typedef void* TY_ISP_HANDLE;
#endif

namespace percipio_layer {

// 像素格式定义 - 避免依赖SDK枚举
const uint32_t MONO = 0x0001;
const uint32_t MONO16 = 0x0002;
const uint32_t DEPTH16 = 0x0030;
const uint32_t TOF_IR_MONO16 = 0x0040;
const uint32_t JPEG = 0x1000;
const uint32_t YVYU = 0x0020;
const uint32_t YUYV = 0x0021;
const uint32_t RGB = 0x0014;
const uint32_t BGR = 0x0015;
const uint32_t BAYER8GBRG = 0x0008;
const uint32_t BAYER8BGGR = 0x0009;
const uint32_t BAYER8GRBG = 0x000A;
const uint32_t BAYER8RGGB = 0x000B;
const uint32_t CSI_MONO10 = 0x010A;
const uint32_t CSI_MONO12 = 0x010C;
const uint32_t XYZ48 = 0x1030;

// 组件ID定义
const int TY_COMPONENT_RGB_CAM = 0;
const int TY_COMPONENT_DEPTH_CAM = 1;
const int TY_COMPONENT_IR_CAM_LEFT = 2;
const int TY_COMPONENT_IR_CAM_RIGHT = 3;

// 状态码定义
#ifndef TY_STATUS_OK
#define TY_STATUS_OK 0
#endif

// 前向声明函数，避免循环依赖
int parseIrFrame(const TY_IMAGE_DATA* img, funny_Mat* pIr);
int parseColorFrame(const TY_IMAGE_DATA* img, funny_Mat* pColor, TY_ISP_HANDLE color_isp_handle);
int parseImage(const TY_IMAGE_DATA* img, funny_Mat* image, TY_ISP_HANDLE color_isp_handle);
int parseDepthFrame(const TY_IMAGE_DATA* img, funny_Mat* pDepth);
int parsePoints(const TY_VECT_3F& pts, int pointSize, funny_Mat* point3f);

// 前向声明辅助函数
int parseBayer8Frame(const TY_IMAGE_DATA* img, funny_Mat* pColor, TY_ISP_HANDLE color_isp_handle);
int parseBayer10Frame(const TY_IMAGE_DATA* img, funny_Mat* pColor);
int parseBayer12Frame(const TY_IMAGE_DATA* img, funny_Mat* pColor);
void parseCsiRaw8(uint8_t* src, funny_Mat& dst, int width, int height);
int parseCsiRaw10(uint8_t* src, funny_Mat& dst, int width, int height);
int parseCsiRaw12(uint8_t* src, funny_Mat& dst, int width, int height);
bool imdecode(const std::vector<uint8_t>& buffer, int flags, funny_Mat& dst);
void cvtColor(const funny_Mat& src, funny_Mat& dst, int code);

// 函数实现
inline int parseIrFrame(const TY_IMAGE_DATA* img, funny_Mat* pIr) {
    if (!img || !pIr) {
        return -1;
    }
    
    if (img->pixelFormat == MONO) {
        *pIr = funny_Mat(img->height, img->width, toInt(PixelFormat::CV_8UC1), img->buffer).clone();
    }
    else if (img->pixelFormat == MONO16) {
        *pIr = funny_Mat(img->height, img->width, toInt(PixelFormat::CV_16U), img->buffer).clone();
    }
    else if (img->pixelFormat == CSI_MONO10) {
        funny_Mat raw16(img->height, img->width, toInt(PixelFormat::CV_16U));
        int ret = parseCsiRaw10(reinterpret_cast<uint8_t*>(img->buffer), raw16, img->width, img->height);
        *pIr = raw16.clone();
        return ret;
    }
    else if (img->pixelFormat == CSI_MONO12) {
        funny_Mat raw16(img->height, img->width, toInt(PixelFormat::CV_16U));
        int ret = parseCsiRaw12(reinterpret_cast<uint8_t*>(img->buffer), raw16, img->width, img->height);
        *pIr = raw16.clone();
        return ret;
    }
    else {
        return -1;
    }
    return 0;
}

inline int parseColorFrame(const TY_IMAGE_DATA* img, funny_Mat* pColor, TY_ISP_HANDLE color_isp_handle) {
    int ret = 0;
    if (!img || !pColor) {
        return -1;
    }
    
    if (img->pixelFormat == JPEG){
        std::vector<uint8_t> _v((uint8_t*)img->buffer, (uint8_t*)img->buffer + img->size);
        bool decode_success = imdecode(_v, 0, *pColor);
        if (!(decode_success && img->width == pColor->cols() && img->height == pColor->rows())) {
            std::cerr << "JPEG解码失败或尺寸不匹配" << std::endl;
            return -1;
        }
    }
    else if (img->pixelFormat == YVYU){
        funny_Mat yuv(img->height, img->width, toInt(PixelFormat::CV_8UC2), img->buffer);
        cvtColor(yuv, *pColor, static_cast<int>(ColorConversionCode::YUV2BGR_YVYU));
    }
    else if (img->pixelFormat == YUYV){
        funny_Mat yuv(img->height, img->width, toInt(PixelFormat::CV_8UC2), img->buffer);
        cvtColor(yuv, *pColor, static_cast<int>(ColorConversionCode::YUV2BGR_YUYV));
    }
    else if (img->pixelFormat == RGB){
        funny_Mat rgb(img->height, img->width, toInt(PixelFormat::CV_8UC3), img->buffer);
        // 手动将RGB转为BGR
        *pColor = funny_Mat(img->height, img->width, toInt(PixelFormat::CV_8UC3));
        uint8_t* src = (uint8_t*)rgb.data();
        uint8_t* dst = (uint8_t*)pColor->data();
        for (int i = 0; i < img->height * img->width; ++i) {
            dst[i*3 + 0] = src[i*3 + 2]; // B
            dst[i*3 + 1] = src[i*3 + 1]; // G
            dst[i*3 + 2] = src[i*3 + 0]; // R
        }
    }
    else if (img->pixelFormat == BGR){
        *pColor = funny_Mat(img->height, img->width, toInt(PixelFormat::CV_8UC3), img->buffer).clone();
    }
    else if (img->pixelFormat == BAYER8GBRG || 
             img->pixelFormat == BAYER8BGGR || 
             img->pixelFormat == BAYER8GRBG || 
             img->pixelFormat == BAYER8RGGB) 
    {
        ret = parseBayer8Frame(img, pColor, color_isp_handle);
    }
    else if (img->pixelFormat == MONO){
        *pColor = funny_Mat(img->height, img->width, toInt(PixelFormat::CV_8UC3));
        uint8_t* gray_data = (uint8_t*)img->buffer;
        uint8_t* color_data = pColor->data();
        for (int i = 0; i < img->height * img->width; i++) {
          color_data[i*3] = gray_data[i];
          color_data[i*3 + 1] = gray_data[i];
          color_data[i*3 + 2] = gray_data[i];
        }
    }
    else if (img->pixelFormat == CSI_MONO10){
        funny_Mat gray16(img->height, img->width, toInt(PixelFormat::CV_16U));
        ret = parseCsiRaw10(reinterpret_cast<uint8_t*>(img->buffer), gray16, img->width, img->height);
        *pColor = gray16.clone();
    }
    else {
        return -1;
    }
    return ret;
}

inline int parseDepthFrame(const TY_IMAGE_DATA* img, funny_Mat* pDepth) {
    if (!img || !pDepth) {
        return -1;
    }
    
    if (img->pixelFormat == DEPTH16){
        *pDepth = funny_Mat(img->height, img->width, toInt(PixelFormat::CV_16U), img->buffer).clone();
    }
    else if(img->pixelFormat == MONO16){
        *pDepth = funny_Mat(img->height, img->width, toInt(PixelFormat::CV_16U), img->buffer).clone();
    }
    else if(img->pixelFormat == TOF_IR_MONO16){
        *pDepth = funny_Mat(img->height, img->width, toInt(PixelFormat::CV_16U), img->buffer).clone();
    }
    else if(img->pixelFormat == CSI_MONO10){
        funny_Mat raw16(img->height, img->width, toInt(PixelFormat::CV_16U));
        int ret = parseCsiRaw10(reinterpret_cast<uint8_t*>(img->buffer), raw16, img->width, img->height);
        *pDepth = raw16.clone();
        return ret;
    }
    else if(img->pixelFormat == CSI_MONO12){
        funny_Mat raw16(img->height, img->width, toInt(PixelFormat::CV_16U));
        int ret = parseCsiRaw12(reinterpret_cast<uint8_t*>(img->buffer), raw16, img->width, img->height);
        *pDepth = raw16.clone();
        return ret;
    }
    else {
        return -1;
    }
    return 0;
}

inline int parseImage(const TY_IMAGE_DATA* img, funny_Mat* image, TY_ISP_HANDLE color_isp_handle) {
    int ret = 0;
    if (!img || !image) {
        return -1;
    }
    
    if (img->pixelFormat == JPEG){
        std::vector<uint8_t> _v((uint8_t*)img->buffer, (uint8_t*)img->buffer + img->size);
        bool decode_success = imdecode(_v, 0, *image);
        if (!(decode_success && img->width == image->cols() && img->height == image->rows())) {
            std::cerr << "JPEG解码失败或尺寸不匹配" << std::endl;
            return -1;
        }
    }
    else if (img->pixelFormat == YVYU){
        funny_Mat yuv(img->height, img->width, toInt(PixelFormat::CV_8UC2), img->buffer);
        cvtColor(yuv, *image, static_cast<int>(ColorConversionCode::YUV2BGR_YVYU));
    }
    else if (img->pixelFormat == YUYV){
        funny_Mat yuv(img->height, img->width, toInt(PixelFormat::CV_8UC2), img->buffer);
        cvtColor(yuv, *image, static_cast<int>(ColorConversionCode::YUV2BGR_YUYV));
    }
    else if (img->pixelFormat == RGB){
        funny_Mat rgb(img->height, img->width, toInt(PixelFormat::CV_8UC3), img->buffer);
        // 手动将RGB转为BGR
        *image = funny_Mat(img->height, img->width, toInt(PixelFormat::CV_8UC3));
        uint8_t* src = (uint8_t*)rgb.data();
        uint8_t* dst = (uint8_t*)image->data();
        for (int i = 0; i < img->height * img->width; ++i) {
            dst[i*3 + 0] = src[i*3 + 2]; // B
            dst[i*3 + 1] = src[i*3 + 1]; // G
            dst[i*3 + 2] = src[i*3 + 0]; // R
        }
    }
    else if (img->pixelFormat == BGR){
        *image = funny_Mat(img->height, img->width, toInt(PixelFormat::CV_8UC3), img->buffer).clone();
    }
    else if (img->pixelFormat == MONO){
        *image = funny_Mat(img->height, img->width, toInt(PixelFormat::CV_8UC1), img->buffer).clone();
    }
    else if (img->pixelFormat == MONO16){
        *image = funny_Mat(img->height, img->width, toInt(PixelFormat::CV_16U), img->buffer).clone();
    }
    else if (img->pixelFormat == DEPTH16){
        *image = funny_Mat(img->height, img->width, toInt(PixelFormat::CV_16U), img->buffer).clone();
    }
    else if (img->pixelFormat == TOF_IR_MONO16){
        *image = funny_Mat(img->height, img->width, toInt(PixelFormat::CV_16U), img->buffer).clone();
    }
    else if (img->pixelFormat == CSI_MONO10){
        funny_Mat gray16(img->height, img->width, toInt(PixelFormat::CV_16U));
        ret = parseCsiRaw10(reinterpret_cast<uint8_t*>(img->buffer), gray16, img->width, img->height);
        *image = gray16.clone();
    }
    else if (img->pixelFormat == CSI_MONO12) {
        funny_Mat gray16(img->height, img->width, toInt(PixelFormat::CV_16U));
        ret = parseCsiRaw12(reinterpret_cast<uint8_t*>(img->buffer), gray16, img->width, img->height);
        *image = gray16.clone();
    } 
    else if (img->pixelFormat == MONO16 || img->pixelFormat==TOF_IR_MONO16){
        *image = funny_Mat(img->height, img->width, toInt(PixelFormat::CV_16U), img->buffer).clone();
    }
    else {
        std::cout << "警告: parseImage中不支持的像素格式: " << img->pixelFormat << std::endl;
        ret = -1;
    }

    return ret;
}

// 解析帧数据
inline int parseFrame(const TY_FRAME_DATA& frame, funny_Mat* pDepth
                             , funny_Mat* pLeftIR, funny_Mat* pRightIR
                             , funny_Mat* pColor, TY_ISP_HANDLE color_isp_handle)
{
    for (int i = 0; i < frame.validCount; i++){
        if (frame.image[i].status != TY_STATUS_OK) continue;

        // get depth image
        if (pDepth && frame.image[i].componentID == TY_COMPONENT_DEPTH_CAM){
                if (frame.image[i].pixelFormat == XYZ48) {
                // 注意：XYZ48格式需要自定义实现
                funny_Mat temp(frame.image[i].height, frame.image[i].width, toInt(PixelFormat::CV_16U), frame.image[i].buffer);
                *pDepth = temp.clone();
                std::cout << "警告: parseFrame中XYZ48格式处理需要自定义实现" << std::endl;
                }
                else {
                funny_Mat temp(frame.image[i].height, frame.image[i].width
                          , toInt(PixelFormat::CV_16U), frame.image[i].buffer);
                *pDepth = temp.clone();
                }
        }
        // get left ir image
        if (pLeftIR && frame.image[i].componentID == TY_COMPONENT_IR_CAM_LEFT){
            parseIrFrame(&frame.image[i], pLeftIR);
        }
        // get right ir image
        if (pRightIR && frame.image[i].componentID == TY_COMPONENT_IR_CAM_RIGHT){
            parseIrFrame(&frame.image[i], pRightIR);
        }
        // get BGR
        if (pColor && frame.image[i].componentID == TY_COMPONENT_RGB_CAM){
            parseColorFrame(&frame.image[i], pColor, color_isp_handle);
        }
    }

    return 0;
}

enum{
    PC_FILE_FORMAT_XYZ = 0,
};

// 写入XYZ格式的点云文件
static void writePC_XYZ(const funny_Point3f* pnts, const funny_Vec3b *color, size_t n, FILE* fp)
{
    if (color){
        for (size_t i = 0; i < n; i++){
            if (!std::isnan(pnts[i].x)){
                fprintf(fp, "%f %f %f %d %d %d\n", pnts[i].x, pnts[i].y, pnts[i].z, color[i][0], color[i][1], color[i][2]);
            }
        }
    }
    else{
        for (size_t i = 0; i < n; i++){
            if (!std::isnan(pnts[i].x)){
                fprintf(fp, "%f %f %f 0 0 0\n", pnts[i].x, pnts[i].y, pnts[i].z);
            }
        }
    }
}

// 写入点云文件
static void writePointCloud(const funny_Point3f* pnts, const funny_Vec3b *color, size_t n, const char* file, int format)
{
    FILE* fp = fopen(file, "w");
    if (!fp){
        return;
    }

    switch (format){
    case PC_FILE_FORMAT_XYZ:
        writePC_XYZ(pnts, color, n, fp);
        break;
    default:
        break;
    }

    fclose(fp);
}

// 回调包装器类
class CallbackWrapper
{
public:
    typedef void(*TY_FRAME_CALLBACK) (TY_FRAME_DATA*, void* userdata);

    CallbackWrapper(){
        _hDevice = NULL;
        _cb = NULL;
        _userdata = NULL;
        _exit = true;
        _cbThread = new std::thread(&workerThread, this);
    }

    ~CallbackWrapper() {
        if (_cbThread->joinable()) {
            _exit = true;
            _cbThread->join();
        }
        delete _cbThread;
        _cbThread = nullptr;
    }

    TY_STATUS TYRegisterCallback(TY_DEV_HANDLE hDevice, TY_FRAME_CALLBACK v, void* userdata)
    {
        _hDevice = hDevice;
        _cb = v;
        _userdata = userdata;
        _exit = false;
        return TY_STATUS_OK;
    }

    void TYUnregisterCallback()
    {
        if (!_exit) {
            _exit = true;
            if (_cbThread->joinable()) {
                _cbThread->join();
            }
        }
    }

private:
    static void workerThread(CallbackWrapper* pWrapper)
    {
        TY_FRAME_DATA frame;

        while (!pWrapper->_exit)
        {
            int err = TYFetchFrame(pWrapper->_hDevice, &frame, 100);
            if (!err) {
                pWrapper->_cb(&frame, pWrapper->_userdata);
            }
        }
        // 使用Utils.hpp中的LOGI或标准输出
        std::cout << "frameCallback exit!" << std::endl;
    }

    TY_DEV_HANDLE _hDevice;
    TY_FRAME_CALLBACK _cb;
    void* _userdata;

    std::atomic<bool> _exit;
    std::thread* _cbThread;
};

// 获取FPS
#ifdef _WIN32
static int get_fps() {
    static int fps_counter = 0;
    static clock_t fps_tm = 0;
   const int kMaxCounter = 250;
   fps_counter++;
   if (fps_counter < kMaxCounter) {
     return -1;
   }
   int elapse = (clock() - fps_tm);
   int v = (int)(((float)fps_counter) / elapse * CLOCKS_PER_SEC);
   fps_tm = clock();

   fps_counter = 0;
   return v;
 }
#else
static int get_fps() {
    static int fps_counter = 0;
    static clock_t fps_tm = 0;
    const int kMaxCounter = 200;
    struct timeval start;
    fps_counter++;
    if (fps_counter < kMaxCounter) {
        return -1;
    }

    gettimeofday(&start, NULL);
    int elapse = start.tv_sec * 1000 + start.tv_usec / 1000 - fps_tm;
    int v = (int)(((float)fps_counter) / elapse * 1000);
    fps_tm = start.tv_sec * 1000 + start.tv_usec / 1000;
    fps_counter = 0;
    return v;
}
#endif

} // namespace percipio_layer