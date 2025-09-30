// This file contains interfaceTest function which is called by main.cpp
// It also contains a main function for standalone execution

#include <iostream>
#include <windows.h>

/**
 * interfaceTest - 测试设备接口的函数
 * 此函数会被main.cpp调用，也可以作为独立程序运行
 */
void interfaceTest() {
    std::cout << "=== Interface Test ===" << std::endl;
    std::cout << "Testing device interfaces" << std::endl;
    std::cout << "This is a simple interface test." << std::endl;
    // 显示一个消息框
    MessageBox(NULL, TEXT("Interface Test Completed"), TEXT("Information"), MB_OK | MB_ICONINFORMATION);
}

/**
 * main - 当此文件作为独立程序运行时的入口点
 */
#ifdef STANDALONE_INTERFACE_TEST
int main() {
    interfaceTest();
    return 0;
}
#endif