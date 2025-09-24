#include <thread>

#include "Frame.hpp"
#include "TYImageProc.h"
#include <time.h>

namespace percipio_layer {


TYImage::TYImage()
{
    memset(&image_data, 0, sizeof(image_data));
}

TYImage::TYImage(const TY_IMAGE_DATA& image) :
    m_isOwner(false)
{
    memcpy(&image_data, &image, sizeof(TY_IMAGE_DATA));
}

TYImage::TYImage(const TYImage& src)
{
    image_data.timestamp = src.timestamp();
    image_data.imageIndex = src.imageIndex();
    image_data.status = src.status();
    image_data.componentID = src.componentID();
    image_data.size = src.size();
    image_data.width = src.width();
    image_data.height = src.height();
    image_data.pixelFormat = src.pixelFormat();
    if(image_data.size) {
        m_isOwner = true;
        image_data.buffer = malloc(image_data.size);
        memcpy(image_data.buffer, src.buffer(), image_data.size);
    }
}

TYImage::TYImage(int32_t width, int32_t height, TY_COMPONENT_ID compID, TY_PIXEL_FORMAT format, int32_t size)
{
    image_data.size = size;
    image_data.width = width;
    image_data.height = height;
    image_data.componentID = compID;
    image_data.pixelFormat = format;
     if(image_data.size) {
        m_isOwner = true;
        image_data.buffer = calloc(image_data.size, 1);
    }
}

bool TYImage::resize(int w, int h)
{
#ifdef OPENCV_DEPENDENCIES
    cv::Mat src, dst;
    switch(image_data.pixelFormat)
    {
        case TY_PIXEL_FORMAT_BGR:
        case TY_PIXEL_FORMAT_RGB:
            src = cv::Mat(cv::Size(width(), height()), CV_8UC3, buffer());
            break;
        case TY_PIXEL_FORMAT_MONO:
            src = cv::Mat(cv::Size(width(), height()), CV_8U, buffer());
            break;
        case TY_PIXEL_FORMAT_MONO16:
            src = cv::Mat(cv::Size(width(), height()), CV_16U, buffer());
            break;
        case TY_PIXEL_FORMAT_BGR48:
            src = cv::Mat(cv::Size(width(), height()), CV_16UC3, buffer());
            break;
        case TY_PIXEL_FORMAT_RGB48:
            src = cv::Mat(cv::Size(width(), height()), CV_16UC3, buffer());
            break;
        case TY_PIXEL_FORMAT_DEPTH16:
            src = cv::Mat(cv::Size(width(), height()), CV_16U, buffer());
            break;
        default:
            return false;
    }

    if(image_data.pixelFormat == TY_PIXEL_FORMAT_DEPTH16)
        cv::resize(src, dst, cv::Size(w, h), 0, 0, cv::INTER_NEAREST);
    else
        cv::resize(src, dst, cv::Size(w, h));
    image_data.size = dst.cols * dst.rows * dst.elemSize() * dst.channels();
    image_data.width = dst.cols;
    image_data.height = dst.rows;
    if(m_isOwner) free(image_data.buffer);
    image_data.buffer = malloc(image_data.size);
    memcpy(image_data.buffer, dst.data, image_data.size);
    return true;
#else
    std::cout << "not support!" << std::endl;
    return false;
#endif
}

TYImage::~TYImage()
{
    if(m_isOwner) {
        free(image_data.buffer);
    }
}

ImageProcesser::ImageProcesser(const char* win, const TY_CAMERA_CALIB_INFO* calib_data, const TY_ISP_HANDLE isp_handle) 
{
    win_name = win;
    hasWin = false;
    color_isp_handle = isp_handle;
    if(calib_data != nullptr) {
        _calib_data = std::shared_ptr<TY_CAMERA_CALIB_INFO>(new TY_CAMERA_CALIB_INFO(*calib_data));
    } 
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
            cv::Mat cvImage;
            int32_t         image_size;
            TY_PIXEL_FORMAT image_fmt;
            TY_COMPONENT_ID comp_id;
            comp_id = image->componentID();
            
            std::cout << "[ImageProcesser] 调用parseImage函数解析图像..." << std::endl;
            parseImage(image->image(),  &cvImage, color_isp_handle);
            
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
                image_size = cvImage.size().area() * 3;
                image_fmt = TY_PIXEL_FORMAT_BGR;
                std::cout << "[ImageProcesser] 图像格式转换为BGR888(默认)" << std::endl;
                break;
            }
            
            std::cout << "[ImageProcesser] 准备创建新图像对象..." << std::endl;
            _image = std::shared_ptr<TYImage>(new TYImage(cvImage.cols, cvImage.rows, comp_id, image_fmt, image_size));
            
            if(_image && _image->buffer()) {
                std::cout << "[ImageProcesser] 复制OpenCV图像数据到TYImage..." << std::endl;
                memcpy(_image->buffer(), cvImage.data, image_size);
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
    cv::Mat depth = cv::Mat(_image->height(), _image->width(), CV_16U, _image->buffer());
    cv::Mat bgr = render.Compute(depth);

    _image = std::shared_ptr<TYImage>(new TYImage(_image->width(), _image->height(), _image->componentID(), TY_PIXEL_FORMAT_BGR,  bgr.size().area() * 3));
    memcpy(_image->buffer(), bgr.data, _image->size());
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
            if(ret > 0) {
                if(func_keyboard_event) func_keyboard_event(ret, user_data);
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
