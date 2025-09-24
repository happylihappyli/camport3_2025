#include <iostream>
#include <chrono>
#include <sstream>
#include <iomanip>

#include "Device.hpp"
#include "TYImageProc.h"

#if _WIN32
#include <conio.h>
#elif __linux__
#include <termio.h>
#define TTY_PATH    "/dev/tty"
#define STTY_DEF    "stty -raw echo -F"
#endif

using namespace percipio_layer;

static char GetChar(void)
{
    char ch = -1;
#if _WIN32
    if (kbhit()) //check fifo
    {
        ch = getche(); //read fifo
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

class P3DCamera : public FastCamera
{
    public:
        P3DCamera() : FastCamera() {};
        ~P3DCamera() {}; 

        TY_STATUS Init();
        int process(const std::shared_ptr<TYImage>&  depth, const std::shared_ptr<TYImage>&  color);

    private:
        float f_depth_scale_unit = 1.f;
        bool depth_needUndistort = false;
        bool b_points_with_color = false;
        TY_CAMERA_CALIB_INFO depth_calib, color_calib;
        std::shared_ptr<ImageProcesser> depth_processer;
        std::shared_ptr<ImageProcesser> color_processer;
        void savePointsToPly(const std::vector<TY_VECT_3F>& p3d, const std::shared_ptr<TYImage>& color, const char* fileName);
        void processDepth16(const std::shared_ptr<TYImage>&  depth, std::vector<TY_VECT_3F>& p3d);
        void processXYZ48(const std::shared_ptr<TYImage>&  depth, std::vector<TY_VECT_3F>& p3d);

        void processDepth16ToPoint3D(const std::shared_ptr<TYImage>&  depth, const std::shared_ptr<TYImage>&  color, std::vector<TY_VECT_3F>& p3d,  std::shared_ptr<TYImage>& registration_color);
        void processXYZ48ToPoint3D(const std::shared_ptr<TYImage>&  depth, const std::shared_ptr<TYImage>&  color, std::vector<TY_VECT_3F>& p3d,  std::shared_ptr<TYImage>& registration_color);
};

TY_STATUS P3DCamera::Init()
{
    TY_STATUS status;
    std::cout << "开始相机初始化..." << std::endl;
    
    // 辅助函数：检查并设置相机分辨率
    auto checkAndSetResolution = [this](TY_COMPONENT_ID component, TY_PIXEL_FORMAT format, int width, int height, const std::string& cameraName) {
        TY_STATUS res;
        
        // 获取相机支持的所有图像模式
        TY_ENUM_ENTRY modes[20]; // 假设最多支持20种模式
        uint32_t num_modes = 0;
        res = TYGetEnumEntryInfo(handle(), component, TY_ENUM_IMAGE_MODE, modes, 20, &num_modes);
        if (res != TY_STATUS_OK) {
            std::cerr << cameraName << "获取支持的分辨率模式失败，状态码: " << res << std::endl;
            return res;
        }
        
        // 打印所有支持的分辨率模式
                std::cout << cameraName << "支持的分辨率模式: " << std::endl;
                bool has_target_mode = false;
                uint32_t target_mode_value = 0;
                TY_IMAGE_MODE target_mode = TYImageMode2(format, width, height);
                
                for (uint32_t i = 0; i < num_modes; i++) {
                    int w = TYImageWidth(modes[i].value);
                    int h = TYImageHeight(modes[i].value);
                    TY_PIXEL_FORMAT pf = TYPixelFormat(modes[i].value);
                    std::cout << "  [" << i << "] " << modes[i].description << " (" << w << "x" << h << ")" << std::endl;
                    
                    // 检查是否支持目标分辨率
                    if (modes[i].value == target_mode) {
                        has_target_mode = true;
                        target_mode_value = modes[i].value;
                        break;
                    }
                }
                
                // 设置分辨率
                if (has_target_mode) {
                    std::cout << "设置" << cameraName << "分辨率为" << width << "x" << height << std::endl;
                    res = TYSetEnum(handle(), component, TY_ENUM_IMAGE_MODE, target_mode_value);
                    if (res != TY_STATUS_OK) {
                        std::cerr << cameraName << "分辨率设置失败，状态码: " << res << std::endl;
                    } else {
                        std::cout << cameraName << "分辨率设置成功" << std::endl;
                    }
                } else {
                    // 使用默认分辨率（第一个可用模式）
                    if (num_modes > 0) {
                        int w = TYImageWidth(modes[0].value);
                        int h = TYImageHeight(modes[0].value);
                        TY_PIXEL_FORMAT pf = TYPixelFormat(modes[0].value);
                        std::cout << "警告: " << cameraName << "不支持" << width << "x" << height << "分辨率，使用默认分辨率: " << w << "x" << h << std::endl;
                        res = TYSetEnum(handle(), component, TY_ENUM_IMAGE_MODE, modes[0].value);
                    } else {
                        std::cerr << "错误: " << cameraName << "没有可用的分辨率模式" << std::endl;
                        res = TY_STATUS_ERROR;
                    }
                }
        
        return res;
    };
    
    // 设置深度相机分辨率
    TY_STATUS depth_res = checkAndSetResolution(TY_COMPONENT_DEPTH_CAM, TY_PIXEL_FORMAT_DEPTH16, 1280, 960, "深度相机");
    if (depth_res != TY_STATUS_OK) {
        return depth_res;
    }
    
    status = stream_enable(stream_depth);
    if(status != TY_STATUS_OK) {
        std::cerr << "深度流使能失败，状态码: " << status << std::endl;
        return status;
    }
    std::cout << "深度流使能成功" << std::endl;
    
    if(has_stream(stream_color)) { 
        // 设置RGB相机分辨率（YUYV格式）
        TY_STATUS color_res = checkAndSetResolution(TY_COMPONENT_RGB_CAM, TY_PIXEL_FORMAT_YUYV, 1280, 960, "RGB相机");
        if (color_res != TY_STATUS_OK) {
            return color_res;
        }
        
        status = stream_enable(stream_color);
        if(status != TY_STATUS_OK) {
            std::cerr << "颜色流使能失败，状态码: " << status << std::endl;
            return status;
        }
        std::cout << "颜色流使能成功" << std::endl;
        bool hasColorCalib = false;
        ASSERT_OK(TYHasFeature(handle(), TY_COMPONENT_RGB_CAM, TY_STRUCT_CAM_CALIB_DATA, &hasColorCalib));
        if (hasColorCalib)
        {
            ASSERT_OK(TYGetStruct(handle(), TY_COMPONENT_RGB_CAM, TY_STRUCT_CAM_CALIB_DATA, &color_calib, sizeof(color_calib)));
            color_processer = std::shared_ptr<ImageProcesser>(new ImageProcesser("color", &color_calib));
            std::cout << "颜色相机校准数据已加载" << std::endl;
        }
    }
    
    ASSERT_OK(TYGetFloat(handle(), TY_COMPONENT_DEPTH_CAM, TY_FLOAT_SCALE_UNIT, &f_depth_scale_unit));
    ASSERT_OK(TYHasFeature(handle(), TY_COMPONENT_DEPTH_CAM, TY_STRUCT_CAM_DISTORTION, &depth_needUndistort));
    ASSERT_OK(TYGetStruct(handle(), TY_COMPONENT_DEPTH_CAM, TY_STRUCT_CAM_CALIB_DATA, &depth_calib, sizeof(depth_calib)));
    depth_processer = std::shared_ptr<ImageProcesser>(new ImageProcesser("depth", &depth_calib));
    
    std::cout << "相机初始化完成，深度比例因子: " << f_depth_scale_unit << ", 需要深度畸变校正: " << (depth_needUndistort ? "是" : "否") << std::endl;
    return TY_STATUS_OK;
}


// 将点云和可选的颜色信息保存为PLY文件格式
// @param p3d 点云数据，包含X、Y、Z坐标
// @param color 颜色图像数据，支持多种像素格式
// @param fileName 输出PLY文件路径
void P3DCamera::savePointsToPly(const std::vector<TY_VECT_3F>& p3d, const std::shared_ptr<TYImage>& color, const char* fileName)
{
    std::cout << "开始保存PLY文件: " << fileName << std::endl;
    // 检查输入参数有效性
    if (fileName == nullptr || p3d.empty()) {
        std::cerr << "无效的输入参数：文件名不能为空或点云数据为空" << std::endl;
        return;
    }
    std::cout << "点云数据点数: " << p3d.size() << std::endl;

    // 打开文件并检查是否成功
    FILE* fp = fopen(fileName, "wb+");
    if (fp == nullptr) {
        std::cerr << "无法打开文件：" << fileName << std::endl;
        return;
    }
    std::cout << "文件打开成功" << std::endl;

    // 计算有效点的数量
    int pointsCnt = 0;
    for (const auto& point : p3d) {
        if (!std::isnan(point.z)) {
            pointsCnt++;
        }
    }
    std::cout << "有效点数量: " << pointsCnt << std::endl;

    // 写入PLY文件头
    fprintf(fp, "ply\n");
    fprintf(fp, "format ascii 1.0\n");
    fprintf(fp, "element vertex %d\n", pointsCnt);
    fprintf(fp, "property float x\n");
    fprintf(fp, "property float y\n");
    fprintf(fp, "property float z\n");
    bool hasColor = (color != nullptr);
    if (hasColor) {
        std::cout << "包含颜色信息，颜色图像格式: " << color->pixelFormat() << std::endl;
        fprintf(fp, "property uchar blue\n");
        fprintf(fp, "property uchar green\n");
        fprintf(fp, "property uchar red\n");
    }
    fprintf(fp, "end_header\n");
    std::cout << "PLY文件头写入完成" << std::endl;

    // 写入点云数据
    const TY_VECT_3F* point = p3d.data();
    if (hasColor) {
        int32_t bpp = TYBitsPerPixel(color->pixelFormat());
        uint8_t* pixels = (uint8_t*)color->buffer();

        switch (bpp) {
            case 8: { // mono8
                uint8_t* mono8 = pixels;
                for (size_t i = 0; i < p3d.size(); i++) {
                    if (!std::isnan(point->z)) {
                        fprintf(fp, "%.6f %.6f %.6f %u %u %u\n", 
                                point->x , point->y , point->z ,
                                (uint32_t)mono8[i], (uint32_t)mono8[i], (uint32_t)mono8[i]);
                    }
                    point++;
                }
                break;
            }
            case 16: { // mono16
                uint16_t* mono16 = (uint16_t*)pixels;
                for (size_t i = 0; i < p3d.size(); i++) {
                    if (!std::isnan(point->z)) {
                        uint8_t val = static_cast<uint8_t>(mono16[i] >> 8);
                        fprintf(fp, "%.6f %.6f %.6f %u %u %u\n", 
                                point->x , point->y , point->z ,
                                (uint32_t)val, (uint32_t)val, (uint32_t)val);
                    }
                    point++;
                }
                break;
            }
            case 24: { // bgr888
                uint8_t* bgr = pixels;
                for (size_t i = 0; i < p3d.size(); i++) {
                    if (!std::isnan(point->z)) {
                        fprintf(fp, "%.6f %.6f %.6f %u %u %u\n", 
                                point->x , point->y , point->z ,
                                (uint32_t)bgr[3 * i], (uint32_t)bgr[3 * i + 1], (uint32_t)bgr[3 * i + 2]);
                    }
                    point++;
                }
                break;
            }
            case 48: { // bgr16
                uint16_t* bgr16 = (uint16_t*)pixels;
                for (size_t i = 0; i < p3d.size(); i++) {
                    if (!std::isnan(point->z)) {
                        fprintf(fp, "%.6f %.6f %.6f %u %u %u\n", 
                                point->x , point->y , point->z ,
                                (uint32_t)(bgr16[3 * i] >> 8), (uint32_t)(bgr16[3 * i + 1] >> 8), (uint32_t)(bgr16[3 * i + 2] >> 8));
                    }
                    point++;
                }
                break;
            }
            default: {
                std::cout << "不支持的颜色格式!" << std::endl;
                // 无颜色信息的点云
                for (size_t i = 0; i < p3d.size(); i++) {
                    if (!std::isnan(point->z)) {
                        fprintf(fp, "%.6f %.6f %.6f\n", 
                                point->x , point->y , point->z);
                    }
                    point++;
                }
                break;
            }
        }
    } else {
        // 无颜色信息的点云
        for (size_t i = 0; i < p3d.size(); i++) {
            if (!std::isnan(point->z)) {
                fprintf(fp, "%.6f %.6f %.6f\n", 
                        point->x , point->y , point->z);
            }
            point++;
        }
    }

    // 确保数据写入并关闭文件
    fflush(fp);
    fclose(fp);
    std::cout << "PLY文件保存完成: " << fileName << "，总大小约" << pointsCnt * 24 / 1024 << "KB" << std::endl;
}

void P3DCamera::processDepth16(const std::shared_ptr<TYImage>&  depth, std::vector<TY_VECT_3F>& p3d)
{
    if(!depth) return;

    if(depth->pixelFormat() == TY_PIXEL_FORMAT_DEPTH16) {
        p3d.resize(depth->width() * depth->height());
        TYMapDepthImageToPoint3d(&depth_calib, depth->width(), depth->height(), (uint16_t*)depth->buffer(), &p3d[0], f_depth_scale_unit);
    }
}

void P3DCamera::processXYZ48(const std::shared_ptr<TYImage>&  depth, std::vector<TY_VECT_3F>& p3d)
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

void P3DCamera::processDepth16ToPoint3D(const std::shared_ptr<TYImage>&  depth, const std::shared_ptr<TYImage>&  color, std::vector<TY_VECT_3F>& p3d, std::shared_ptr<TYImage>& registration_color)
{
    if(!depth) {
        std::cerr << "深度图像为空，无法处理" << std::endl;
        return;
    }

    std::cout << "开始处理深度图像: 宽度=" << depth->width() << ", 高度=" << depth->height() << ", 格式=" << depth->pixelFormat() << std::endl;
    depth_processer->parse(depth);
    if(depth_needUndistort) {
        std::cout << "执行深度图像畸变校正" << std::endl;
        depth_processer->doUndistortion();
        std::cout << "深度图像畸变校正完成" << std::endl;
    }
    
    if(color) { 
        std::cout << "检测到颜色图像: 宽度=" << color->width() << ", 高度=" << color->height() << ", 格式=" << color->pixelFormat() << std::endl;
        std::cout << "解析颜色图像数据" << std::endl;
        color_processer->parse(color);
        std::cout << "准备颜色图像畸变校正" << std::endl;
        TY_STATUS status = color_processer->doUndistortion();
        std::cout << "颜色图像畸变校正结束: 状态码=" << status << std::endl;
        if(TY_STATUS_OK == status) { 
            std::cout << "颜色图像畸变校正成功，开始RGBD配准" << std::endl;
            //do rgbd registration
            const std::shared_ptr<TYImage>& depth_image = depth_processer->image();
            const std::shared_ptr<TYImage>& color_image = color_processer->image();
            int dstW = depth_image->width();
            int dstH = depth_image->width() * color_image->height() / color_image->width();
            std::cout << "配准目标尺寸: 宽度=" << dstW << ", 高度=" << dstH << std::endl;
            std::shared_ptr<TYImage> registration_depth = std::shared_ptr<TYImage>(new TYImage(dstW, dstH, 
                                                                depth_image->componentID(), 
                                                                TY_PIXEL_FORMAT_DEPTH16, 
                                                                sizeof(uint16_t) * dstW * dstH));

            TYMapDepthImageToColorCoordinate(
                &depth_calib,
                depth_image->width(), depth_image->height(), static_cast<const uint16_t*>(depth_image->buffer()),
                &color_calib,
                registration_depth->width(), registration_depth->height(), static_cast<uint16_t*>(registration_depth->buffer()), f_depth_scale_unit
            );
            //registration_depth->resize(color_image->width(), color_image->height());
            registration_color = color_image;
            p3d.resize(registration_depth->width() * registration_depth->height());
            TYMapDepthImageToPoint3d(&color_calib, registration_depth->width(), registration_depth->height()
                , (uint16_t*)registration_depth->buffer(), &p3d[0], f_depth_scale_unit);
        } else {
            std::cerr << "颜色图像畸变校正失败，状态码: " << status << std::endl;
            processDepth16(depth_processer->image(), p3d);
        }
    } else {
        processDepth16(depth_processer->image(), p3d);
    }
}

void P3DCamera::processXYZ48ToPoint3D(const std::shared_ptr<TYImage>&  depth, const std::shared_ptr<TYImage>&  color, std::vector<TY_VECT_3F>& p3d, std::shared_ptr<TYImage>& registration_color)
{
    if(!depth) return;
    
    if(color) { 
        color_processer->parse(color);
        if(TY_STATUS_OK == color_processer->doUndistortion()) { 
            registration_color = color_processer->image();

            processXYZ48(depth, p3d);

            TY_CAMERA_EXTRINSIC extri_inv;
            TYInvertExtrinsic(&color_calib.extrinsic, &extri_inv);
            TYMapPoint3dToPoint3d(&extri_inv, p3d.data(), p3d.size(), p3d.data());

            std::vector<uint16_t> mappedDepth(registration_color->width() * registration_color->height());
            TYMapPoint3dToDepthImage(&color_calib, p3d.data(), depth->width() * depth->height(),  registration_color->width(), registration_color->height(), mappedDepth.data(), f_depth_scale_unit);
            p3d.resize(registration_color->width() * registration_color->height());
            TYMapDepthImageToPoint3d(&color_calib, registration_color->width(), registration_color->height()
                , mappedDepth.data(), &p3d[0], f_depth_scale_unit);
        } else {
            processXYZ48(depth, p3d);
        }
    } else {
        processXYZ48(depth, p3d);
    }
}

int P3DCamera::process(const std::shared_ptr<TYImage>&  depth, const std::shared_ptr<TYImage>&  color)
{
    std::cout << "开始处理图像帧..." << std::endl;
    std::vector<TY_VECT_3F> p3d;
    std::shared_ptr<TYImage> registration_color = nullptr;
    
    if(!depth) { 
        std::cerr << "深度图像为空，无法处理" << std::endl;
        return -1;
    }
    
    TY_PIXEL_FORMAT fmt = depth->pixelFormat();
    std::cout << "深度图像格式: " << fmt << std::endl;
    
    if(fmt == TY_PIXEL_FORMAT_DEPTH16) {
        std::cout << "处理DEPTH16格式图像" << std::endl;
        processDepth16ToPoint3D(depth, color, p3d, registration_color);
    } else if(fmt == TY_PIXEL_FORMAT_XYZ48) {
        std::cout << "处理XYZ48格式图像" << std::endl;
        processXYZ48ToPoint3D(depth, color, p3d, registration_color);
    } else { 
        std::cerr << "无效的深度图像格式!" << std::endl;
        return -1;
    }

    static int m_frame_cnt = 0;
    std::cout << "准备执行拍照操作..." << std::endl;
    
    // 直接执行拍照操作
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);

    char file[32];
    struct tm* p = std::localtime(&now_time);
    if (p == nullptr) {
        std::cerr << "获取本地时间失败" << std::endl;
        return -1;
    }
    sprintf(file, "%d.%d.%d %02d_%02d_%02d.ply", 1900 + p->tm_year, 1+p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
    std::cout << "Save : " << file << std::endl;
    savePointsToPly(p3d, registration_color,  file);
    std::cout << file << "Saved!" << std::endl;
    
    std::cout << "拍照完成，准备返回-1退出程序" << std::endl;
    // 拍照后返回-1表示退出
    return -1;
    
    m_frame_cnt++;

    return 0;
}


int main(int argc, char* argv[])
{
    std::cout << "程序启动，版本: PointCloud_v2" << std::endl;
    std::string ID;
    for(int i = 1; i < argc; i++) { 
        if(strcmp(argv[i], "-id") == 0) {
            ID = argv[++i];
            std::cout << "指定相机ID: " << ID << std::endl;
        } else if(strcmp(argv[i], "-h") == 0) { 
            std::cout << "Usage: " << argv[0] << "   [-h] [-id <ID>]" << std::endl;
            return 0;
        }
    }

    std::cout << "创建相机对象..." << std::endl;
    P3DCamera _3dcam;
    std::cout << "打开相机..." << std::endl;
    if(TY_STATUS_OK != _3dcam.open(ID.c_str())) { 
        std::cerr << "open camera failed!" << std::endl;
        return -1;
    }
    std::cout << "相机打开成功" << std::endl;

    std::cout << "初始化相机..." << std::endl;
    _3dcam.Init();
    std::cout << "启动相机流..." << std::endl;
    if(TY_STATUS_OK != _3dcam.start()) { 
        std::cerr << "stream start failed!" << std::endl;
        return -1;
    }
    std::cout << "相机流启动成功" << std::endl;

    // 程序将自动拍照并保存后退出
    std::cout << "开始尝试获取图像帧..." << std::endl;
    bool process_exit = false;
    while(!process_exit) { 
        std::cout << "尝试获取一帧数据(等待2000ms)..." << std::endl;
        auto frame = _3dcam.tryGetFrames(2000);
        if(frame) { 
            std::cout << "成功获取一帧数据" << std::endl;
            auto depth = frame->depthImage();
            auto color = frame->colorImage();
            std::cout << "深度图像: " << (depth ? "存在" : "不存在") << ", 颜色图像: " << (color ? "存在" : "不存在") << std::endl;
            if(depth && _3dcam.process(depth, color) < 0) { 
                std::cout << "处理完成，准备退出程序" << std::endl;
                process_exit = true;
            }
        } else {
            std::cerr << "未能获取图像帧，超时2000ms" << std::endl;
        }
    }
    
    std::cout << "Main done!" << std::endl;
    return 0;
}
