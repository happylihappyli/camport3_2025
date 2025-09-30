// This file contains correctInterfaceTest function which is called by main.cpp
// It also contains a main function for standalone execution

#include <iostream>

/**
 * correctInterfaceTest - 接口功能测试的函数
 * 此函数会被main.cpp调用，也可以作为独立程序运行
 */
void correctInterfaceTest() {
    std::cout << "=== Correct Interface Test ===" << std::endl;
    std::cout << "Testing interface functionality" << std::endl;
    std::cout << "This is a correct interface test example." << std::endl;
}

/**
 * main - 当此文件作为独立程序运行时的入口点
 */
#ifdef STANDALONE_CORRECT_INTERFACE_TEST
int main() {
    correctInterfaceTest();
    return 0;
}
#endif