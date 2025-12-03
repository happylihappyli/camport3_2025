#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <chrono>
#include <ctime>
#include <thread>
#include <cstdio>
#include <ctime>
#define WIN32_LEAN_AND_MEAN  // 避免winsock冲突
#include <windows.h>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <algorithm>

// API头文件 - 必须先包含TYDefs.h，因为TYApi.h依赖于它
#include "../include/TYDefs.h"
#include "../include/TYApi.h"
#include "../include/TYImageProc.h"

// 本地头文件
#include "../sample/sample_v2/hpp/Device.hpp"
#include "../sample/sample_v2/hpp/Frame.hpp"

// 包含Utils.hpp以获取TY_STATUS_OK等定义
#include "../sample/common/Utils.hpp"

// 包含common.hpp以获取LOGD宏 - 注意：cloud项目使用完整的SDK头文件
#define USE_FULL_SDK_HEADERS  // 标记使用完整SDK头文件
#include "../sample/common/common.hpp"

// 定义 ASSERT_OK 宏
#ifndef ASSERT_OK
#define ASSERT_OK(x) do { TY_STATUS ret = (x); if (ret != TY_STATUS_OK) { std::cerr << "ERROR: TY call failed with status " << ret << " at " << __FILE__ << ":" << __LINE__ << std::endl; return ret; } } while(0)
#endif





// using namespace percipio_layer;  // 移除重复的using语句

#if _WIN32
#include <conio.h>
#elif __linux__
#include <termio.h>
#define TTY_PATH    "/dev/tty"
#define STTY_DEF    "stty -raw echo -F"
#endif

static char GetChar(void)
{
    char ch = -1;
#if _WIN32
    if (_kbhit()) //check fifo - 使用ISO C++标准名称
    {
        ch = _getche(); //read fifo - 使用ISO C++标准名称
    }
#elif __linux__
    fd_set rfds;
    struct timeval tv;
    system(STTY_DEF TTY_PATH);
    FD_ZERO(&rfds);
    FD_SET(0, &rfds);
    tv.tv_sec = 0;
    tv.tv_usec = 10;
    if (select(1, &rfds, NULL, NULL, &tv) > 0)
    {
        ch = getchar();
    }
#endif
    return ch;
}

class P3DCamera : public percipio_layer::FastCamera
{
    public:
        P3DCamera() : percipio_layer::FastCamera() {};
        ~P3DCamera() {}; 

        TY_STATUS Init();
        TY_STATUS InitForFileMode();  // 文件模式初始化（不需要相机设备）
        int process(const std::shared_ptr<percipio_layer::TYImage>&  depth, const std::shared_ptr<percipio_layer::TYImage>&  color);

    private:
        float f_depth_scale_unit = 1.f;
        bool depth_needUndistort = false;
        bool b_points_with_color = false;
        TY_CAMERA_CALIB_INFO depth_calib, color_calib;
        std::shared_ptr<percipio_layer::ImageProcesser> depth_processer;
        std::shared_ptr<percipio_layer::ImageProcesser> color_processer;
        void savePointsToPly(const std::vector<TY_VECT_3F>& p3d, const std::shared_ptr<percipio_layer::TYImage>& color, const char* fileName);
        void processDepth16(const std::shared_ptr<percipio_layer::TYImage>&  depth, std::vector<TY_VECT_3F>& p3d);
        void processXYZ48(const std::shared_ptr<percipio_layer::TYImage>&  depth, std::vector<TY_VECT_3F>& p3d);

        void processDepth16ToPoint3D(const std::shared_ptr<percipio_layer::TYImage>&  depth, const std::shared_ptr<percipio_layer::TYImage>&  color, std::vector<TY_VECT_3F>& p3d,  std::shared_ptr<percipio_layer::TYImage>& registration_color);
        void processXYZ48ToPoint3D(const std::shared_ptr<percipio_layer::TYImage>&  depth, const std::shared_ptr<percipio_layer::TYImage>&  color, std::vector<TY_VECT_3F>& p3d,  std::shared_ptr<percipio_layer::TYImage>& registration_color);
        
        // 静态流索引枚举值
        static const stream_idx stream_depth = static_cast<stream_idx>(0x1);
        static const stream_idx stream_color = static_cast<stream_idx>(0x2);
};

TY_STATUS P3DCamera::Init()
{
    TY_STATUS status;
    
    status = stream_enable(stream_depth);
    if(status != TY_STATUS_OK) return status;

    if(has_stream(stream_color)) {
        status = stream_enable(stream_color);
        if(status != TY_STATUS_OK) return status;
        bool hasColorCalib = false;
        ASSERT_OK(TYHasFeature(handle(), TY_COMPONENT_RGB_CAM, TY_STRUCT_CAM_CALIB_DATA, &hasColorCalib));
        if (hasColorCalib)
        {
            ASSERT_OK(TYGetStruct(handle(), TY_COMPONENT_RGB_CAM, TY_STRUCT_CAM_CALIB_DATA, &color_calib, sizeof(color_calib)));
            color_processer = std::shared_ptr<percipio_layer::ImageProcesser>(new percipio_layer::ImageProcesser("color", &color_calib));
        }
    }
    
    ASSERT_OK(TYGetFloat(handle(), TY_COMPONENT_DEPTH_CAM, TY_FLOAT_SCALE_UNIT, &f_depth_scale_unit));
    ASSERT_OK(TYHasFeature(handle(), TY_COMPONENT_DEPTH_CAM, TY_STRUCT_CAM_DISTORTION, &depth_needUndistort));
    ASSERT_OK(TYGetStruct(handle(), TY_COMPONENT_DEPTH_CAM, TY_STRUCT_CAM_CALIB_DATA, &depth_calib, sizeof(depth_calib)));
    depth_processer = std::shared_ptr<percipio_layer::ImageProcesser>(new percipio_layer::ImageProcesser("depth", &depth_calib));

    return TY_STATUS_OK;
}

// 文件模式初始化函数（不需要相机设备）
TY_STATUS P3DCamera::InitForFileMode()
{
    // 文件模式下使用默认的校准参数
    std::cout << "初始化文件模式..." << std::endl;
    
    // 设置默认的深度比例单位（毫米到米转换）- 使用与sample_v2版本相同的值
    f_depth_scale_unit = 0.001f;  // 改为0.001，毫米到米转换
    
    // 设置默认需要去除畸变
    depth_needUndistort = true;
    
    // 设置默认的深度相机校准数据 - 使用与sample_v2版本完全相同的参数
    memset(&depth_calib, 0, sizeof(depth_calib));
    depth_calib.intrinsicWidth = 1280;
    depth_calib.intrinsicHeight = 960;
    depth_calib.intrinsic.data[0] = 1031.15f;  // fx
    depth_calib.intrinsic.data[4] = 1031.15f;  // fy  
    depth_calib.intrinsic.data[2] = 636.174f;   // cx
    depth_calib.intrinsic.data[5] = 505.396f;   // cy
    
    // 初始化depth_processer
    depth_processer = std::shared_ptr<percipio_layer::ImageProcesser>(new percipio_layer::ImageProcesser("depth", &depth_calib));
    
    // 初始化color_processer（如果可用）- 使用与sample_v2版本完全相同的参数
    memset(&color_calib, 0, sizeof(color_calib));
    color_calib.intrinsicWidth = 1280;
    color_calib.intrinsicHeight = 960;
    color_calib.intrinsic.data[0] = 1857.5f;   // fx
    color_calib.intrinsic.data[4] = 1856.85f;  // fy
    color_calib.intrinsic.data[2] = 1221.04f;  // cx
    color_calib.intrinsic.data[5] = 928.009f;  // cy
    
    // 设置彩色相机的外参矩阵 - 使用与sample_v2版本完全相同的外参数据
    // 旋转矩阵 R (3x3)
    color_calib.extrinsic.data[0] = 0.999137f;   color_calib.extrinsic.data[1] = 0.000711224f; color_calib.extrinsic.data[2] = -0.0415417f;
    color_calib.extrinsic.data[4] = -0.00154226f; color_calib.extrinsic.data[5] = 0.999799f;   color_calib.extrinsic.data[6] = -0.0199763f;
    color_calib.extrinsic.data[8] = 0.0415192f;  color_calib.extrinsic.data[9] = 0.0200231f;   color_calib.extrinsic.data[10] = 0.998937f;
    // 平移向量 T (3x1)
    color_calib.extrinsic.data[3] = 24.827f;
    color_calib.extrinsic.data[7] = -0.109068f;
    color_calib.extrinsic.data[11] = -0.147f;
    
    color_processer = std::shared_ptr<percipio_layer::ImageProcesser>(new percipio_layer::ImageProcesser("color", &color_calib));
    
    std::cout << "文件模式初始化完成" << std::endl;
    std::cout << "深度相机内参: fx=" << depth_calib.intrinsic.data[0] << ", fy=" << depth_calib.intrinsic.data[4] 
              << ", cx=" << depth_calib.intrinsic.data[2] << ", cy=" << depth_calib.intrinsic.data[5] << std::endl;
    std::cout << "彩色相机内参: fx=" << color_calib.intrinsic.data[0] << ", fy=" << color_calib.intrinsic.data[4] 
              << ", cx=" << color_calib.intrinsic.data[2] << ", cy=" << color_calib.intrinsic.data[5] << std::endl;
    std::cout << "彩色相机外参旋转矩阵:" << std::endl;
    std::cout << "  [" << color_calib.extrinsic.data[0] << ", " << color_calib.extrinsic.data[1] << ", " << color_calib.extrinsic.data[2] << "]" << std::endl;
    std::cout << "  [" << color_calib.extrinsic.data[4] << ", " << color_calib.extrinsic.data[5] << ", " << color_calib.extrinsic.data[6] << "]" << std::endl;
    std::cout << "  [" << color_calib.extrinsic.data[8] << ", " << color_calib.extrinsic.data[9] << ", " << color_calib.extrinsic.data[10] << "]" << std::endl;
    std::cout << "彩色相机外参平移向量: [" << color_calib.extrinsic.data[3] << ", " << color_calib.extrinsic.data[7] << ", " << color_calib.extrinsic.data[11] << "]" << std::endl;
    return TY_STATUS_OK;
}


void P3DCamera::savePointsToPly(const std::vector<TY_VECT_3F>& p3d, const std::shared_ptr<percipio_layer::TYImage>& color, const char* fileName)
{
    int   pointsCnt = 0;
    const TY_VECT_3F *point = p3d.data();
    std::stringstream ss;
    
    if(color) {
        // 使用标准的RGB处理流程，与第二个程序保持一致
        uint8_t* pixels = (uint8_t*)color->buffer();
        
        // 假设颜色图像是BGR888格式（标准RGB格式）
        for(int i = 0; i < p3d.size(); i++) {
            if(!std::isnan(point->z)) {
                // 检查颜色数组边界
                int color_idx = 3 * i;
                if (color_idx + 2 < color->size()) {
                    ss << (point->x)/1000 << " " << (point->y)/1000 << " " << (point->z)/1000 << " " << 
                        (uint32_t)pixels[color_idx] << " " << (uint32_t)pixels[color_idx + 1] << " " << (uint32_t)pixels[color_idx + 2] << std::endl;
                } else {
                    // 如果颜色索引越界，使用白色
                    ss << (point->x)/1000 << " " << (point->y)/1000 << " " << (point->z)/1000 << " 255 255 255" << std::endl;
                }
                pointsCnt++;
            }
            point++;
        }
    } else {
        for(int i = 0; i < p3d.size(); i++) {
            if(!std::isnan(point->z)) {
                ss << (point->x)/1000 << " " << (point->y)/1000 << " " << (point->z)/1000 << std::endl;
                pointsCnt++;
            }
            point++;
        }
    }

    FILE *fp = fopen(fileName, "wb+");
    fprintf(fp, "ply\n");
    fprintf(fp, "format ascii 1.0\n");
    fprintf(fp, "element vertex %d\n", pointsCnt);
    fprintf(fp, "property float x\n");
    fprintf(fp, "property float y\n");
    fprintf(fp, "property float z\n");
    if(color) {
        fprintf(fp, "property uchar blue\n");
        fprintf(fp, "property uchar green\n");
        fprintf(fp, "property uchar red\n");
    }
    fprintf(fp, "end_header\n");
    fprintf(fp, "%s", ss.str().c_str());
    fflush(fp);
    fclose(fp);
}

void P3DCamera::processDepth16(const std::shared_ptr<percipio_layer::TYImage>&  depth, std::vector<TY_VECT_3F>& p3d)
{
    if(!depth) return;

    if(depth->pixelFormat() == TY_PIXEL_FORMAT_DEPTH16) {
        p3d.resize(depth->width() * depth->height());
        TYMapDepthImageToPoint3d(&depth_calib, depth->width(), depth->height(), (uint16_t*)depth->buffer(), &p3d[0], f_depth_scale_unit);
    }
}

void P3DCamera::processXYZ48(const std::shared_ptr<percipio_layer::TYImage>&  depth, std::vector<TY_VECT_3F>& p3d)
{
    if(!depth) return;

    if(depth->pixelFormat() == TY_PIXEL_FORMAT_XYZ48) {
        int16_t* src = static_cast<int16_t*>(depth->buffer());
        p3d.resize(depth->width() * depth->height());
        for (int pix = 0; pix < p3d.size(); pix++) {
            p3d[pix].x = *(src + 3*pix + 0) * f_depth_scale_unit;
            p3d[pix].y = *(src + 3*pix + 1) * f_depth_scale_unit;
            p3d[pix].z = *(src + 3*pix + 2) * f_depth_scale_unit;
        }
    }
}

void P3DCamera::processDepth16ToPoint3D(const std::shared_ptr<percipio_layer::TYImage>&  depth, const std::shared_ptr<percipio_layer::TYImage>&  color, std::vector<TY_VECT_3F>& p3d, std::shared_ptr<percipio_layer::TYImage>& registration_color)
{
    std::cout << "=== processDepth16ToPoint3D 开始处理 ===" << std::endl;
    
    if(!depth) {
        std::cout << "[错误] 深度图像为空!" << std::endl;
        return;
    }
    
    // 直接使用原始深度图像数据，避免ImageProcesser的简化实现问题
    const uint16_t* original_depth_buffer = static_cast<const uint16_t*>(depth->buffer());
    
    // 调试：检查原始深度图像数据
    int original_valid_depth = 0;
    int original_zero_depth = 0;
    int original_sample_count = std::min(20, depth->width() * depth->height());
    std::cout << "  原始深度图前" << original_sample_count << "个深度值:" << std::endl;
    for(int i = 0; i < original_sample_count; i++) {
        if(original_depth_buffer[i] > 0) {
            original_valid_depth++;
            if(i < 10) {
                std::cout << "    [" << i << "] " << original_depth_buffer[i] << std::endl;
            }
        } else {
            original_zero_depth++;
        }
    }
    std::cout << "  原始深度图统计: 有效深度值=" << original_valid_depth << ", 零值=" << original_zero_depth << std::endl;
    
    // 强制跳过RGB-D配准，直接生成点云来验证深度数据
    std::cout << "强制跳过RGB-D配准，直接生成点云" << std::endl;
    
    // 直接生成点云
    std::cout << "直接生成点云" << std::endl;
    p3d.resize(depth->width() * depth->height());
    std::cout << "  点云数组大小: " << p3d.size() << std::endl;
    
    TYMapDepthImageToPoint3d(&depth_calib, depth->width(), depth->height()
        , original_depth_buffer, &p3d[0], f_depth_scale_unit);
        
    std::cout << "  [OK] TYMapDepthImageToPoint3d完成" << std::endl;
    
    // 验证点云数据
    int valid_p3d = 0;
    int nan_p3d = 0;
    float max_coord = 0;
    float min_coord = 0;
    int sample_count = std::min(10, (int)p3d.size());
    
    std::cout << "  前" << sample_count << "个点云样本:" << std::endl;
    for(int i = 0; i < p3d.size(); i++) {
        if(std::isnan(p3d[i].x) || std::isnan(p3d[i].y) || std::isnan(p3d[i].z)) {
            nan_p3d++;
        } else {
            valid_p3d++;
            float coord_max = std::max({std::abs(p3d[i].x), std::abs(p3d[i].y), std::abs(p3d[i].z)});
            if (coord_max > max_coord) max_coord = coord_max;
            if (i < sample_count) {
                std::cout << "    [" << i << "] (" << p3d[i].x << ", " << p3d[i].y << ", " << p3d[i].z << ")" << std::endl;
            }
        }
    }
    
    std::cout << "  点云数据统计:" << std::endl;
    std::cout << "    总点数: " << p3d.size() << std::endl;
    std::cout << "    有效点: " << valid_p3d << std::endl;
    std::cout << "    NaN点: " << nan_p3d << std::endl;
    std::cout << "    最大坐标值: " << max_coord << std::endl;
    
    std::cout << "=== processDepth16ToPoint3D 处理完成，生成点云数量: " << p3d.size() << " ===" << std::endl;
}

void P3DCamera::processXYZ48ToPoint3D(const std::shared_ptr<percipio_layer::TYImage>&  depth, const std::shared_ptr<percipio_layer::TYImage>&  color, std::vector<TY_VECT_3F>& p3d, std::shared_ptr<percipio_layer::TYImage>& registration_color)
{
    std::cout << "=== processXYZ48ToPoint3D 开始处理 ===" << std::endl;
    
    if(!depth) {
        std::cout << "[错误] 深度图像为空!" << std::endl;
        return;
    }
    
    if(color) {
        std::cout << "检测到彩色图像，开始处理..." << std::endl;
        
        // 步骤1: 解析彩色图像
        std::cout << "步骤1: 解析彩色图像..." << std::endl;
        color_processer->parse(color);
        std::cout << "步骤1完成" << std::endl;
        
        // 步骤2: 执行彩色去畸变
        std::cout << "步骤2: 执行彩色去畸变..." << std::endl;
        TY_STATUS undistort_status = color_processer->doUndistortion();
        std::cout << "步骤2完成，状态: " << undistort_status << std::endl;
        
        if(TY_STATUS_OK == undistort_status) {
            registration_color = color_processer->image();
            
            // 步骤3: 处理XYZ48深度数据
            std::cout << "步骤3: 处理XYZ48深度数据..." << std::endl;
            processXYZ48(depth, p3d);
            std::cout << "  初始点云数量: " << p3d.size() << std::endl;
            
            // 步骤4: 计算外参逆矩阵
            std::cout << "步骤4: 计算外参逆矩阵..." << std::endl;
            TY_CAMERA_EXTRINSIC extri_inv;
            TYInvertExtrinsic(&color_calib.extrinsic, &extri_inv);
            std::cout << "步骤4完成" << std::endl;
            
            // 步骤5: 执行坐标系变换
            std::cout << "步骤5: 执行坐标系变换..." << std::endl;
            TYMapPoint3dToPoint3d(&extri_inv, p3d.data(), p3d.size(), p3d.data());
            std::cout << "步骤5完成" << std::endl;
            
            // 步骤6: 映射到彩色图像平面
            std::cout << "步骤6: 映射到彩色图像平面..." << std::endl;
            std::vector<uint16_t> mappedDepth(registration_color->width() * registration_color->height());
            TYMapPoint3dToDepthImage(&color_calib, p3d.data(), depth->width() * depth->height(),  registration_color->width(), registration_color->height(), mappedDepth.data(), f_depth_scale_unit);
            std::cout << "步骤6完成" << std::endl;
            
            // 步骤7: 重新生成点云
            std::cout << "步骤7: 重新生成点云..." << std::endl;
            p3d.resize(registration_color->width() * registration_color->height());
            TYMapDepthImageToPoint3d(&color_calib, registration_color->width(), registration_color->height()
                , mappedDepth.data(), &p3d[0], f_depth_scale_unit);
            std::cout << "步骤7完成" << std::endl;
            
        } else {
            std::cout << "彩色图像去畸变失败，使用普通XYZ48处理" << std::endl;
            processXYZ48(depth, p3d);
        }
    } else {
        std::cout << "没有彩色图像，使用普通XYZ48处理" << std::endl;
        processXYZ48(depth, p3d);
    }
    
    std::cout << "=== processXYZ48ToPoint3D 处理完成，生成点云数量: " << p3d.size() << " ===" << std::endl;
}

int P3DCamera::process(const std::shared_ptr<percipio_layer::TYImage>&  depth, const std::shared_ptr<percipio_layer::TYImage>&  color)
{
    std::cout << "=== P3DCamera::process 开始处理 ===" << std::endl;
    
    // 步骤1: 验证输入参数
    std::cout << "步骤1: 验证输入参数" << std::endl;
    if(!depth) {
        std::cout << "[错误] depth image is empty!" << std::endl;
        return -1;
    }
    
    std::cout << "深度图像信息:" << std::endl;
    std::cout << "  指针: " << depth.get() << std::endl;
    std::cout << "  缓冲区: " << depth->buffer() << std::endl;
    std::cout << "  大小: " << depth->size() << std::endl;
    std::cout << "  宽度: " << depth->width() << std::endl;
    std::cout << "  高度: " << depth->height() << std::endl;
    std::cout << "  像素格式: 0x" << std::hex << static_cast<int>(depth->pixelFormat()) << std::dec << std::endl;
    
    if(color) {
        std::cout << "检测到彩色图像，开始处理..." << std::endl;
        std::cout << "彩色图像信息:" << std::endl;
        std::cout << "  指针: " << color.get() << std::endl;
        std::cout << "  缓冲区: " << color->buffer() << std::endl;
        std::cout << "  大小: " << color->size() << std::endl;
        std::cout << "  宽度: " << color->width() << std::endl;
        std::cout << "  高度: " << color->height() << std::endl;
        std::cout << "  像素格式: 0x" << std::hex << static_cast<int>(color->pixelFormat()) << std::dec << std::endl;
    } else {
        std::cout << "未检测到彩色图像" << std::endl;
    }
    std::cout << "步骤1完成" << std::endl;
    
    // 步骤2: 初始化点云和配准彩色图像
    std::cout << "步骤2: 初始化点云和配准彩色图像" << std::endl;
    std::vector<TY_VECT_3F> p3d;
    std::shared_ptr<percipio_layer::TYImage> registration_color = nullptr;
    std::cout << "步骤2完成" << std::endl;
    
    // 步骤3: 根据深度图像格式选择处理流程
    std::cout << "步骤3: 根据深度图像格式选择处理流程" << std::endl;
    TY_PIXEL_FORMAT fmt = depth->pixelFormat();
    std::cout << "深度图像像素格式: 0x" << std::hex << static_cast<int>(fmt) << std::dec << std::endl;
    
    if(fmt == TY_PIXEL_FORMAT_DEPTH16) {
        std::cout << "使用DEPTH16处理流程" << std::endl;
        processDepth16ToPoint3D(depth, color, p3d, registration_color);
    }
    else if(fmt == TY_PIXEL_FORMAT_XYZ48) {
        std::cout << "使用XYZ48处理流程" << std::endl;
        processXYZ48ToPoint3D(depth, color, p3d, registration_color);
    }
    else {
        std::cout << "[错误] Invalid depth image format!" << std::endl;
        return -1;
    }
    std::cout << "步骤3完成" << std::endl;
    
    // 步骤4: 验证点云数据
    std::cout << "步骤4: 验证点云数据" << std::endl;
    std::cout << "处理完成，生成点云数量: " << p3d.size() << std::endl;
    
    if(p3d.empty()) {
        std::cout << "[警告] 生成的点云为空!" << std::endl;
        return -1;
    }
    
    // 统计有效点和NaN点
    int valid_points = 0;
    int nan_points = 0;
    for(const auto& point : p3d) {
        if(std::isnan(point.x) || std::isnan(point.y) || std::isnan(point.z)) {
            nan_points++;
        } else {
            valid_points++;
        }
    }
    std::cout << "点云统计 - 有效点: " << valid_points << ", NaN点: " << nan_points << std::endl;
    std::cout << "步骤4完成" << std::endl;
    
    // 步骤5: 保存点云到PLY文件
    std::cout << "步骤5: 保存点云到PLY文件" << std::endl;
    static int m_frame_cnt = 0;

    {
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);

        char file[32];
        struct tm* p = std::localtime(&now_time);
        sprintf(file, "%d.%d.%d %02d_%02d_%02d.ply", 1900 + p->tm_year, 1+p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
        std::cout << "Save : " << file << std::endl;
        std::cout << "点云数量: " << p3d.size() << std::endl;
        if (registration_color) {
            std::cout << "彩色图像: " << registration_color->width() << "x" << registration_color->height() << std::endl;
        } else {
            std::cout << "无彩色图像" << std::endl;
        }
        savePointsToPly(p3d, registration_color,  file);
        std::cout << file << "Saved!" << std::endl;
    }

    m_frame_cnt++;
    std::cout << "步骤5完成" << std::endl;
    
    std::cout << "=== P3DCamera::process 处理完成 ===" << std::endl;

    return 0;
}


// 初始化库并获取接口列表
static bool initializeLibAndGetInterfaces(TY_INTERFACE_INFO** pIfaceList, uint32_t* ifaceCount) {
    // 1. 首先创建TYContext实例来初始化库
    std::cout << "Initializing TYContext..." << std::endl;
    auto& context = percipio_layer::TYContext::getInstance();
    
    // 更新接口列表
    TY_STATUS status = TYUpdateInterfaceList();
    if (status != TY_STATUS_OK) {
        std::cout << "Failed to update interface list: " << status << std::endl;
        return false;
    }

    // 获取接口数量
    *ifaceCount = 0;
    status = TYGetInterfaceNumber(ifaceCount);
    if (status != TY_STATUS_OK) {
        std::cout << "Failed to get interface number: " << status << std::endl;
        return false;
    }

    if (*ifaceCount == 0) {
        std::cout << "No interface found" << std::endl;
        return false;
    }

    // 获取接口列表
    *pIfaceList = (TY_INTERFACE_INFO*)malloc(sizeof(TY_INTERFACE_INFO) * (*ifaceCount));
    if (!*pIfaceList) {
        std::cout << "Failed to allocate memory for interface list" << std::endl;
        return false;
    }
    status = TYGetInterfaceList(*pIfaceList, *ifaceCount, ifaceCount);
    if (status != TY_STATUS_OK) {
        std::cout << "Failed to get interface list: " << status << std::endl;
        free(*pIfaceList);
        *pIfaceList = NULL;
        return false;
    }

    std::cout << "Found " << *ifaceCount << " interfaces" << std::endl;
    return true;
}

// 从接口列表中打开设备
static bool openDeviceFromInterfaceList(
    TY_INTERFACE_INFO* pIfaceList, 
    const std::string& deviceID,
    uint32_t ifaceCount, TY_DEV_HANDLE* device) {
    // 打开设备
    *device = NULL;
    bool deviceOpened = false;

    std::cout << "Starting device enumeration..." << std::endl;

    // 遍历所有接口，尝试打开设备
    for (uint32_t ifaceIdx = 0; ifaceIdx < ifaceCount; ifaceIdx++) {
        std::cout << "Trying interface " << ifaceIdx << ": " << pIfaceList[ifaceIdx].id << ", type: " << pIfaceList[ifaceIdx].type << std::endl;

        // 打开接口
        TY_INTERFACE_HANDLE ifaceHandle = NULL;
        TY_STATUS status = TYOpenInterface(pIfaceList[ifaceIdx].id, &ifaceHandle);
        if (status == TY_STATUS_OK) {
            std::cout << "Interface opened successfully" << std::endl;
            
            // 更新设备列表
            status = TYUpdateDeviceList(ifaceHandle);
            if (status == TY_STATUS_OK) {
                std::cout << "Device list updated successfully" << std::endl;
            } else {
                std::cout << "Failed to update device list: " << status << std::endl;
            }
            
            // 增加设备列表缓冲区大小
            uint32_t bufferSize = 20;
            TY_DEVICE_BASE_INFO* pDeviceList = (TY_DEVICE_BASE_INFO*)malloc(sizeof(TY_DEVICE_BASE_INFO) * bufferSize);
            if (pDeviceList) {
                uint32_t deviceCount = 0;
                status = TYGetDeviceList(ifaceHandle, pDeviceList, bufferSize, &deviceCount);
                if (status == TY_STATUS_OK) {
                    std::cout << "Got device list successfully, found " << deviceCount << " devices on this interface" << std::endl;
                    
                    if (deviceCount > 0) {
                        // 打印所有设备信息
                        for (uint32_t devIdx = 0; devIdx < deviceCount; devIdx++) {
                            std::cout << "  Device " << devIdx << ": ID=" << pDeviceList[devIdx].id 
                                      << ", Vendor=" << pDeviceList[devIdx].vendorName 
                                      << ", Model=" << pDeviceList[devIdx].modelName << std::endl;
                        }
                        
                        // 选择要打开的设备
                        const char* targetDeviceID = deviceID.empty() ? pDeviceList[0].id : deviceID.c_str();
                        std::cout << "Attempting to open device: " << targetDeviceID << std::endl;
                        
                        // 尝试打开设备
                        status = TYOpenDevice(ifaceHandle, targetDeviceID, device);
                        if (status == TY_STATUS_OK) {
                            deviceOpened = true;
                            std::cout << "Device opened successfully" << std::endl;
                            std::cout << "Device ID: " << pDeviceList[0].id << std::endl;
                            std::cout << "Vendor: " << pDeviceList[0].vendorName << std::endl;
                            std::cout << "Model: " << pDeviceList[0].modelName << std::endl;
                        } else {
                            std::cout << "Failed to open device: " << status << std::endl;
                        }
                    } else {
                        std::cout << "No devices found on this interface" << std::endl;
                    }
                } else {
                    std::cout << "Failed to get device list: " << status << std::endl;
                }
                free(pDeviceList);
            } else {
                std::cout << "Failed to allocate memory for device list" << std::endl;
            }
            TYCloseInterface(ifaceHandle);
            
            // 如果已经打开设备，退出循环
            if (deviceOpened) {
                break;
            }
        } else {
            std::cout << "Failed to open interface: " << status << std::endl;
        }
    }
    return deviceOpened;
}

// 从文件读取数据的函数
std::shared_ptr<percipio_layer::TYImage> loadRawDataFromDisk(const std::string& filename, TY_PIXEL_FORMAT_LIST pixelFormat, TY_COMPONENT_ID compID) {
    // 读取元信息文件
    // 确保文件名不包含.raw扩展名，只添加.meta
    std::string metaFilename = filename;
    // 如果文件名已经包含.raw，先移除它
    size_t rawPos = metaFilename.find(".raw");
    if (rawPos != std::string::npos) {
        metaFilename = metaFilename.substr(0, rawPos);
    }
    metaFilename += ".meta";
    
    // 使用Windows API检查文件是否存在
    std::cout << "读取元信息文件: " << metaFilename << std::endl;
    
    // 尝试不同的文件打开方式
    FILE* metaFile = nullptr;
    if (GetFileAttributesA(metaFilename.c_str()) != INVALID_FILE_ATTRIBUTES) {
        metaFile = fopen(metaFilename.c_str(), "r");
        if (!metaFile) {
            metaFile = fopen(metaFilename.c_str(), "rb");
        }
    } else {
        return nullptr;
    }
    if (!metaFile) {
        std::cout << "无法打开元信息文件: " << metaFilename << std::endl;
        return nullptr;
    }
    
    int width = 0, height = 0, format = 0;
    unsigned int size = 0;
    fscanf(metaFile, "width=%d\n", &width);
    fscanf(metaFile, "height=%d\n", &height);
    fscanf(metaFile, "pixelFormat=%d\n", &format);
    fscanf(metaFile, "size=%u\n", &size);
    fclose(metaFile);
    
    std::cout << "从文件读取数据: " << filename << std::endl;
    std::cout << "  宽度: " << width << std::endl;
    std::cout << "  高度: " << height << std::endl;
    std::cout << "  像素格式: " << format << std::endl;
    std::cout << "  数据大小: " << size << " 字节" << std::endl;
    
    // 读取原始数据文件
    FILE* dataFile = fopen(filename.c_str(), "rb");
    if (!dataFile) {
        std::cout << "无法打开数据文件: " << filename << std::endl;
        // 尝试使用当前工作目录
        char cwd[1024];
        if (GetCurrentDirectoryA(1024, cwd)) {
            std::string fullPath = std::string(cwd) + "\\" + filename;
            std::cout << "尝试完整路径: " << fullPath << std::endl;
            dataFile = fopen(fullPath.c_str(), "rb");
            if (dataFile) {
                std::cout << "使用完整路径成功打开数据文件" << std::endl;
            }
        }
        if (!dataFile) {
            fclose(metaFile);
            return nullptr;
        }
    }
    
    // 分配内存并读取数据
    uint8_t* buffer = new uint8_t[size];
    size_t readSize = fread(buffer, 1, size, dataFile);
    fclose(dataFile);
    
    if (readSize != size) {
        std::cout << "数据文件读取不完整: " << readSize << " vs " << size << std::endl;
        delete[] buffer;
        return nullptr;
    }
    
    // 创建TYImage对象
    auto image = std::make_shared<percipio_layer::TYImage>(width, height, compID, 
        static_cast<TY_PIXEL_FORMAT_LIST>(format), size);
    memcpy(image->buffer(), buffer, size);
    delete[] buffer;
    
    std::cout << "  [OK] 数据加载成功" << std::endl;
    return image;
}

// 查找最新的数据文件
std::string findLatestDataFile(const std::string& prefix) {
    WIN32_FIND_DATAA findData;
    HANDLE hFind;
    std::string pattern = prefix + "*.raw";
    std::string latestFile;
    FILETIME latestTime = {0, 0};
    
    hFind = FindFirstFileA(pattern.c_str(), &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
                if (CompareFileTime(&findData.ftLastWriteTime, &latestTime) > 0) {
                    latestTime = findData.ftLastWriteTime;
                    latestFile = findData.cFileName;  // 只保存文件名
                }
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }
    return latestFile;  // 返回文件名，不包含路径
}

// 添加数据保存函数
void saveRawDataToDisk(const std::shared_ptr<percipio_layer::TYImage>& depth, const std::shared_ptr<percipio_layer::TYImage>& color) {
    if(!depth && !color) {
        std::cout << "没有数据需要保存!" << std::endl;
        return;
    }
    
    // 生成时间戳
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    struct tm* p = std::localtime(&now_time);
    char timestamp[64];
    sprintf(timestamp, "%d%02d%02d_%02d%02d%02d", 
            1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday, 
            p->tm_hour, p->tm_min, p->tm_sec);
    
    std::cout << "\n=== 保存原始数据到磁盘 ===" << std::endl;
    std::cout << "时间戳: " << timestamp << std::endl;
    
    // 保存深度数据
    if(depth) {
        std::string depthFilename = std::string("depth_") + timestamp + ".raw";
        std::cout << "保存深度数据: " << depthFilename << std::endl;
        std::cout << "  宽度: " << depth->width() << std::endl;
        std::cout << "  高度: " << depth->height() << std::endl;
        std::cout << "  像素格式: " << static_cast<int>(depth->pixelFormat()) << std::endl;
        std::cout << "  数据大小: " << depth->size() << " 字节" << std::endl;
        
        FILE* depthFile = fopen(depthFilename.c_str(), "wb");
        if(depthFile) {
            fwrite(depth->buffer(), 1, depth->size(), depthFile);
            fclose(depthFile);
            std::cout << "  [OK] 深度数据保存成功" << std::endl;
        } else {
            std::cout << "  [FAIL] 深度数据保存失败" << std::endl;
        }
        
        // 同时保存深度数据的元信息
        std::string depthMetaFilename = std::string("depth_") + timestamp + ".meta";
        FILE* depthMetaFile = fopen(depthMetaFilename.c_str(), "w");
        if(depthMetaFile) {
            fprintf(depthMetaFile, "width=%d\n", depth->width());
            fprintf(depthMetaFile, "height=%d\n", depth->height());
            fprintf(depthMetaFile, "pixelFormat=%d\n", depth->pixelFormat());
            fprintf(depthMetaFile, "size=%u\n", depth->size());
            fclose(depthMetaFile);
        }
    }
    
    // 保存彩色数据
    if(color) {
        std::string colorFilename = std::string("color_") + timestamp + ".raw";
        std::cout << "保存彩色数据: " << colorFilename << std::endl;
        std::cout << "  宽度: " << color->width() << std::endl;
        std::cout << "  高度: " << color->height() << std::endl;
        std::cout << "  像素格式: " << static_cast<int>(color->pixelFormat()) << std::endl;
        std::cout << "  数据大小: " << color->size() << " 字节" << std::endl;
        
        FILE* colorFile = fopen(colorFilename.c_str(), "wb");
        if(colorFile) {
            fwrite(color->buffer(), 1, color->size(), colorFile);
            fclose(colorFile);
            std::cout << "  [OK] 彩色数据保存成功" << std::endl;
        } else {
            std::cout << "  [FAIL] 彩色数据保存失败" << std::endl;
        }
        
        // 同时保存彩色数据的元信息
        std::string colorMetaFilename = std::string("color_") + timestamp + ".meta";
        FILE* colorMetaFile = fopen(colorMetaFilename.c_str(), "w");
        if(colorMetaFile) {
            fprintf(colorMetaFile, "width=%d\n", color->width());
            fprintf(colorMetaFile, "height=%d\n", color->height());
            fprintf(colorMetaFile, "pixelFormat=%d\n", color->pixelFormat());
            fprintf(colorMetaFile, "size=%u\n", color->size());
            fclose(colorMetaFile);
        }
    }
    
    std::cout << "=== 数据保存完成 ===\n" << std::endl;
}

int main(int argc, char* argv[])
{
    // 设置控制台为UTF-8编码，解决中文乱码问题
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    
    std::string ID;
    bool useCamera = true;  // 默认使用摄像头
    std::string depthFile;
    std::string colorFile;
    
    for(int i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-id") == 0) {
            ID = argv[++i];
        } else if(strcmp(argv[i], "-file") == 0) {
            useCamera = false;  // 使用文件数据
            std::cout << "使用文件模式：从磁盘读取数据" << std::endl;
        } else if(strcmp(argv[i], "-depth") == 0) {
            depthFile = argv[++i];
            useCamera = false;  // 指定深度文件时使用文件模式
        } else if(strcmp(argv[i], "-color") == 0) {
            colorFile = argv[++i];
            useCamera = false;  // 指定彩色文件时使用文件模式
        } else if(strcmp(argv[i], "-h") == 0) {
            std::cout << "Usage: " << argv[0] << "   [-h] [-id <ID>] [-file] [-depth <depth_file>] [-color <color_file>]" << std::endl;
            std::cout << "  -h      : 显示帮助信息" << std::endl;
            std::cout << "  -id <ID>: 指定设备ID" << std::endl;
            std::cout << "  -file   : 使用文件模式（从磁盘读取数据）" << std::endl;
            std::cout << "  -depth <depth_file>: 指定深度数据文件" << std::endl;
            std::cout << "  -color <color_file>: 指定彩色数据文件" << std::endl;
            return 0;
        }
    }

    // 初始化TY库
    LOGD("=== Init lib");
    ASSERT_OK( TYInitLib() );
    TY_VERSION_INFO ver;
    ASSERT_OK( TYLibVersion(&ver) );
    LOGD("     - lib version: %u.%u.%u", ver.major, ver.minor, ver.patch);

    std::shared_ptr<percipio_layer::TYImage> depth, color;
    
    if (useCamera) {
        // 使用摄像头模式
        std::cout << "=== 摄像头模式 ===" << std::endl;
        
        P3DCamera _3dcam;
        if(TY_STATUS_OK != _3dcam.open(ID.c_str())) {
            std::cout << "open camera failed!" << std::endl;
            return -1;
        }

        _3dcam.Init();
        if(TY_STATUS_OK != _3dcam.start()) {
            std::cout << "stream start failed!" << std::endl;
            return -1;
        }

        auto frame = _3dcam.tryGetFrames(2000);
        if(frame) {
            depth = frame->depthImage();
            color = frame->colorImage();
            
            // Save raw data to disk
            std::cout << "Save raw data to disk!" << std::endl;
            saveRawDataToDisk(depth, color);
            
            if(depth && _3dcam.process(depth, color) < 0) {
                std::cout << "Failed to process depth image!" << std::endl;
            }
        }
    } else {
        // 使用文件模式
        std::cout << "=== 文件模式 ===" << std::endl;
        
        // 如果指定了具体的文件，使用指定的文件；否则查找最新的文件
        if (depthFile.empty()) {
            depthFile = findLatestDataFile("depth_");
        }
        if (colorFile.empty()) {
            colorFile = findLatestDataFile("color_");
        }
        
        if (depthFile.empty()) {
            std::cout << "未找到深度数据文件！" << std::endl;
            return -1;
        }
        
        std::cout << "使用深度数据文件: " << depthFile << std::endl;
        depth = loadRawDataFromDisk(depthFile, TY_PIXEL_FORMAT_DEPTH16, TY_COMPONENT_DEPTH_CAM);
        
        if (!colorFile.empty()) {
            std::cout << "使用彩色数据文件: " << colorFile << std::endl;
            color = loadRawDataFromDisk(colorFile, TY_PIXEL_FORMAT_RGB, TY_COMPONENT_RGB_CAM);
        }
        
        if (!depth) {
            std::cout << "无法加载深度数据！" << std::endl;
            return -1;
        }
        
        // 创建P3DCamera实例来处理数据
        P3DCamera _3dcam;
        
        // 文件模式初始化（不需要相机设备）
        if (_3dcam.InitForFileMode() != TY_STATUS_OK) {
            std::cout << "文件模式初始化失败！" << std::endl;
            return -1;
        }
        
        if(depth && _3dcam.process(depth, color) < 0) {
            std::cout << "Failed to process depth image!" << std::endl;
        }
    }

    std::cout << "Main done!" << std::endl;
    
    // 清理TY库
    ASSERT_OK( TYDeinitLib() );
    
    return 0;
}