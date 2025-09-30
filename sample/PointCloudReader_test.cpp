#include <iostream>
#include <vector>
#include <string>
#include "TYApi.h"
#include "TYCoordinateMapper.h"
#include "PointCloudReader.cpp"

// 打印设备列表
void listDevices() {
    TY_VERSION_INFO version;
    TYInitLib();
    TYGetLibVersion(&version);
    std::cout << "SDK版本: " << version.major << "." << version.minor << "." << version.patch << std::endl;

    TY_DEVICE_BASE_INFO* pBaseInfos = NULL;
    uint32_t deviceCount = 0;
    TY_STATUS status = TYGetDeviceList(&pBaseInfos, &deviceCount);
    
    if (status != TY_STATUS_OK) {
        std::cerr << "获取设备列表失败，错误码: " << status << std::endl;
        return;
    }

    std::cout << "发现 " << deviceCount << " 个设备: " << std::endl;
    for (uint32_t i = 0; i < deviceCount; i++) {
        std::cout << "设备 " << i + 1 << ":" << std::endl;
        std::cout << "  类型: " << pBaseInfos[i].devType << std::endl;
        std::cout << "  ID: " << pBaseInfos[i].id << std::endl;
        std::cout << "  接口: " << (pBaseInfos[i].ifType == TY_INTERFACE_USB ? "USB" : "其他") << std::endl;
        
        if (pBaseInfos[i].ifType == TY_INTERFACE_USB) {
            std::cout << "  USB端口: " << pBaseInfos[i].usbPath << std::endl;
        } else if (pBaseInfos[i].ifType == TY_INTERFACE_GIGE) {
            std::cout << "  IP: " << pBaseInfos[i].ip << std::endl;
        }
        
        std::cout << "  序列号: " << pBaseInfos[i].serialNumber << std::endl;
        std::cout << "  型号: " << pBaseInfos[i].modelName << std::endl;
    }

    if (pBaseInfos) {
        TYFreeDevicesList(pBaseInfos);
    }
}

// 测试点云读取
int main() {
    std::cout << "=== 图样相机点云读取测试程序 ===" << std::endl;
    
    // 列出所有可用设备
    listDevices();
    
    // 创建点云读取器实例
    PointCloudReader reader;
    
    // 提示用户输入设备ID
    std::string deviceID;
    std::cout << "\n请输入要连接的设备ID（留空使用第一个设备）: ";
    std::getline(std::cin, deviceID);
    
    // 如果用户没有输入设备ID，自动选择第一个设备
    if (deviceID.empty()) {
        TY_DEVICE_BASE_INFO* pBaseInfos = NULL;
        uint32_t deviceCount = 0;
        TY_STATUS status = TYGetDeviceList(&pBaseInfos, &deviceCount);
        
        if (status == TY_STATUS_OK && deviceCount > 0) {
            deviceID = pBaseInfos[0].id;
            std::cout << "自动选择第一个设备，ID: " << deviceID << std::endl;
            TYFreeDevicesList(pBaseInfos);
        } else {
            std::cerr << "没有找到可用设备" << std::endl;
            return -1;
        }
    }
    
    // 打开设备
    if (!reader.open(deviceID)) {
        std::cerr << "无法打开设备，程序退出" << std::endl;
        return -1;
    }
    
    // 开始捕获数据
    if (!reader.startCapture()) {
        std::cerr << "无法开始捕获数据，程序退出" << std::endl;
        return -1;
    }
    
    // 获取点云数据
    std::vector<TY_VECT_3F> pointCloud;
    std::cout << "正在获取点云数据..." << std::endl;
    
    // 重试获取点云，最多5次
    int retryCount = 0;
    const int maxRetries = 5;
    bool success = false;
    
    while (retryCount < maxRetries && !success) {
        if (reader.getPointCloud(pointCloud)) {
            success = true;
            std::cout << "成功获取点云数据，共 " << pointCloud.size() << " 个点" << std::endl;
        } else {
            retryCount++;
            std::cout << "尝试获取点云失败，重试中... (" << retryCount << "/" << maxRetries << ")" << std::endl;
        }
    }
    
    if (!success) {
        std::cerr << "多次尝试后仍无法获取点云数据，程序退出" << std::endl;
        reader.stopCapture();
        return -1;
    }
    
    // 保存点云到PLY文件
    std::string filename;
    std::cout << "请输入保存的PLY文件名（默认为point_cloud.ply）: ";
    std::getline(std::cin, filename);
    
    if (filename.empty()) {
        filename = "point_cloud.ply";
    }
    
    if (!filename.ends_with(".ply")) {
        filename += ".ply";
    }
    
    if (reader.savePointCloudToPLY(pointCloud, filename)) {
        std::cout << "点云数据已成功保存到文件: " << filename << std::endl;
    } else {
        std::cerr << "保存点云数据失败" << std::endl;
    }
    
    // 停止捕获并关闭设备
    reader.stopCapture();
    
    std::cout << "\n=== 测试程序执行完毕 ===" << std::endl;
    std::cout << "按任意键退出..." << std::endl;
    std::cin.get();
    
    return 0;
}