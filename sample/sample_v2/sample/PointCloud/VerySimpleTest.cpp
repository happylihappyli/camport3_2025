// This file contains verySimpleTest function which is called by main.cpp
// It also contains a main function for standalone execution

#include <iostream>

/**
 * verySimpleTest - 最简单的功能测试函数
 * 此函数会被main.cpp调用，也可以作为独立程序运行
 */
void verySimpleTest() {
    std::cout << "=== Very Simple Test ===" << std::endl;
    std::cout << "This is a very simple test example." << std::endl;
    std::cout << "End of test." << std::endl;
}

/**
 * main - 当此文件作为独立程序运行时的入口点
 */
#ifdef STANDALONE_VERY_SIMPLE_TEST
int main() {
    verySimpleTest();
    return 0;
}
#endif