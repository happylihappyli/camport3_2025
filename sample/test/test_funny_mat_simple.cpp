// 简单的测试文件，用于验证funny_Mat类和TYInitImageData函数

// 只包含最基本的标准库
#include <cstdint>
#include <cstring>
#include <iostream>

// 像素格式定义
enum {
    CV_8UC1 = 0
};

class funny_Mat {
public:
    // 构造函数
    funny_Mat() : rows_(0), cols_(0), data_(nullptr) {}
    
    funny_Mat(int rows, int cols, int type) 
        : rows_(rows), cols_(cols) {
        data_ = new uint8_t[static_cast<size_t>(rows) * cols];
        memset(data_, 0, static_cast<size_t>(rows) * cols);
    }
    
    // 析构函数
    ~funny_Mat() {
        if (data_) {
            delete[] data_;
        }
    }
    
    // 获取行数和列数
    int rows() const { return rows_; }
    int cols() const { return cols_; }
    
    // 获取数据指针
    uint8_t* data() { return data_; }
    
    // 克隆矩阵
    funny_Mat clone() const {
        funny_Mat result(rows_, cols_, 0);
        if (data_) {
            memcpy(result.data_, data_, static_cast<size_t>(rows_) * cols_);
        }
        return result;
    }
    
private:
    int rows_;         // 行数
    int cols_;         // 列数
    uint8_t* data_;    // 数据指针
};

// 假设的TYApi.h中的相关定义
typedef struct {
    size_t size;
    void* buffer;
    size_t width;
    size_t height;
} TY_IMAGE_DATA;

inline TY_IMAGE_DATA TYInitImageData(size_t size, void* buffer, size_t width, size_t height) {
    TY_IMAGE_DATA data;
    data.size = size;
    data.buffer = buffer;
    data.width = width;
    data.height = height;
    return data;
}

int main() {
    // 测试funny_Mat类的基本功能
    funny_Mat mat(10, 10, CV_8UC1);
    std::cout << "Matrix created: " << mat.rows() << "x" << mat.cols() << std::endl;
    
    // 测试clone方法
    funny_Mat cloned = mat.clone();
    std::cout << "Matrix cloned: " << cloned.rows() << "x" << cloned.cols() << std::endl;
    
    // 测试data方法
    if (mat.data()) {
        std::cout << "Data pointer is not null" << std::endl;
        
        // 测试TYInitImageData函数
        size_t size = 10 * 10 * 1;
        void* buffer = mat.data();
        size_t width = 10;
        size_t height = 10;
        TY_IMAGE_DATA image_data = TYInitImageData(size, buffer, width, height);
        std::cout << "TYInitImageData called successfully" << std::endl;
    }
    
    return 0;
}