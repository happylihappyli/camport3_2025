#include "funny_Mat.hpp"
#include <iostream>

int main() {
    // 创建一个简单的矩阵
    funny_Mat mat(10, 10, CV_8UC1);
    
    // 测试基本功能
    std::cout << "Matrix created: " << mat.rows() << "x" << mat.cols() << std::endl;
    
    // 测试数据访问
    if (mat.data()) {
        std::cout << "Data pointer is valid" << std::endl;
    }
    
    // 测试克隆功能
    funny_Mat cloned = mat.clone();
    std::cout << "Matrix cloned: " << cloned.rows() << "x" << cloned.cols() << std::endl;
    
    return 0;
}