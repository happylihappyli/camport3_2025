// 这是一个简化的实现文件，只包含必要的方法
#include "Frame.hpp"
#include <memory>
#include <cstring>

namespace percipio_layer {

// 只实现必要的ImageProcesser::parse方法
int ImageProcesser::parse(const std::shared_ptr<TYImage>& image) {
    _image = image;
    return 0;
}

// 只实现必要的ImageProcesser::clear方法
void ImageProcesser::clear() {
    _image.reset();
}

} // namespace percipio_layer