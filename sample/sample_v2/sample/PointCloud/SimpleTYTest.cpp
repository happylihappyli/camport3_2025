// This file contains simpleTYTest function which is called by main.cpp
// It also contains a main function for standalone execution

#include <iostream>

/**
 * simpleTYTest - TY库基本功能测试的函数
 * 此函数会被main.cpp调用，也可以作为独立程序运行
 */
void simpleTYTest() {
    std::cout << "=== Simple TY Test ===" << std::endl;
    std::cout << "Testing basic TY library functionality" << std::endl;
    std::cout << "This is a simple TY library test example." << std::endl;
}

/**
 * main - 当此文件作为独立程序运行时的入口点
 */
#ifdef STANDALONE_SIMPLE_TY_TEST
int main() {
    simpleTYTest();
    return 0;
}
#endif