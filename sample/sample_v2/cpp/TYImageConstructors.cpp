#include <cstring>
#include "Frame.hpp"
#include <memory>
#include <cstring>

namespace percipio_layer {

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

// TYImage 析构函数实现
TYImage::~TYImage() {
    if (image_data.buffer) {
        free(image_data.buffer);
        image_data.buffer = NULL;
    }
}

// ImageProcesser 构造函数实现
ImageProcesser::ImageProcesser(const char* win, const TY_CAMERA_CALIB_INFO* calib_data, const TY_ISP_HANDLE isp_handle) {
    if (win) {
        win_name = win;
    }
    color_isp_handle = isp_handle;
    if (calib_data) {
        _calib_data.reset(new TY_CAMERA_CALIB_INFO(*calib_data));
    }
}

// ImageProcesser parse方法实现
int ImageProcesser::parse(const std::shared_ptr<TYImage>& image) {
    // 检查输入图像是否有效
    if(!image || !image->buffer()) {
        return -1;
    }
    
    _image = image;
    return 0;
}

// ImageProcesser clear方法实现
void ImageProcesser::clear() {
    _image.reset();
}

} // namespace percipio_layer