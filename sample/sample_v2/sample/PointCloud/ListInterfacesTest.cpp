// This file contains listInterfacesTest function which is called by main.cpp
// It also contains a main function for standalone execution

#include <iostream>

/**
 * listInterfacesTest - 列出设备接口的函数
 * 此函数会被main.cpp调用，也可以作为独立程序运行
 */
void listInterfacesTest() {
    std::cout << "=== List Interfaces Test ===" << std::endl;
    std::cout << "Listing available device interfaces" << std::endl;
    std::cout << "This is a simple interface listing test." << std::endl;
}

/**
 * main - 当此文件作为独立程序运行时的入口点
 */
#ifdef STANDALONE_LIST_INTERFACES_TEST
int main() {
    listInterfacesTest();
    return 0;
}
#endif