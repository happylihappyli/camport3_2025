// This file contains simpleDeviceTest function which is called by main.cpp
// It also contains a main function for standalone execution

#include <iostream>

/**
 * simpleDeviceTest - 设备基本功能测试的函数
 * 此函数会被main.cpp调用，也可以作为独立程序运行
 */
void simpleDeviceTest() {
    std::cout << "=== Simple Device Test ===" << std::endl;
    std::cout << "Testing basic device functionality" << std::endl;
    std::cout << "This is a simple device test example." << std::endl;
}

/**
 * main - 当此文件作为独立程序运行时的入口点
 */
#ifdef STANDALONE_SIMPLE_DEVICE_TEST
int main() {
    simpleDeviceTest();
    return 0;
}
#endif