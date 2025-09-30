#include "funny_Mat.hpp"
#include <iostream>

int main() {
    // 测试funny_Mat的基本功能
    funny_Mat mat(10, 10, CV_8UC1);
    
    // 测试rows()和cols()方法
    std::cout << "矩阵尺寸: " << mat.rows() << "x" << mat.cols() << std::endl;
    
    // 测试data()方法
    if (mat.data()) {
        std::cout << "数据指针不为空" << std::endl;
    }
    
    // 测试clone()方法
    funny_Mat cloned = mat.clone();
    std::cout << "克隆矩阵尺寸: " << cloned.rows() << "x" << cloned.cols() << std::endl;
    
    // 测试operator*方法
    funny_Mat scaled = mat * 2.0f;
    std::cout << "缩放后矩阵尺寸: " << scaled.rows() << "x" << scaled.cols() << std::endl;
    
    return 0;
}