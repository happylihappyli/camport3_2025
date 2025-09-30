// 完全独立的测试文件，所有代码都在一个文件中

// 只包含最基本的标准库
#include <cstdint>
#include <cassert>
#include <cstring>
#include <iostream>

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

// 像素格式定义
enum {
    CV_8UC1 = 0,   // 8位无符号单通道
    CV_8UC2 = 8,   // 8位无符号双通道
    CV_8UC3 = 16,  // 8位无符号三通道
    CV_8UC4 = 24,  // 8位无符号四通道
    CV_16U = 25,   // 16位无符号
    CV_16UC1 = 25  // 16位无符号单通道
};

class funny_Mat {
public:
    // 构造函数
    funny_Mat() : rows_(0), cols_(0), type_(CV_8UC1), data_(nullptr), owns_data_(false), data_size_(0) {}
    
    funny_Mat(int rows, int cols, int type) 
        : rows_(rows), cols_(cols), type_(type), owns_data_(true) {
        int channels = getChannels(type);
        data_size_ = static_cast<size_t>(rows) * static_cast<size_t>(cols) * static_cast<size_t>(channels);
        data_ = new uint8_t[data_size_];
        memset(data_, 0, data_size_);
    }
    
    // 拷贝构造函数
    funny_Mat(const funny_Mat& other) 
        : rows_(other.rows_), cols_(other.cols_), type_(other.type_), data_size_(other.data_size_), owns_data_(true) {
        data_ = new uint8_t[data_size_];
        if (other.data_) {
            memcpy(data_, other.data_, data_size_);
        }
    }
    
    // 析构函数
    ~funny_Mat() {
        if (owns_data_ && data_) {
            delete[] data_;
            data_ = nullptr;
        }
    }
    
    // 赋值操作符
    funny_Mat& operator=(const funny_Mat& other) {
        if (this != &other) {
            if (owns_data_ && data_) {
                delete[] data_;
            }
            
            rows_ = other.rows_;
            cols_ = other.cols_;
            type_ = other.type_;
            data_size_ = other.data_size_;
            owns_data_ = true;
            
            data_ = new uint8_t[data_size_];
            if (other.data_) {
                memcpy(data_, other.data_, data_size_);
            }
        }
        return *this;
    }
    
    // 获取行数
    int rows() const { return rows_; }
    
    // 获取列数
    int cols() const { return cols_; }
    
    // 获取数据指针
    uint8_t* data() { return data_; }
    const uint8_t* data() const { return data_; }
    
    // 克隆矩阵
    funny_Mat clone() const {
        funny_Mat result(rows_, cols_, type_);
        if (data_ && result.data_) {
            memcpy(result.data_, data_, data_size_);
        }
        return result;
    }
    
private:
    int rows_;         // 行数
    int cols_;         // 列数
    int type_;         // 类型
    uint8_t* data_;    // 数据指针
    bool owns_data_;   // 是否拥有数据
    size_t data_size_; // 数据大小
    
    // 辅助函数：从类型中获取通道数
    static int getChannels(int type) {
        switch (type) {
            case CV_8UC1: return 1;
            case CV_8UC2: return 2;
            case CV_8UC3: return 3;
            case CV_8UC4: return 4;
            default: return 1;
        }
    }
};

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
    }
    
    // 测试TYInitImageData函数
    size_t size = 10 * 10 * 1;
    void* buffer = mat.data();
    size_t width = 10;
    size_t height = 10;
    TY_IMAGE_DATA image_data = TYInitImageData(size, buffer, width, height);
    std::cout << "TYInitImageData called successfully" << std::endl;
    
    return 0;
}