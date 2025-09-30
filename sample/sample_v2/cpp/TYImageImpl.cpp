#include <cstring>
#include <memory>
#include "Frame.hpp"

namespace percipio_layer {

// TYImage 构造函数实现
TYImage::TYImage() {
    memset(&image_data, 0, sizeof(TY_IMAGE_DATA));
    m_isOwner = false;
}

TYImage::TYImage(const TY_IMAGE_DATA& image) {
    memcpy(&image_data, &image, sizeof(TY_IMAGE_DATA));
    m_isOwner = false;
}

TYImage::TYImage(const TYImage& src) {
    memcpy(&image_data, &src.image_data, sizeof(TY_IMAGE_DATA));
    m_isOwner = false;
}

TYImage::TYImage(int32_t width, int32_t height, TY_COMPONENT_ID compID, TY_PIXEL_FORMAT format, int32_t size) {
    memset(&image_data, 0, sizeof(TY_IMAGE_DATA));
    image_data.width = width;
    image_data.height = height;
    image_data.componentID = compID;
    image_data.pixelFormat = format;
    image_data.size = size;
    image_data.buffer = (size > 0) ? malloc(size) : NULL;
    m_isOwner = (size > 0);
}

// TYImage 析构函数实现
TYImage::~TYImage() {
    if (m_isOwner && image_data.buffer) {
        free(image_data.buffer);
        image_data.buffer = NULL;
    }
}

// ImageProcesser 构造函数实现
ImageProcesser::ImageProcesser(const char* win, const TY_CAMERA_CALIB_INFO* calib_data, const TY_ISP_HANDLE isp_handle) {
    win_name = (win) ? win : "";
    hasWin = (win != NULL);
    color_isp_handle = isp_handle;
    if (calib_data) {
        _calib_data.reset(new TY_CAMERA_CALIB_INFO);
        memcpy(&*_calib_data, calib_data, sizeof(TY_CAMERA_CALIB_INFO));
    } else {
        _calib_data.reset();
    }
    _image.reset();
}

} // namespace percipio_layer