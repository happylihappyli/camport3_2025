#include <iostream>
#include <iomanip>
#include "TYApi.h"
#include "TYDefs.h"
#include <cstdlib>

using namespace std;

void printCalibInfo(const char* name, TY_CAMERA_CALIB_INFO& calib) {
    cout << "\n=== " << name << " 校准参数 ===" << endl;
    cout << "图像尺寸: " << calib.intrinsicWidth << "x" << calib.intrinsicHeight << endl;
    cout << "\n内参矩阵: " << endl;
    cout << "fx: " << calib.intrinsic.data[0] << endl;
    cout << "fy: " << calib.intrinsic.data[4] << endl;
    cout << "cx: " << calib.intrinsic.data[2] << endl;
    cout << "cy: " << calib.intrinsic.data[5] << endl;
    
    cout << "\n外参矩阵: " << endl;
    cout << "旋转矩阵 R: " << endl;
    for(int i = 0; i < 3; i++) {
        cout << "  ";
        for(int j = 0; j < 3; j++) {
            cout << setw(10) << calib.extrinsic.data[i*4 + j] << " ";
        }
        cout << endl;
    }
    cout << "平移向量 T: " << endl;
    cout << "  " << calib.extrinsic.data[3] << " " << calib.extrinsic.data[7] << " " << calib.extrinsic.data[11] << endl;
    
    cout << "\n畸变系数: " << endl;
    cout << "k1: " << calib.distortion.data[0] << endl;
    cout << "k2: " << calib.distortion.data[1] << endl;
    cout << "p1: " << calib.distortion.data[2] << endl;
    cout << "p2: " << calib.distortion.data[3] << endl;
    cout << "k3: " << calib.distortion.data[4] << endl;
}

// 初始化库并获取接口列表
static bool initializeLibAndGetInterfaces(TY_INTERFACE_INFO** pIfaceList, uint32_t* ifaceCount) {
    // 更新接口列表
    TY_STATUS status = TYUpdateInterfaceList();
    if (status != TY_STATUS_OK) {
        cout << "Failed to update interface list: " << status << endl;
        return false;
    }

    // 获取接口数量
    *ifaceCount = 0;
    status = TYGetInterfaceNumber(ifaceCount);
    if (status != TY_STATUS_OK) {
        cout << "Failed to get interface number: " << status << endl;
        return false;
    }

    if (*ifaceCount == 0) {
        cout << "No interface found" << endl;
        return false;
    }

    // 获取接口列表
    *pIfaceList = (TY_INTERFACE_INFO*)malloc(sizeof(TY_INTERFACE_INFO) * (*ifaceCount));
    if (!*pIfaceList) {
        cout << "Failed to allocate memory for interface list" << endl;
        return false;
    }
    status = TYGetInterfaceList(*pIfaceList, *ifaceCount, ifaceCount);
    if (status != TY_STATUS_OK) {
        cout << "Failed to get interface list: " << status << endl;
        free(*pIfaceList);
        *pIfaceList = NULL;
        return false;
    }

    cout << "Found " << *ifaceCount << " interfaces" << endl;
    return true;
}

// 从接口列表中打开设备
static bool openDeviceFromInterfaceList(
    TY_INTERFACE_INFO* pIfaceList, 
    uint32_t ifaceCount, TY_DEV_HANDLE* device) {
    
    *device = NULL;
    bool deviceOpened = false;

    cout << "Starting device enumeration..." << endl;

    // 遍历所有接口，尝试打开设备
    for (uint32_t ifaceIdx = 0; ifaceIdx < ifaceCount; ifaceIdx++) {
        cout << "Trying interface " << ifaceIdx << ": " << pIfaceList[ifaceIdx].id << ", type: " << pIfaceList[ifaceIdx].type << endl;

        // 打开接口
        TY_INTERFACE_HANDLE ifaceHandle = NULL;
        TY_STATUS status = TYOpenInterface(pIfaceList[ifaceIdx].id, &ifaceHandle);
        if (status == TY_STATUS_OK) {
            cout << "Interface opened successfully" << endl;
            
            // 更新设备列表
            status = TYUpdateDeviceList(ifaceHandle);
            if (status == TY_STATUS_OK) {
                cout << "Device list updated successfully" << endl;
            } else {
                cout << "Failed to update device list: " << status << endl;
            }
            
            // 增加设备列表缓冲区大小
            uint32_t bufferSize = 20;
            TY_DEVICE_BASE_INFO* pDeviceList = (TY_DEVICE_BASE_INFO*)malloc(sizeof(TY_DEVICE_BASE_INFO) * bufferSize);
            if (pDeviceList) {
                uint32_t deviceCount = 0;
                status = TYGetDeviceList(ifaceHandle, pDeviceList, bufferSize, &deviceCount);
                if (status == TY_STATUS_OK) {
                    cout << "Got device list successfully, found " << deviceCount << " devices on this interface" << endl;
                    
                    if (deviceCount > 0) {
                        // 打印所有设备信息
                        for (uint32_t devIdx = 0; devIdx < deviceCount; devIdx++) {
                            cout << "  Device " << devIdx << ": ID=" << pDeviceList[devIdx].id 
                                      << ", Vendor=" << pDeviceList[devIdx].vendorName 
                                      << ", Model=" << pDeviceList[devIdx].modelName << endl;
                        }
                        
                        // 尝试打开第一个设备
                        const char* targetDeviceID = pDeviceList[0].id;
                        cout << "Attempting to open device: " << targetDeviceID << endl;
                        
                        // 尝试打开设备
                        status = TYOpenDevice(ifaceHandle, targetDeviceID, device);
                        if (status == TY_STATUS_OK) {
                            deviceOpened = true;
                            cout << "Device opened successfully" << endl;
                            cout << "Device ID: " << pDeviceList[0].id << endl;
                            cout << "Vendor: " << pDeviceList[0].vendorName << endl;
                            cout << "Model: " << pDeviceList[0].modelName << endl;
                        } else {
                            cout << "Failed to open device: " << status << endl;
                        }
                    } else {
                        cout << "No devices found on this interface" << endl;
                    }
                } else {
                    cout << "Failed to get device list: " << status << endl;
                }
                free(pDeviceList);
            } else {
                cout << "Failed to allocate memory for device list" << endl;
            }
            TYCloseInterface(ifaceHandle);
            
            // 如果已经打开设备，退出循环
            if (deviceOpened) {
                break;
            }
        } else {
            cout << "Failed to open interface: " << status << endl;
        }
    }
    return deviceOpened;
}

int main() {
    TY_STATUS status;
    TY_DEV_HANDLE handle;
    
    // 初始化库
    status = TYInitLib();
    if(status != TY_STATUS_OK) {
        cout << "初始化库失败: " << status << endl;
        return -1;
    }
    
    // 获取接口列表
    TY_INTERFACE_INFO* ifaceList = NULL;
    uint32_t ifaceCount = 0;
    if (!initializeLibAndGetInterfaces(&ifaceList, &ifaceCount)) {
        TYDeinitLib();
        return -1;
    }
    
    // 从接口列表中打开设备
    if (!openDeviceFromInterfaceList(ifaceList, ifaceCount, &handle)) {
        cout << "无法打开设备" << endl;
        free(ifaceList);
        TYDeinitLib();
        return -1;
    }
    
    // 获取深度相机校准参数
    TY_CAMERA_CALIB_INFO depth_calib;
    status = TYGetStruct(handle, TY_COMPONENT_DEPTH_CAM, TY_STRUCT_CAM_CALIB_DATA, &depth_calib, sizeof(depth_calib));
    if(status == TY_STATUS_OK) {
        printCalibInfo("深度相机", depth_calib);
    } else {
        cout << "获取深度相机校准参数失败: " << status << endl;
    }
    
    // 获取彩色相机校准参数
    TY_CAMERA_CALIB_INFO color_calib;
    bool has_color_calib = false;
    status = TYHasFeature(handle, TY_COMPONENT_RGB_CAM, TY_STRUCT_CAM_CALIB_DATA, &has_color_calib);
    if(has_color_calib) {
        status = TYGetStruct(handle, TY_COMPONENT_RGB_CAM, TY_STRUCT_CAM_CALIB_DATA, &color_calib, sizeof(color_calib));
        if(status == TY_STATUS_OK) {
            printCalibInfo("彩色相机", color_calib);
        } else {
            cout << "获取彩色相机校准参数失败: " << status << endl;
        }
    } else {
        cout << "设备不支持彩色相机校准参数" << endl;
    }
    
    // 获取深度比例单位
    float depth_scale_unit;
    status = TYGetFloat(handle, TY_COMPONENT_DEPTH_CAM, TY_FLOAT_SCALE_UNIT, &depth_scale_unit);
    if(status == TY_STATUS_OK) {
        cout << "\n=== 深度比例单位 ===" << endl;
        cout << "深度比例单位: " << depth_scale_unit << " (毫米)" << endl;
    } else {
        cout << "获取深度比例单位失败: " << status << endl;
    }
    
    // 关闭设备
    TYCloseDevice(handle);
    free(ifaceList);
    TYDeinitLib();
    
    return 0;
}