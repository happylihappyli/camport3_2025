#pragma once

#include <string>
#include <memory>
#include <vector>
#include <cstdint>
#include <cstring>
#include <map>
#include <queue>
#include <mutex>
#include <thread>

// 包含必要的头文件
#include "TYImageProc.h"
#include "../common/funny_Mat.hpp"

namespace percipio_layer {

class TYImage {
public:
    TY_IMAGE_DATA image_data; // 直接使用TY_IMAGE_DATA结构体
    bool m_isOwner;
    
    TYImage();
    TYImage(const TY_IMAGE_DATA& image);
    TYImage(const TYImage& src);
    TYImage(int32_t width, int32_t height, TY_COMPONENT_ID compID, TY_PIXEL_FORMAT format, int32_t size);
    ~TYImage();
    
    bool resize(int w, int h);
    
    // Getter方法
    int64_t timestamp() const { return image_data.timestamp; }
    int32_t imageIndex() const { return image_data.imageIndex; }
    int32_t status() const { return image_data.status; }
    TY_COMPONENT_ID componentID() const { return image_data.componentID; }
    int32_t size() const { return image_data.size; }
    int32_t width() const { return image_data.width; }
    int32_t height() const { return image_data.height; }
    TY_PIXEL_FORMAT pixelFormat() const { return image_data.pixelFormat; }
    void* buffer() const { return image_data.buffer; }
    const TY_IMAGE_DATA& image() const { return image_data; }
};

class ImageProcesser {
public:
    std::string win_name;
    bool hasWin;
    TY_ISP_HANDLE color_isp_handle;
    std::shared_ptr<TY_CAMERA_CALIB_INFO> _calib_data;
    std::shared_ptr<TYImage> _image; // 添加_image成员变量

    ImageProcesser(const char* win = NULL, const TY_CAMERA_CALIB_INFO* calib_data = NULL, const TY_ISP_HANDLE isp_handle = NULL);
    
    int parse(const std::shared_ptr<TYImage>& image);
    int DepthImageRender();
    TY_STATUS doUndistortion();
    int show();
    void clear();
    
    // 其他方法
    const char* win() const { return win_name.c_str(); }
};

class TYFrame {
public:
    int bufferSize;
    std::vector<uint8_t> userBuffer;
    std::map<TY_COMPONENT_ID, std::shared_ptr<TYImage>> _images;
    
    TYFrame(const TY_FRAME_DATA& frame);
    ~TYFrame();
    
    const std::shared_ptr<TYImage>& depthImage() const {
        auto it = _images.find(TY_COMPONENT_DEPTH_CAM);
        if (it != _images.end()) return it->second;
        static std::shared_ptr<TYImage> empty;
        return empty;
    }
    
    const std::shared_ptr<TYImage>& colorImage() const {
        auto it = _images.find(TY_COMPONENT_RGB_CAM);
        if (it != _images.end()) return it->second;
        static std::shared_ptr<TYImage> empty;
        return empty;
    }
    
    const std::shared_ptr<TYImage>& leftIRImage() const {
        auto it = _images.find(TY_COMPONENT_IR_CAM_LEFT);
        if (it != _images.end()) return it->second;
        static std::shared_ptr<TYImage> empty;
        return empty;
    }
    
    const std::shared_ptr<TYImage>& rightIRImage() const {
        auto it = _images.find(TY_COMPONENT_IR_CAM_RIGHT);
        if (it != _images.end()) return it->second;
        static std::shared_ptr<TYImage> empty;
        return empty;
    }
};

class TYFrameParser {
public:
    uint32_t _max_queue_size;
    bool isRuning;
    std::thread processThread_;
    std::map<TY_COMPONENT_ID, std::shared_ptr<ImageProcesser>> stream;
    std::queue<std::shared_ptr<TYFrame>> images;
    std::mutex _queue_lock;
    
    TYFrameParser(uint32_t max_queue_size = 10, const TY_ISP_HANDLE isp_handle = NULL);
    ~TYFrameParser();
    
    int setImageProcesser(TY_COMPONENT_ID id, std::shared_ptr<ImageProcesser> proc);
    int doProcess(const std::shared_ptr<TYFrame>& img);
    void display();
    void pushFrame(const std::shared_ptr<TYFrame>& frame);
    void ImageQueueSizeCheck();
    void update(const std::shared_ptr<TYFrame>& frame);
};

} // namespace percipio_layer