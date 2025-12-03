#ifndef SAMPLE_COMMON_FUNNY_MAT_HPP_
#define SAMPLE_COMMON_FUNNY_MAT_HPP_

#include <cstdint>
#include <cassert>
#include <cstring>
#include <vector>
#include <iostream>
#include <algorithm>
#include <thread>
#include <ctime>
#include <memory>

// 像素格式定义 - 移动到全局命名空间
enum PixelFormat {
    CV_8UC1 = 0,
    CV_8UC2 = 8,
    CV_8UC3 = 16,
    CV_8UC4 = 24,
    CV_16U = 25,
    CV_16UC1 = 26,
    CV_16S = 27,
    CV_32S = 28,
    CV_32F = 29,
    CV_64F = 30,
    CV_16SC1 = 31,
    CV_32SC1 = 32,
    CV_32FC1 = 33,
    CV_64FC1 = 34,
    CV_32FC3 = 35
};

// 辅助函数：安全地将PixelFormat转换为int
inline int toInt(PixelFormat pf) {
    return static_cast<int>(pf);
}

// 颜色转换常量 - 移动到全局命名空间
enum ColorConversionCode {
    YUV2BGR_YVYU = 0,
    YUV2BGR_YUYV = 1
};

// 3D点结构体
struct funny_Point3f {
    float x, y, z;
    funny_Point3f() : x(0), y(0), z(0) {}
    funny_Point3f(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
};

// 3D向量结构体（颜色）
struct funny_Vec3b {
    uint8_t val[3];
    funny_Vec3b() { val[0] = val[1] = val[2] = 0; }
    funny_Vec3b(uint8_t r, uint8_t g, uint8_t b) { val[0] = r; val[1] = g; val[2] = b; }
    uint8_t& operator[](int i) { return val[i]; }
    const uint8_t& operator[](int i) const { return val[i]; }
};

class funny_Mat {
public:
    // 构造函数
    funny_Mat() : rows_(0), cols_(0), type_(toInt(PixelFormat::CV_8UC1)), data_(nullptr), owns_data_(false), data_size_(0) {}
    
    funny_Mat(int rows, int cols, int type) 
        : rows_(rows), cols_(cols), type_(type), owns_data_(true) {
        int channels = getChannels(type);
        data_size_ = static_cast<size_t>(rows) * static_cast<size_t>(cols) * static_cast<size_t>(channels);
        data_ = new uint8_t[data_size_];
        memset(data_, 0, data_size_);
    }
    
    funny_Mat(int rows, int cols, int type, void* data) 
        : rows_(rows), cols_(cols), type_(type), data_(reinterpret_cast<uint8_t*>(data)), owns_data_(false) {
        int channels = getChannels(type);
        data_size_ = static_cast<size_t>(rows) * static_cast<size_t>(cols) * static_cast<size_t>(channels);
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
    
    // 获取类型
    int type() const { return type_; }
    
    // 获取数据指针
    uint8_t* data() { return data_; }
    const uint8_t* data() const { return data_; }
    
    // 获取数据大小
    size_t dataSize() const { return data_size_; }
    
    // 辅助函数：从类型中获取通道数
    static int getChannels(int type) {
        if (type == toInt(PixelFormat::CV_8UC1)) return 1;
        else if (type == toInt(PixelFormat::CV_8UC2)) return 2;
        else if (type == toInt(PixelFormat::CV_8UC3)) return 3;
        else if (type == toInt(PixelFormat::CV_8UC4)) return 4;
        else if (type == toInt(PixelFormat::CV_16U)) return 1;
        else if (type == toInt(PixelFormat::CV_16UC1)) return 1;
        else if (type == toInt(PixelFormat::CV_16S)) return 1;
        else if (type == toInt(PixelFormat::CV_16SC1)) return 1;
        else if (type == toInt(PixelFormat::CV_32S)) return 1;
        else if (type == toInt(PixelFormat::CV_32SC1)) return 1;
        else if (type == toInt(PixelFormat::CV_32F)) return 1;
        else if (type == toInt(PixelFormat::CV_32FC1)) return 1;
        else if (type == toInt(PixelFormat::CV_32FC3)) return 3;
        else if (type == toInt(PixelFormat::CV_64F)) return 1;
        else if (type == toInt(PixelFormat::CV_64FC1)) return 1;
        else return 1;
    }
    
    // 矩阵乘法操作符重载
    funny_Mat operator*(float scalar) const {
        funny_Mat result(rows_, cols_, type_);
        
        if (type_ == toInt(PixelFormat::CV_16U) || type_ == toInt(PixelFormat::CV_16UC1)) {
            uint16_t* src_data = reinterpret_cast<uint16_t*>(const_cast<uint8_t*>(data_));
            uint16_t* dst_data = reinterpret_cast<uint16_t*>(result.data_);
            
            size_t pixels = static_cast<size_t>(rows_) * static_cast<size_t>(cols_);
            for (size_t i = 0; i < pixels; ++i) {
                dst_data[i] = static_cast<uint16_t>(static_cast<float>(src_data[i]) * scalar);
            }
        } else if (type_ == toInt(PixelFormat::CV_8UC1)) {
            uint8_t* dst_data = result.data_;
            
            size_t pixels = static_cast<size_t>(rows_) * static_cast<size_t>(cols_);
            for (size_t i = 0; i < pixels; ++i) {
                dst_data[i] = static_cast<uint8_t>(static_cast<float>(data_[i]) * scalar);
            }
        }
        
        return result;
    }
    
    // 克隆矩阵
    funny_Mat clone() const {
        funny_Mat result(rows_, cols_, type_);
        if (data_ && result.data_) {
            memcpy(result.data_, data_, data_size_);
        }
        return result;
    }
    
    // 检查图像是否为空
    bool empty() const {
        return (data_ == nullptr) || (rows_ == 0) || (cols_ == 0);
    }
    
    // 内部类：表示图像大小
    struct Size {
        int rows;  // 行数
        int cols;  // 列数
        
        // 构造函数
        Size(int r, int c) : rows(r), cols(c) {}
        
        // 计算面积（像素数量）
        size_t area() const {
            return static_cast<size_t>(rows) * static_cast<size_t>(cols);
        }
    };
    
    // 获取图像大小
    Size size() const {
        return Size(rows_, cols_);
    }
    
    // 创建图像
    void create(int rows, int cols, int type) {
        // 如果当前对象已经拥有数据，先释放
        if (owns_data_ && data_) {
            delete[] data_;
            data_ = nullptr;
        }
        
        rows_ = rows;
        cols_ = cols;
        type_ = type;
        owns_data_ = true;
        
        int channels = getChannels(type);
        data_size_ = static_cast<size_t>(rows) * static_cast<size_t>(cols) * static_cast<size_t>(channels);
        data_ = new uint8_t[data_size_];
        memset(data_, 0, data_size_);
    }
    
private:
    int rows_;         // 行数
    int cols_;         // 列数
    int type_;         // 类型
    uint8_t* data_;    // 数据指针
    bool owns_data_;   // 是否拥有数据
    size_t data_size_; // 数据大小
};

// 自定义点数据结构
struct funny_Point {
    int x, y;
    
    funny_Point() : x(0), y(0) {}
    funny_Point(int x_, int y_) : x(x_), y(y_) {}
};

// 自定义矩形数据结构
struct funny_Rect {
    int x, y, width, height;
    
    funny_Rect() : x(0), y(0), width(0), height(0) {}
    funny_Rect(int x_, int y_, int w_, int h_) : x(x_), y(y_), width(w_), height(h_) {}
    
    // 检查矩形是否为空
    bool empty() const {
        return (width <= 0) || (height <= 0);
    }
    
    // 计算面积
    size_t area() const {
        return static_cast<size_t>(width) * static_cast<size_t>(height);
    }
};

// 颜色结构体
struct funny_Scalar {
    double val[4];
    
    funny_Scalar() {
        val[0] = val[1] = val[2] = val[3] = 0;
    }
    
    funny_Scalar(double v0, double v1, double v2, double v3 = 0) {
        val[0] = v0; val[1] = v1; val[2] = v2; val[3] = v3;
    }
    
    double& operator[](int i) { return val[i]; }
    const double& operator[](int i) const { return val[i]; }
};

// 颜色常量
namespace funny_colors {
    const funny_Scalar BLACK(0, 0, 0);
    const funny_Scalar WHITE(255, 255, 255);
    const funny_Scalar RED(0, 0, 255);
    const funny_Scalar GREEN(0, 255, 0);
    const funny_Scalar BLUE(255, 0, 0);
}

#endif // SAMPLE_COMMON_FUNNY_MAT_HPP_
