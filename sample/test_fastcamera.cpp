// 测试FastCamera类的简单程序
#include "device_fix.hpp"
#include <iostream>

int main() {
    std::cout << "测试FastCamera类..." << std::endl;
    
    // 创建FastCamera对象
    percipio_layer::FastCamera camera;
    
    // 测试open方法
    TY_STATUS status = camera.open("test_device");
    std::cout << "open方法返回: " << status << std::endl;
    
    // 测试stream_enable方法
    status = camera.stream_enable(percipio_layer::FastCamera::stream_depth);
    std::cout << "stream_enable方法返回: " << status << std::endl;
    
    // 测试start方法
    status = camera.start();
    std::cout << "start方法返回: " << status << std::endl;
    
    // 测试tryGetFrames方法
    auto frame = camera.tryGetFrames(1000);
    std::cout << "tryGetFrames方法返回: " << (frame ? "非空指针" : "空指针") << std::endl;
    
    std::cout << "测试完成!" << std::endl;
    return 0;
}