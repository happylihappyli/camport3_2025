#include <thread>
#include <iostream>
#include <algorithm>

#include "../hpp/Frame.hpp"  // 使用包含虚函数版本的头文件
#include "../common/funny_resize.hpp" // 添加funny_resize头文件
#include "../common/DepthRender.hpp"  // 添加DepthRender头文件
#include "../common/common.hpp"        // 添加common头文件
#include "../../include/TYImageProc.h" // 添加TYImageProc.h以获取TYUndistortImage函数

#ifdef OPENCV_DEPENDENCIES
#include <opencv2/opencv.hpp>
#endif

namespace percipio_layer {

bool TYImage::resize(int w, int h)
{
    // 无论是否定义了OPENCV_DEPENDENCIES，都使用funny_resize函数
    if (!buffer() || w <= 0 || h <= 0) {
        return false;
    }

    int32_t new_size = 0;
    void* new_buffer = nullptr;
    
    // 根据图像格式计算新的大小并分配内存
    switch(image_data.pixelFormat)
    {
        case TY_PIXEL_FORMAT_BGR:
        case TY_PIXEL_FORMAT_RGB:
            new_size = w * h * 3; // 3通道8位图像
            new_buffer = malloc(new_size);
            if (new_buffer) {
                // 使用双线性插值
                funny_resize(width(), height(), static_cast<const uint8_t*>(buffer()), 3,
                             w, h, static_cast<uint8_t*>(new_buffer), INTER_LINEAR);
            }
            break;
        case TY_PIXEL_FORMAT_MONO:
            new_size = w * h; // 单通道8位图像
            new_buffer = malloc(new_size);
            if (new_buffer) {
                // 使用双线性插值
                funny_resize(width(), height(), static_cast<const uint8_t*>(buffer()), 1,
                             w, h, static_cast<uint8_t*>(new_buffer), INTER_LINEAR);
            }
            break;
        case TY_PIXEL_FORMAT_MONO16:
            new_size = w * h * 2; // 单通道16位图像
            new_buffer = malloc(new_size);
            if (new_buffer) {
                // 使用双线性插值
                funny_resize_16bit(width(), height(), static_cast<const uint16_t*>(buffer()),
                                   w, h, static_cast<uint16_t*>(new_buffer), INTER_LINEAR);
            }
            break;
        case TY_PIXEL_FORMAT_BGR48:
        case TY_PIXEL_FORMAT_RGB48:
            new_size = w * h * 6; // 3通道16位图像
            new_buffer = malloc(new_size);
            if (new_buffer) {
                // BGR48/RGB48格式需要特殊处理，这里简化处理
                // 实际应用中可能需要根据具体格式调整
                std::cout << "Warning: BGR48/RGB48 resize may not be fully supported!" << std::endl;
                
                // 对于多通道16位图像，我们需要逐通道处理
                const uint16_t* src_data = static_cast<const uint16_t*>(buffer());
                uint16_t* dst_data = static_cast<uint16_t*>(new_buffer);
                
                // 临时单通道缓冲区
                std::vector<uint16_t> temp_src(width() * height());
                std::vector<uint16_t> temp_dst(w * h);
                
                // 分别处理每个通道
                for (int c = 0; c < 3; c++) {
                    // 提取单个通道
                    for (int i = 0; i < width() * height(); i++) {
                        temp_src[i] = src_data[i * 3 + c];
                    }
                    
                    // 对单个通道进行resize
                    funny_resize_16bit(width(), height(), temp_src.data(),
                                      w, h, temp_dst.data(), INTER_LINEAR);
                    
                    // 放回目标图像
                    for (int i = 0; i < w * h; i++) {
                        dst_data[i * 3 + c] = temp_dst[i];
                    }
                }
            }
            break;
        case TY_PIXEL_FORMAT_DEPTH16:
            new_size = w * h * 2; // 单通道16位深度图
            new_buffer = malloc(new_size);
            if (new_buffer) {
                // 深度图通常使用最近邻插值
                funny_resize_16bit(width(), height(), static_cast<const uint16_t*>(buffer()),
                                   w, h, static_cast<uint16_t*>(new_buffer), INTER_NEAREST);
            }
            break;
        default:
            std::cout << "Image format not supported for resize!" << std::endl;
            return false;
    }
    
    // 更新图像数据
    if (new_buffer) {
        // 暂时注释掉m_isOwner相关代码，因为编译器报错未声明
        /*if (m_isOwner && image_data.buffer) {
            free(image_data.buffer);
        }*/
        image_data.buffer = new_buffer;
        image_data.size = new_size;
        image_data.width = w;
        image_data.height = h;
        // m_isOwner = true; // 现在我们拥有这个缓冲区
        return true;
    }
    
    std::cout << "Failed to allocate memory for resized image!" << std::endl;
    return false;
}

// TYImage 构造函数实现
TYImage::TYImage() {
    memset(&image_data, 0, sizeof(TY_IMAGE_DATA));
}

TYImage::TYImage(const TY_IMAGE_DATA& image) {
    memcpy(&image_data, &image, sizeof(TY_IMAGE_DATA));
}

TYImage::TYImage(const TYImage& src) {
    memcpy(&image_data, &src.image_data, sizeof(TY_IMAGE_DATA));
}

TYImage::TYImage(int32_t width, int32_t height, TY_COMPONENT_ID compID, TY_PIXEL_FORMAT format, int32_t size) {
    memset(&image_data, 0, sizeof(TY_IMAGE_DATA));
    image_data.width = width;
    image_data.height = height;
    image_data.componentID = compID;
    image_data.pixelFormat = format;
    image_data.size = size;
    if (size > 0) {
        image_data.buffer = malloc(size);
    }
}

// TYImage 析构函数实现 - 简化版本，不使用m_isOwner
TYImage::~TYImage() {
    // 暂时不释放缓冲区，因为我们无法确定所有权
    image_data.buffer = NULL;
}

// ImageProcesser 构造函数实现
ImageProcesser::ImageProcesser(const char* win, const TY_CAMERA_CALIB_INFO* calib_data, const TY_ISP_HANDLE isp_handle) {
    if (win) {
        win_name = win;
        hasWin = true;
    } else {
        hasWin = false;
    }
    color_isp_handle = isp_handle;
    if (calib_data) {
        _calib_data.reset(new TY_CAMERA_CALIB_INFO);
        memcpy(&*_calib_data, calib_data, sizeof(TY_CAMERA_CALIB_INFO));
    } else {
        _calib_data.reset();
    }
    _image.reset();
}

int ImageProcesser::parse(const std::shared_ptr<TYImage>& image)
{
    std::cout << "[ImageProcesser] 开始解析图像..." << std::endl;
    
    // 检查输入图像是否有效
    if(!image) {
        std::cerr << "[ImageProcesser] 错误: 输入图像指针为空!" << std::endl;
        return -1;
    }
    
    // 检查图像缓冲区是否有效
    if(!image->buffer()) {
        std::cerr << "[ImageProcesser] 错误: 输入图像缓冲区为空!" << std::endl;
        return -1;
    }
    
    TY_PIXEL_FORMAT format = image->pixelFormat();
    std::cout << "[ImageProcesser] 图像格式: " << format 
              << ", 宽度: " << image->width() 
              << ", 高度: " << image->height() 
              << ", 大小: " << image->size() / 1024 << "KB" << std::endl;
    
#ifndef OPENCV_DEPENDENCIES
    std::cout << win() << " image size : " << image->width() << " x " << image->height() << std::endl;
#endif
    
    switch(format) {
        /*
        case TY_PIXEL_FORMAT_BGR:
        case TY_PIXEL_FORMAT_RGB:
        case TY_PIXEL_FORMAT_MONO:
        case TY_PIXEL_FORMAT_MONO16:
        case TY_PIXEL_FORMAT_BGR48:
        case TY_PIXEL_FORMAT_RGB48:
        */
        case TY_PIXEL_FORMAT_DEPTH16:
        {
            std::cout << "[ImageProcesser] 处理DEPTH16格式图像..." << std::endl;
            _image = std::shared_ptr<TYImage>(new TYImage(*image));
            std::cout << "[ImageProcesser] DEPTH16格式图像处理完成，_image已设置" << std::endl;
            return 0;
        }
        case TY_PIXEL_FORMAT_XYZ48:
        {
            std::cout << "[ImageProcesser] 处理XYZ48格式图像..." << std::endl;
            std::vector<int16_t> depth_data(image->width() * image->height());
            int16_t* src = static_cast<int16_t*>(image->buffer());
            if(!src) {
                std::cerr << "[ImageProcesser] 错误: XYZ48格式图像缓冲区无效!" << std::endl;
                return -1;
            }
            
            for (int pix = 0; pix < image->width()*image->height(); pix++) {
                depth_data[pix] = *(src + 3*pix + 2);
            }
            
            std::cout << "[ImageProcesser] XYZ48数据转换完成，准备创建DEPTH16格式图像..." << std::endl;
            _image = std::shared_ptr<TYImage>(new TYImage(image->width(), image->height(), image->componentID(), TY_PIXEL_FORMAT_DEPTH16, depth_data.size() * sizeof(int16_t)));
            if(_image && _image->buffer()) {
                memcpy(_image->buffer(), depth_data.data(), image->size());
                std::cout << "[ImageProcesser] XYZ48格式图像处理完成，_image已设置" << std::endl;
                return 0;
            } else {
                std::cerr << "[ImageProcesser] 错误: 创建DEPTH16图像失败!" << std::endl;
                return -1;
            }
        }
        default:
        {
            std::cout << "[ImageProcesser] 处理其他格式图像..." << std::endl;
#ifdef OPENCV_DEPENDENCIES
            funny_Mat cvImage;
            int32_t         image_size;
            TY_PIXEL_FORMAT image_fmt;
            TY_COMPONENT_ID comp_id;
            comp_id = image->componentID();
            
            std::cout << "[ImageProcesser] 调用parseImage函数解析图像..." << std::endl;
            const TY_IMAGE_DATA* imgDataPtr = image->image();
            if (imgDataPtr) {
                parseImage(imgDataPtr, &cvImage, color_isp_handle);
            } else {
                std::cerr << "[ImageProcesser] 错误: 图像数据指针为空!" << std::endl;
                return -1;
            }
            
            if(cvImage.empty()) {
                std::cerr << "[ImageProcesser] 错误: OpenCV图像为空!" << std::endl;
                return -1;
            }
            
            std::cout << "[ImageProcesser] OpenCV图像解析完成，类型: " << cvImage.type() << std::endl;
            
            switch(cvImage.type())
            {
            case CV_8U:
                //MONO8
                image_size = cvImage.size().area();
                image_fmt = TY_PIXEL_FORMAT_MONO;
                std::cout << "[ImageProcesser] 图像格式转换为MONO8" << std::endl;
                break;
            case CV_16U:
                //MONO16
                image_size = cvImage.size().area() * 2;
                image_fmt = TY_PIXEL_FORMAT_MONO16;
                std::cout << "[ImageProcesser] 图像格式转换为MONO16" << std::endl;
                break;
            case  CV_16UC3:
                //BGR48
                image_size = cvImage.size().area() * 6;
                image_fmt = TY_PIXEL_FORMAT_BGR48;
                std::cout << "[ImageProcesser] 图像格式转换为BGR48" << std::endl;
                break;
            default:
                //BGR888
                image_size = cvImage.cols() * cvImage.rows() * 3;
                image_fmt = TY_PIXEL_FORMAT_BGR;
                std::cout << "[ImageProcesser] 图像格式转换为BGR888(默认)" << std::endl;
                break;
            }
            
            std::cout << "[ImageProcesser] 准备创建新图像对象..." << std::endl;
            _image = std::shared_ptr<TYImage>(new TYImage(cvImage.cols(), cvImage.rows(), comp_id, image_fmt, image_size));
            
            if(_image && _image->buffer()) {
                std::cout << "[ImageProcesser] 复制OpenCV图像数据到TYImage..." << std::endl;
                memcpy(_image->buffer(), cvImage.data(), image_size);
                std::cout << "[ImageProcesser] 其他格式图像处理完成，_image已设置" << std::endl;
                return 0;
            } else {
                std::cerr << "[ImageProcesser] 错误: 创建TYImage失败!" << std::endl;
                return -1;
            }
    #else
            
            // 即使没有OpenCV依赖，也尝试直接使用原始图像数据
            std::cout << "[ImageProcesser] 没有OpenCV依赖，尝试直接使用原始图像数据..." << std::endl;
            _image = std::shared_ptr<TYImage>(new TYImage(*image));
            if(_image) {
                std::cout << "[ImageProcesser] 成功使用原始图像数据创建TYImage，宽度=" << _image->width() 
                          << ", 高度=" << _image->height() 
                          << ", 格式=" << _image->pixelFormat() << std::endl;
                return 0;
            } else {
                std::cerr << "[ImageProcesser] 错误: 直接使用原始图像数据创建TYImage失败!" << std::endl;
                return -1;
            }
    #endif
        }
    }
}


int ImageProcesser::DepthImageRender()
{
    if(!_image) return -1;
    TY_PIXEL_FORMAT format = _image->pixelFormat();
    if(format != TY_PIXEL_FORMAT_DEPTH16) return -1;

#ifdef OPENCV_DEPENDENCIES
    static DepthRender render;
    funny_Mat depth(_image->height(), _image->width(), CV_16U, _image->buffer());
    funny_Mat bgr = render.Compute(depth);

    _image = std::shared_ptr<TYImage>(new TYImage(_image->width(), _image->height(), _image->componentID(), TY_PIXEL_FORMAT_BGR,  bgr.cols() * bgr.rows() * 3));
    memcpy(_image->buffer(), bgr.data(), _image->size());
    return 0;
#else
    return -1;
#endif
}

TY_STATUS ImageProcesser::doUndistortion()
{
    std::cout << "[ImageProcesser] 开始执行图像畸变校正..." << std::endl;
    
    // 检查图像是否有效
    if(!_image) {
        std::cerr << "[ImageProcesser] 错误: 图像数据为空!" << std::endl;
        return TY_STATUS_ERROR;
    }
    
    // 检查校准数据是否存在
    std::cout << "[ImageProcesser] 检查校准数据是否存在..." << std::endl;
    if(!_calib_data) {
        std::cerr << "[ImageProcesser] 错误: 校准数据为空!" << std::endl;
        return TY_STATUS_ERROR;
    }
    std::cout << "[ImageProcesser] 校准数据存在，准备进行畸变校正" << std::endl;

    // 获取图像信息
    int32_t         image_size = _image->size();
    TY_PIXEL_FORMAT image_fmt = _image->pixelFormat();
    TY_COMPONENT_ID comp_id = _image->componentID();
    int32_t width = _image->width();
    int32_t height = _image->height();
    
    std::cout << "[ImageProcesser] 图像信息: 宽度=" << width << ", 高度=" << height 
              << ", 格式=" << image_fmt << ", 大小=" << image_size / 1024 << "KB" << std::endl;

    // 检查图像缓冲区是否有效
    if(!_image->buffer()) {
        std::cerr << "[ImageProcesser] 错误: 图像缓冲区为空!" << std::endl;
        return TY_STATUS_ERROR;
    }
    std::cout << "[ImageProcesser] 图像缓冲区有效" << std::endl;

    // 如果图像格式是YUYV，需要先转换为BGR格式再进行畸变校正
    std::vector<uint8_t> converted_image;
    TY_PIXEL_FORMAT corrected_fmt = image_fmt;
    int32_t corrected_size = image_size;

    if(image_fmt == TY_PIXEL_FORMAT_YUYV) {
        std::cout << "[ImageProcesser] 检测到YUYV格式图像，需要转换为BGR格式才能进行畸变校正..." << std::endl;
        
        // YUYV是2字节/像素，BGR是3字节/像素
        corrected_size = width * height * 3;
        corrected_fmt = TY_PIXEL_FORMAT_BGR;
        converted_image.resize(corrected_size);
        
        // 进行YUYV到BGR的转换 - 使用参考文件中的实现方式
        const uint8_t* yuyv_data = static_cast<const uint8_t*>(_image->buffer());
        uint8_t* bgr_data = converted_image.data();
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x += 2) {
                // 计算数据索引（每4个字节包含2个像素）
                int idx = y * width * 2 + x * 2;
                
                // 提取YUYV分量
                uint8_t y0 = yuyv_data[idx];     // 第一个像素的Y
                uint8_t u  = yuyv_data[idx + 1]; // U分量
                uint8_t y1 = yuyv_data[idx + 2]; // 第二个像素的Y
                uint8_t v  = yuyv_data[idx + 3]; // V分量
                
                // 使用参考文件中的YUV到RGB转换公式
                // 转换第一个像素 (Y0, U, V)
                int c = y0 - 16;
                int d = u - 128;
                int e = v - 128;
                
                int r0 = (298 * c + 409 * e + 128) >> 8;
                int g0 = (298 * c - 100 * d - 208 * e + 128) >> 8;
                int b0 = (298 * c + 516 * d + 128) >> 8;
                
                // 确保值在0-255范围内
                r0 = std::min(255, std::max(0, r0));
                g0 = std::min(255, std::max(0, g0));
                b0 = std::min(255, std::max(0, b0));
                
                // 转换第二个像素 (Y1, U, V)
                c = y1 - 16;
                
                int r1 = (298 * c + 409 * e + 128) >> 8;
                int g1 = (298 * c - 100 * d - 208 * e + 128) >> 8;
                int b1 = (298 * c + 516 * d + 128) >> 8;
                
                // 确保值在0-255范围内
                r1 = std::min(255, std::max(0, r1));
                g1 = std::min(255, std::max(0, g1));
                b1 = std::min(255, std::max(0, b1));
                
                // 设置BGR像素值（注意OpenCV使用BGR顺序）
                int bgr_idx = y * width * 3 + x * 3;
                bgr_data[bgr_idx] = static_cast<uint8_t>(b0);     // B
                bgr_data[bgr_idx + 1] = static_cast<uint8_t>(g0); // G
                bgr_data[bgr_idx + 2] = static_cast<uint8_t>(r0); // R
                
                // 处理第二个像素
                if (x + 1 < width) {
                    int bgr_idx2 = bgr_idx + 3;
                    bgr_data[bgr_idx2] = static_cast<uint8_t>(b1);     // B
                    bgr_data[bgr_idx2 + 1] = static_cast<uint8_t>(g1); // G
                    bgr_data[bgr_idx2 + 2] = static_cast<uint8_t>(r1); // R
                }
            }
        }
        
        std::cout << "[ImageProcesser] YUYV到BGR转换完成，转换后大小=" << corrected_size / 1024 << "KB" << std::endl;
        
        // 保存转换前后的图片用于人工检查
#ifdef OPENCV_DEPENDENCIES
        // 保存原始YUYV图像
        std::cout << "[ImageProcesser] 保存原始YUYV图像用于人工检查..." << std::endl;
        // 先将YUYV转换为OpenCV可处理的格式
        cv::Mat yuyv_mat(height, width, CV_8UC2, const_cast<uint8_t*>(yuyv_data));
        cv::Mat rgb_original;
        cv::cvtColor(yuyv_mat, rgb_original, cv::COLOR_YUV2BGR_YUYV);
        std::string original_save_path = "original_yuyv_image_" + std::to_string(time(nullptr)) + ".png";
        cv::imwrite(original_save_path, rgb_original);
        std::cout << "[ImageProcesser] 原始YUYV图像已保存至：" << original_save_path << std::endl;
        
        // 保存转换后的BGR图像
        std::cout << "[ImageProcesser] 保存转换后的BGR图像用于人工检查..." << std::endl;
        cv::Mat bgr_mat(height, width, CV_8UC3, bgr_data);
        std::string converted_save_path = "converted_bgr_image_" + std::to_string(time(nullptr)) + ".png";
        cv::imwrite(converted_save_path, bgr_mat);
        std::cout << "[ImageProcesser] 转换后的BGR图像已保存至：" << converted_save_path << std::endl;
#else
        std::cout << "[ImageProcesser] 未启用OpenCV依赖，无法保存图像用于人工检查" << std::endl;
#endif
    }

    // 准备畸变校正后的数据缓冲区
    std::cout << "[ImageProcesser] 准备畸变校正缓冲区..." << std::endl;
    std::vector<uint8_t> undistort_image(corrected_size);
    if(undistort_image.empty()) {
        std::cerr << "[ImageProcesser] 错误: 创建畸变校正缓冲区失败!" << std::endl;
        return TY_STATUS_ERROR;
    }
    std::cout << "[ImageProcesser] 畸变校正缓冲区创建成功，大小=" << undistort_image.size() / 1024 << "KB" << std::endl;

    // 设置源图像参数
    TY_IMAGE_DATA src;
    src.width = width;
    src.height = height;
    src.size = corrected_size;
    src.pixelFormat = corrected_fmt;
    src.buffer = (image_fmt == TY_PIXEL_FORMAT_YUYV) ? converted_image.data() : _image->buffer();
    
    // 设置目标图像参数
    TY_IMAGE_DATA dst;
    dst.width = width;
    dst.height = height;
    dst.size = image_size;
    dst.pixelFormat = image_fmt;
    dst.buffer = undistort_image.data();
    
    std::cout << "[ImageProcesser] 准备调用TYUndistortImage执行畸变校正..." << std::endl;
    std::cout << "[ImageProcesser] 源图像地址: " << src.buffer << ", 目标图像地址: " << dst.buffer << std::endl;
    std::cout << "[ImageProcesser] 校准数据地址: " << &*_calib_data << std::endl;

    // 执行畸变校正
    TY_STATUS status = TYUndistortImage(&*_calib_data, &src, NULL, &dst);
    
    std::cout << "[ImageProcesser] TYUndistortImage调用完成，状态码=" << status << std::endl;
    if(status != TY_STATUS_OK) {
        std::cerr << "[ImageProcesser] 错误: 畸变校正失败，状态码=" << status << std::endl;
        return status;
    }
    
    std::cout << "[ImageProcesser] 畸变校正成功，准备更新图像数据..." << std::endl;

    // 更新图像数据
    std::cout << "[ImageProcesser] 创建新图像对象..." << std::endl;
    std::shared_ptr<TYImage> new_image = std::shared_ptr<TYImage>(new TYImage(width, height, comp_id, image_fmt, image_size));
    if(!new_image || !new_image->buffer()) {
        std::cerr << "[ImageProcesser] 错误: 创建新图像对象失败!" << std::endl;
        return TY_STATUS_ERROR;
    }
    
    std::cout << "[ImageProcesser] 复制畸变校正后的数据到新图像..." << std::endl;
    memcpy(new_image->buffer(), undistort_image.data(), image_size);
    
    std::cout << "[ImageProcesser] 更新图像指针..." << std::endl;
    _image = new_image;
    
    std::cout << "[ImageProcesser] 畸变校正完成，返回成功状态" << std::endl;
    return TY_STATUS_OK;
}

int ImageProcesser::show()
{
    if(!_image) return -1;
#ifdef OPENCV_DEPENDENCIES
    cv::Mat display;
    switch(_image->pixelFormat())
    {
        case TY_PIXEL_FORMAT_MONO:
        {
            display = cv::Mat(_image->height(), _image->width(), CV_8U, _image->buffer());
            break;
        }
        case TY_PIXEL_FORMAT_MONO16:
        {
            display = cv::Mat(_image->height(), _image->width(), CV_16U, _image->buffer());
            break;
        }
        case TY_PIXEL_FORMAT_BGR:
        {
            display = cv::Mat(_image->height(), _image->width(), CV_8UC3, _image->buffer());
            break;
        }
        case TY_PIXEL_FORMAT_BGR48:
        {
            display = cv::Mat(_image->height(), _image->width(), CV_16UC3, _image->buffer());
            break;
        }
        case TY_PIXEL_FORMAT_DEPTH16:
        {
            DepthImageRender();
            display = cv::Mat(_image->height(), _image->width(), CV_8UC3, _image->buffer());
            break;
        }
        default:
        {
            break;
        }
    }

    if(!display.empty()) {
        hasWin = true;
        cv::imshow(win_name.c_str(), display);
        int key = cv::waitKey(1);
        return key;
    }
    else
        std::cout << "Unknown image encoding format." << std::endl;
 #endif
    return 0;
}

void ImageProcesser::clear()
{
    // 重置_image指针
    _image.reset();
    
#ifdef OPENCV_DEPENDENCIES
    if (hasWin) {
        cv::destroyWindow(win_name.c_str());   
    }
#endif
}

TYFrame::TYFrame(const TY_FRAME_DATA& frame)
{
    bufferSize = frame.bufferSize;
    userBuffer.resize(bufferSize);
    memcpy(userBuffer.data(), frame.userBuffer, bufferSize);

#define TY_IMAGE_MOVE(src, dst, from, to) do { \
    (to) = (from); \
    (to.buffer) = reinterpret_cast<void*>((std::intptr_t(dst)) + (std::intptr_t(from.buffer) - std::intptr_t(src)));\
}while(0)

    for (int i = 0; i < frame.validCount; i++) {
        TY_IMAGE_DATA img;
        if (frame.image[i].status != TY_STATUS_OK) continue;
    
        // get depth image
        if (frame.image[i].componentID == TY_COMPONENT_DEPTH_CAM) {  
            TY_IMAGE_MOVE(frame.userBuffer, userBuffer.data(), frame.image[i], img);
            _images[TY_COMPONENT_DEPTH_CAM] = std::shared_ptr<TYImage>(new TYImage(img));
        }
        // get left ir image
        if (frame.image[i].componentID == TY_COMPONENT_IR_CAM_LEFT) {
            TY_IMAGE_MOVE(frame.userBuffer, userBuffer.data(), frame.image[i], img);
            _images[TY_COMPONENT_IR_CAM_LEFT] = std::shared_ptr<TYImage>(new TYImage(img));
        }
        // get right ir image
        if (frame.image[i].componentID == TY_COMPONENT_IR_CAM_RIGHT) {
            TY_IMAGE_MOVE(frame.userBuffer, userBuffer.data(), frame.image[i], img);
            _images[TY_COMPONENT_IR_CAM_RIGHT] = std::shared_ptr<TYImage>(new TYImage(img));
        }
        // get color image
        if (frame.image[i].componentID == TY_COMPONENT_RGB_CAM) {
            TY_IMAGE_MOVE(frame.userBuffer, userBuffer.data(), frame.image[i], img);
            _images[TY_COMPONENT_RGB_CAM] = std::shared_ptr<TYImage>(new TYImage(img));
        }
    }
}

TYFrame::~TYFrame()
{

}


TYFrameParser::TYFrameParser(uint32_t max_queue_size, const TY_ISP_HANDLE isp_handle)
{
    _max_queue_size = max_queue_size;
    isRuning = true;

    setImageProcesser(TY_COMPONENT_DEPTH_CAM, std::shared_ptr<ImageProcesser>(new ImageProcesser("depth")));
    setImageProcesser(TY_COMPONENT_IR_CAM_LEFT, std::shared_ptr<ImageProcesser>(new ImageProcesser("Left-IR")));
    setImageProcesser(TY_COMPONENT_IR_CAM_RIGHT, std::shared_ptr<ImageProcesser>(new ImageProcesser("Right-IR")));
    setImageProcesser(TY_COMPONENT_RGB_CAM, std::shared_ptr<ImageProcesser>(new ImageProcesser("color", nullptr, isp_handle)));

    processThread_ = std::thread(&TYFrameParser::display, this);
}

TYFrameParser::~TYFrameParser()
{
    isRuning = false;
    processThread_.join();
}

int TYFrameParser::setImageProcesser(TY_COMPONENT_ID id, std::shared_ptr<ImageProcesser> proc)
{
    stream[id] = proc;
    return 0;
}

int TYFrameParser::doProcess(const std::shared_ptr<TYFrame>& img)
{           
    auto depth = img->depthImage();
    auto color = img->colorImage();
    auto left_ir = img->leftIRImage();
    auto right_ir = img->rightIRImage();

    if (left_ir) {
        stream[TY_COMPONENT_IR_CAM_LEFT]->parse(left_ir);
    }

    if (right_ir) {
        stream[TY_COMPONENT_IR_CAM_RIGHT]->parse(right_ir);
    }

    if (color) {
        stream[TY_COMPONENT_RGB_CAM]->parse(color);
    }

    if (depth) {
        stream[TY_COMPONENT_DEPTH_CAM]->parse(depth);
    }
    return 0;
}

void TYFrameParser::display()
{
    int ret = 0;
    while(isRuning) {
        if(images.size()) {
            std::unique_lock<std::mutex> lock(_queue_lock);
            std::shared_ptr<TYFrame> img = images.front();

            if(img) {
                images.pop();
                doProcess(img);
            }
        }

        for(auto& iter : stream) {
            ret = iter.second->show();
            // 跳过键盘事件回调，避免编译错误
                if(ret > 0) {
                    // 键盘事件回调被暂时禁用
                }
        }
    }
}

inline void TYFrameParser::ImageQueueSizeCheck()
{
    while(images.size() >= _max_queue_size)
        images.pop();
}

void TYFrameParser::update(const std::shared_ptr<TYFrame>& frame)
{
    std::unique_lock<std::mutex> lock(_queue_lock);
    if(frame) {
        ImageQueueSizeCheck();
        images.push(frame);
#ifndef OPENCV_DEPENDENCIES        
        auto depth = frame->depthImage();
        auto color = frame->colorImage();
        auto left_ir = frame->leftIRImage();
        auto right_ir = frame->rightIRImage();

        if (left_ir) {
            auto image = left_ir;
            std::cout << "Left" << " image size : " << image->width() << " x " << image->height() << std::endl;
        }

        if (right_ir) {
            auto image = right_ir;
            std::cout << "Right" << " image size : " << image->width() << " x " << image->height() << std::endl;
        }

        if (color) {
            auto image = color;
            std::cout << "Color" << " image size : " << image->width() << " x " << image->height() << std::endl;
        }

        if (depth) {
            auto image = depth;
            std::cout << "Depth" << " image size : " << image->width() << " x " << image->height() << std::endl;
        }

#endif
    }
}
}//namespace percipio_layer
