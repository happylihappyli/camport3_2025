// This file contains minimalPointCloud function which is called by main.cpp
// It also contains a main function for standalone execution

#include <iostream>

/**
 * minimalPointCloud - 最小化点云处理的函数
 * 此函数会被main.cpp调用，也可以作为独立程序运行
 */
void minimalPointCloud() {
    std::cout << "=== Minimal PointCloud Test ===" << std::endl;
    std::cout << "Processing point cloud data" << std::endl;
    std::cout << "This is a minimal point cloud example." << std::endl;
}

/**
 * main - 当此文件作为独立程序运行时的入口点
 */
#ifdef STANDALONE_MINIMAL_POINT_CLOUD
int main() {
    minimalPointCloud();
    return 0;
}
#endif