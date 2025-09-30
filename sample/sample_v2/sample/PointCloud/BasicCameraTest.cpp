// This file contains basicCameraTest function which is called by main.cpp
// It also contains a main function for standalone execution

#include <iostream>

/**
 * basicCameraTest - 基础相机功能测试的函数
 * 此函数会被main.cpp调用，也可以作为独立程序运行
 */
void basicCameraTest() {
    std::cout << "=== Basic Camera Test ===" << std::endl;
    std::cout << "Testing fundamental camera operations" << std::endl;
    std::cout << "This is a basic camera test example." << std::endl;
}

/**
 * main - 当此文件作为独立程序运行时的入口点
 */
#ifdef STANDALONE_BASIC_CAMERA_TEST
int main() {
    basicCameraTest();
    return 0;
}
#endif