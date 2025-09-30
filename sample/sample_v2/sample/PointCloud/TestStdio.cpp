#include <stdio.h>

/**
 * @brief 简单的测试函数，用于验证stdio.h是否能被正确包含
 */
void testStdio() {
    printf("Testing stdio.h functions\n");
    
    // 测试fopen, fprintf, fclose
    FILE* testFile = fopen("test_output.txt", "w");
    if (testFile != NULL) {
        fprintf(testFile, "This is a test\n");
        fclose(testFile);
        printf("Test file created successfully\n");
    }
    
    printf("Test completed\n");
}

#ifdef STANDALONE_TEST_STDIO
int main() {
    testStdio();
    return 0;
}
#endif