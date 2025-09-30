// 最简单的测试文件，所有代码都在一个文件中

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
    
    funny_Mat(int rows, int cols) 
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
        funny_Mat result(rows_, cols_);
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

int main() {
    // 测试funny_Mat类的基本功能
    funny_Mat mat(10, 10);
    std::cout << "Matrix created: " << mat.rows() << "x" << mat.cols() << std::endl;
    
    // 测试clone方法
    funny_Mat cloned = mat.clone();
    std::cout << "Matrix cloned: " << cloned.rows() << "x" << cloned.cols() << std::endl;
    
    return 0;
}