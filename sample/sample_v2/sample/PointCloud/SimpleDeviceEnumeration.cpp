// This file contains simpleDeviceEnumeration function which is called by main.cpp
// It also contains a main function for standalone execution

#include <iostream>

/**
 * simpleDeviceEnumeration - 设备枚举功能测试的函数
 * 此函数会被main.cpp调用，也可以作为独立程序运行
 */
void simpleDeviceEnumeration() {
    std::cout << "=== Simple Device Enumeration Test ===" << std::endl;
    std::cout << "Enumerating connected devices" << std::endl;
    std::cout << "This is a simple device enumeration example." << std::endl;
}

/**
 * main - 当此文件作为独立程序运行时的入口点
 */
#ifdef STANDALONE_SIMPLE_DEVICE_ENUMERATION
int main() {
    simpleDeviceEnumeration();
    return 0;
}
#endif