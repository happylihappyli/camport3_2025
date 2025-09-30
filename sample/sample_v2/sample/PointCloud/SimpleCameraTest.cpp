// This file contains simpleCameraTest function which is called by main.cpp
// It also contains a main function for standalone execution

#include <iostream>

/**
 * simpleCameraTest - 相机基础功能测试的函数
 * 此函数会被main.cpp调用，也可以作为独立程序运行
 */
void simpleCameraTest() {
    std::cout << "=== Simple Camera Test ===" << std::endl;
    std::cout << "Testing basic camera functionality" << std::endl;
    std::cout << "This is a simple camera test example." << std::endl;
}

/**
 * main - 当此文件作为独立程序运行时的入口点
 */
#ifdef STANDALONE_SIMPLE_CAMERA_TEST
int main() {
    simpleCameraTest();
    return 0;
}
#endif