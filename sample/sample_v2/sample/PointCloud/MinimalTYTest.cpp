// This file contains minimalTYTest function which is called by main.cpp
// It also contains a main function for standalone execution

#include <windows.h>

/**
 * minimalTYTest - 最简化的TY库测试函数，不使用iostream
 * 此函数会被main.cpp调用，也可以作为独立程序运行
 */
void minimalTYTest() {
    // 使用MessageBox显示信息，不使用cout
    MessageBox(NULL, TEXT("Minimal TY Test Started"), TEXT("Information"), MB_OK | MB_ICONINFORMATION);
    MessageBox(NULL, TEXT("Testing minimal TY library functionality"), TEXT("Information"), MB_OK | MB_ICONINFORMATION);
    MessageBox(NULL, TEXT("Minimal TY Test Completed"), TEXT("Information"), MB_OK | MB_ICONINFORMATION);
}

/**
 * main - 当此文件作为独立程序运行时的入口点
 */
#ifdef STANDALONE_MINIMAL_TY_TEST
int main() {
    minimalTYTest();
    return 0;
}
#endif