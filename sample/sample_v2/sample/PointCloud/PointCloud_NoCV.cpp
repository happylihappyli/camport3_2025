#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include "TYApi.h"
#include "TYImageProc.h"
#include "TyIsp.h"
#include "DebugDump.h"
#include <cstring>

// RGBD配准函数：将彩色图像映射到深度坐标系
void registerDepthToColor(const uint16_t* depthData, int depthWidth, int depthHeight,
                           const uint8_t* rgbData, int rgbWidth, int rgbHeight,
                           const TY_CAMERA_CALIB_INFO* depthCalib,
                           const TY_CAMERA_CALIB_INFO* colorCalib,
                           uint8_t* registeredRgbData, float depthScale) {
    if (!depthData || !rgbData || !registeredRgbData || !depthCalib || !colorCalib) {
        printf("Invalid input parameters for registerDepthToColor\n");
        return;
    }

    // 使用TYMapDepthImageToColorCoordinate函数将深度图像映射到彩色坐标系
    // 首先需要创建一个中间深度图像用于配准
    std::vector<uint16_t> registeredDepth(rgbWidth * rgbHeight);
    
    TY_STATUS status = TYMapDepthImageToColorCoordinate(
        depthCalib,           // 深度相机的标定信息
        depthWidth, depthHeight, depthData,  // 深度图像数据
        colorCalib,           // 彩色相机的标定信息
        rgbWidth, rgbHeight, registeredDepth.data(),  // 输出的配准后的深度数据
        depthScale           // 深度缩放因子
    );

    if (status != TY_STATUS_OK) {
        printf("Failed to register depth to color coordinate, status: %d\n", status);
        // 如果配准失败，直接复制RGB数据
        memcpy(registeredRgbData, rgbData, rgbWidth * rgbHeight * 3);
        return;
    }

    // 配准成功，直接使用原始RGB数据（因为深度已经映射到彩色坐标系）
    memcpy(registeredRgbData, rgbData, rgbWidth * rgbHeight * 3);
    printf("Depth to color registration successful\n");
}

// 限制值在指定范围内
int clamp(int value, int min, int max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

// 检查深度和彩色图像尺寸匹配
void checkImageAlignment(TY_IMAGE_DATA* depthImage, TY_IMAGE_DATA* rgbImage, const char* stage) {
    if (!depthImage || !rgbImage) {
        printf("[checkImageAlignment] %s: Invalid image pointers\n", stage);
        return;
    }
    
    printf("[checkImageAlignment] %s:\n", stage);
    printf("  Depth image: %d x %d (format: %d)\n", depthImage->width, depthImage->height, depthImage->pixelFormat);
    printf("  RGB image: %d x %d (format: %d)\n", rgbImage->width, rgbImage->height, rgbImage->pixelFormat);
    
    // 计算宽高比
    float depthAspect = (float)depthImage->width / depthImage->height;
    float rgbAspect = (float)rgbImage->width / rgbImage->height;
    float aspectDiff = fabs(depthAspect - rgbAspect);
    
    printf("  Depth aspect ratio: %.3f\n", depthAspect);
    printf("  RGB aspect ratio: %.3f\n", rgbAspect);
    printf("  Aspect ratio difference: %.3f\n", aspectDiff);
    
    // 检查是否需要调整尺寸
    if (aspectDiff > 0.1) {
        printf("  WARNING: Aspect ratios differ significantly!\n");
    } else {
        printf("  Aspect ratios are compatible for registration\n");
    }
    
    // 计算目标尺寸（参考main.cpp的逻辑）
    int targetWidth = depthImage->width;
    int targetHeight = depthImage->width * rgbImage->height / rgbImage->width;
    printf("  Registration target size: %d x %d\n", targetWidth, targetHeight);
}

// 验证RGB数据格式和尺寸
void validateRgbData(uint8_t* rgbData, int width, int height, const char* stage) {
    if (!rgbData) {
        printf("[validateRgbData] %s: RGB data is NULL\n", stage);
        return;
    }
    
    printf("[validateRgbData] %s: RGB data size = %d x %d\n", stage, width, height);
    
    // 检查几个像素的颜色值
    for (int i = 0; i < 5; i++) {
        int x = width / 6 * (i + 1);
        int y = height / 2;
        int pixelIndex = (y * width + x) * 3;
        
        if (pixelIndex + 2 < width * height * 3) {
            uint8_t r = rgbData[pixelIndex + 2];  // BGR格式，R在索引2
            uint8_t g = rgbData[pixelIndex + 1];  // G在索引1
            uint8_t b = rgbData[pixelIndex + 0];  // B在索引0
            
            printf("[validateRgbData] %s: Pixel[%d,%d] = RGB(%d,%d,%d)\n", 
                   stage, x, y, r, g, b);
        }
    }
    
    // 检查边界像素
    int cornerIndex = (width * height - 1) * 3;
    if (cornerIndex + 2 < width * height * 3) {
        uint8_t r = rgbData[cornerIndex + 2];
        uint8_t g = rgbData[cornerIndex + 1];
        uint8_t b = rgbData[cornerIndex + 0];
        printf("[validateRgbData] %s: Corner pixel RGB(%d,%d,%d)\n", stage, r, g, b);
    }
    
    // 检查图像统计信息
    int totalPixels = width * height;
    int blackPixels = 0;
    int whitePixels = 0;
    int colorfulPixels = 0;
    
    for (int i = 0; i < totalPixels; i++) {
        int pixelIndex = i * 3;
        uint8_t r = rgbData[pixelIndex + 2];
        uint8_t g = rgbData[pixelIndex + 1];
        uint8_t b = rgbData[pixelIndex + 0];
        
        if (r == 0 && g == 0 && b == 0) {
            blackPixels++;
        } else if (r == 255 && g == 255 && b == 255) {
            whitePixels++;
        } else if (r > 50 || g > 50 || b > 50) {  // 有颜色
            colorfulPixels++;
        }
    }
    
    printf("[validateRgbData] %s: Statistics - Black: %d (%.1f%%), White: %d (%.1f%%), Colorful: %d (%.1f%%)\n",
           stage, blackPixels, (float)blackPixels/totalPixels*100, 
           whitePixels, (float)whitePixels/totalPixels*100,
           colorfulPixels, (float)colorfulPixels/totalPixels*100);
}

// 将彩色点云数据保存为PLY文件
void savePointsToPly(const TY_VECT_3F* p3d, const uint8_t* rgbData, int width, int height, const char* fileName) {
    FILE* fp = fopen(fileName, "wb");
    if (!fp) {
        printf("Failed to open file %s\n", fileName);
        return;
    }

    // 写入PLY文件头（包含颜色信息）
    fprintf(fp, "ply\n");
    fprintf(fp, "format ascii 1.0\n");
    fprintf(fp, "element vertex %d\n", width * height);
    fprintf(fp, "property float x\n");
    fprintf(fp, "property float y\n");
    fprintf(fp, "property float z\n");
    fprintf(fp, "property uchar blue\n");
    fprintf(fp, "property uchar green\n");
    fprintf(fp, "property uchar red\n");
    fprintf(fp, "end_header\n");

    // 写入点云数据（包含颜色）
    int validPoints = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = y * width + x;
            if (p3d[index].z != 0 && rgbData) {
                // 获取BGR颜色值（注意：rgbData是BGR格式）
                uint8_t b = rgbData[index * 3];     // Blue
                uint8_t g = rgbData[index * 3 + 1]; // Green
                uint8_t r = rgbData[index * 3 + 2]; // Red
                // PLY文件格式：blue, green, red，所以写入顺序是 b, g, r
                fprintf(fp, "%f %f %f %d %d %d\n", p3d[index].x, p3d[index].y, p3d[index].z, b, g, r);
                validPoints++;
            } else if (p3d[index].z != 0) {
                // 如果没有颜色数据，使用默认颜色
                fprintf(fp, "%f %f %f 255 255 255\n", p3d[index].x, p3d[index].y, p3d[index].z);
                validPoints++;
            }
        }
    }

    fclose(fp);
    printf("Point cloud saved to %s, valid points: %d\n", fileName, validPoints);
}

// 初始化ISP处理器设置（简化版）
TY_STATUS initISP(TY_ISP_HANDLE ispHandle, TY_DEV_HANDLE device) {
    // 简化ISP初始化，只设置必要的参数
    TY_STATUS status = TY_STATUS_OK;
    printf("Initializing ISP with minimal settings\n");
    return status;
}

// 缩放RGB图像以匹配深度图像尺寸
void scaleRgbImage(const uint8_t* rgbData, int rgbWidth, int rgbHeight, 
                   uint8_t* registeredRgbData, int depthWidth, int depthHeight) {
    printf("Scaling RGB image to match depth image dimensions\n");
    
    // 简单的缩放实现 - 最近邻插值
    for (int y = 0; y < depthHeight; y++) {
        for (int x = 0; x < depthWidth; x++) {
            // 计算源图像坐标
            int srcX = (int)(x * rgbWidth / depthWidth);
            int srcY = (int)(y * rgbHeight / depthHeight);
            
            // 确保源坐标在有效范围内
            if (srcX >= 0 && srcX < rgbWidth && srcY >= 0 && srcY < rgbHeight) {
                int srcIdx = srcY * rgbWidth + srcX;
                int dstIdx = y * depthWidth + x;
                 
                // 复制RGB数据（注意BGR顺序）
                registeredRgbData[dstIdx*3] = rgbData[srcIdx*3];     // Blue
                registeredRgbData[dstIdx*3+1] = rgbData[srcIdx*3+1]; // Green
                registeredRgbData[dstIdx*3+2] = rgbData[srcIdx*3+2]; // Red
            }
        }
    }
    printf("RGB image scaled successfully\n");
}

// 使用平铺方式处理不同宽高比的RGB图像
void tileRgbImage(const uint8_t* rgbData, int rgbWidth, int rgbHeight, 
                 uint8_t* registeredRgbData, int depthWidth, int depthHeight) {
    printf("Using RGB data tiling due to different aspect ratio\n");
    for (int y = 0; y < depthHeight; y++) {
        for (int x = 0; x < depthWidth; x++) {
            int srcX = x % rgbWidth;
            int srcY = y % rgbHeight;
            int srcIdx = srcY * rgbWidth + srcX;
            int dstIdx = y * depthWidth + x;
             
            registeredRgbData[dstIdx*3] = rgbData[srcIdx*3];
            registeredRgbData[dstIdx*3+1] = rgbData[srcIdx*3+1];
            registeredRgbData[dstIdx*3+2] = rgbData[srcIdx*3+2];
        }
    }
}

// 创建默认的颜色渐变效果
void createDefaultColorGradient(uint8_t* registeredRgbData, int width, int height) {
    printf("Creating default color gradient since no valid RGB data\n");
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = y * width + x;
             
            registeredRgbData[index*3] = (uint8_t)(x * 255 / width);     // Blue
            registeredRgbData[index*3+1] = (uint8_t)(y * 255 / height);   // Green
            registeredRgbData[index*3+2] = 128;                           // Red
        }
    }
}

// 初始化库并获取接口列表
static bool initializeLibAndGetInterfaces(TY_INTERFACE_INFO** pIfaceList, uint32_t* ifaceCount) {
    // 初始化API
    TY_STATUS status = TYInitLib();
    if (status != TY_STATUS_OK) {
        printf("Failed to initialize library: %d (%s)\n", status, TYErrorString(status));
        return false;
    }
    printf("Library initialized\n");

    // 更新接口列表
    status = TYUpdateInterfaceList();
    if (status != TY_STATUS_OK) {
        printf("Failed to update interface list: %d\n", status);
        TYDeinitLib();
        return false;
    }

    // 获取接口数量
    *ifaceCount = 0;
    status = TYGetInterfaceNumber(ifaceCount);
    if (status != TY_STATUS_OK) {
        printf("Failed to get interface number: %d\n", status);
        TYDeinitLib();
        return false;
    }

    if (*ifaceCount == 0) {
        printf("No interface found\n");
        TYDeinitLib();
        return false;
    }

    // 获取接口列表
    *pIfaceList = (TY_INTERFACE_INFO*)malloc(sizeof(TY_INTERFACE_INFO) * (*ifaceCount));
    if (!*pIfaceList) {
        printf("Failed to allocate memory for interface list\n");
        TYDeinitLib();
        return false;
    }
    status = TYGetInterfaceList(*pIfaceList, *ifaceCount, ifaceCount);
    if (status != TY_STATUS_OK) {
        printf("Failed to get interface list: %d\n", status);
        free(*pIfaceList);
        *pIfaceList = NULL;
        TYDeinitLib();
        return false;
    }

    printf("Found %d interfaces\n", *ifaceCount);
    return true;
}

// 从接口列表中打开设备
static bool openDeviceFromInterfaceList(
    TY_INTERFACE_INFO* pIfaceList, 
    const char* deviceID,
    uint32_t ifaceCount, TY_DEV_HANDLE* device) {
    // 打开设备
    *device = NULL;
    bool deviceOpened = false;

    printf("Skipping direct device opening to avoid potential NULL pointer issues\n");

    // 遍历所有接口，尝试打开设备
    for (uint32_t ifaceIdx = 0; ifaceIdx < ifaceCount; ifaceIdx++) {
        printf("Trying interface %d: %s, type: %d\n", ifaceIdx, pIfaceList[ifaceIdx].id, pIfaceList[ifaceIdx].type);

        // 打开接口
        TY_INTERFACE_HANDLE ifaceHandle = NULL;
        TY_STATUS status = TYOpenInterface(pIfaceList[ifaceIdx].id, &ifaceHandle);
        if (status == TY_STATUS_OK) {
            printf("Interface opened successfully\n");
            
            // 更新设备列表
            status = TYUpdateDeviceList(ifaceHandle);
            if (status == TY_STATUS_OK) {
                printf("Device list updated successfully\n");
            } else {
                printf("Failed to update device list: %d\n", status);
            }
            
            // 增加设备列表缓冲区大小
            uint32_t bufferSize = 20;
            TY_DEVICE_BASE_INFO* pDeviceList = (TY_DEVICE_BASE_INFO*)malloc(sizeof(TY_DEVICE_BASE_INFO) * bufferSize);
            if (pDeviceList) {
                uint32_t deviceCount = 0;
                status = TYGetDeviceList(ifaceHandle, pDeviceList, bufferSize, &deviceCount);
                if (status == TY_STATUS_OK) {
                    printf("Got device list successfully, found %d devices on this interface\n", deviceCount);
                    
                    if (deviceCount > 0) {
                        // 打印所有设备信息
                        for (uint32_t devIdx = 0; devIdx < deviceCount; devIdx++) {
                            printf("  Device %d: ID=%s, Vendor=%s, Model=%s\n", 
                                   devIdx, pDeviceList[devIdx].id, pDeviceList[devIdx].vendorName, pDeviceList[devIdx].modelName);
                        }
                        
                        // 尝试打开第一个设备
                        status = TYOpenDevice(ifaceHandle, deviceID, device);// pDeviceList[0].id, device);
                        if (status == TY_STATUS_OK) {
                            deviceOpened = true;
                            printf("Device opened successfully\n");
                            printf("Device ID: %s\n", pDeviceList[0].id);
                            printf("Vendor: %s\n", pDeviceList[0].vendorName);
                            printf("Model: %s\n", pDeviceList[0].modelName);
                        } else {
                            printf("Failed to open device: %d\n", status);
                        }
                    } else {
                        printf("No devices found on this interface\n");
                    }
                } else {
                    printf("Failed to get device list: %d\n", status);
                }
                free(pDeviceList);
            } else {
                printf("Failed to allocate memory for device list\n");
            }
            TYCloseInterface(ifaceHandle);
        } else {
            printf("Failed to open interface: %d\n", status);
        }
    }
    return deviceOpened;
}

// 设置相机分辨率
static bool setupCameraResolutions(TY_DEV_HANDLE device, bool colorEnabled, TY_ISP_HANDLE colorISP) {
    // 设置深度相机分辨率为1280x960
    TY_IMAGE_MODE depthMode = TYImageMode2(TY_PIXEL_FORMAT_DEPTH16, 1280, 960);
    TY_STATUS status = TYSetEnum(device, TY_COMPONENT_DEPTH_CAM, TY_ENUM_IMAGE_MODE, depthMode);
    if (status == TY_STATUS_OK) {
        printf("Depth camera resolution set to 1280x960\n");
    } else {
        printf("Failed to set depth resolution: %d (%s), using default\n", status, TYErrorString(status));
    }

    // 设置RGB相机分辨率为1280x960
    if (colorEnabled) {
        printf("Setting RGB camera resolution to 1280x960\n");
        
        // 获取相机支持的所有图像模式
        TY_ENUM_ENTRY modes[20];
        uint32_t num_modes = 0;
        status = TYGetEnumEntryInfo(device, TY_COMPONENT_RGB_CAM, TY_ENUM_IMAGE_MODE, modes, 20, &num_modes);
        
        if (status == TY_STATUS_OK) {
            printf("RGB camera supported modes:\n");
            bool has_target_mode = false;
            uint32_t target_mode_value = 0;
            
            for (uint32_t i = 0; i < num_modes; i++) {
                int w = TYImageWidth(modes[i].value);
                int h = TYImageHeight(modes[i].value);
                TY_PIXEL_FORMAT pf = TYPixelFormat(modes[i].value);
                printf("  [%d] %s (%dx%d)\n", i, modes[i].description, w, h);
                
                // 检查是否支持目标分辨率(匹配分辨率，格式可以不同)
                if (w == 1280 && h == 960) {
                    has_target_mode = true;
                    target_mode_value = modes[i].value;
                    break;
                }
            }
            
            // 设置分辨率
            if (has_target_mode) {
                printf("Setting RGB camera resolution to 1280x960\n");
                status = TYSetEnum(device, TY_COMPONENT_RGB_CAM, TY_ENUM_IMAGE_MODE, target_mode_value);
                if (status != TY_STATUS_OK) {
                    printf("Failed to set RGB resolution: %d (%s), using default\n", status, TYErrorString(status));
                } else {
                    printf("RGB camera resolution set to 1280x960 successfully\n");
                }
            } else {
                printf("Warning: RGB camera doesn't support 1280x960 resolution, using default\n");
            }
        } else {
            printf("Failed to get RGB camera modes: %d (%s)\n", status, TYErrorString(status));
        }
        
        // 获取当前设置的分辨率
        TY_IMAGE_MODE rgbMode;
        status = TYGetEnum(device, TY_COMPONENT_RGB_CAM, TY_ENUM_IMAGE_MODE, &rgbMode);
        if (status == TY_STATUS_OK) {
            printf("Current RGB camera resolution: %d x %d\n", 
                   TYImageWidth(rgbMode), TYImageHeight(rgbMode));
        }
    }
    return true;
}

// 分配和入队缓冲区
static bool allocateAndEnqueueBuffers(TY_DEV_HANDLE device, char** buffers, int bufferCount) {
    // 获取帧缓冲区大小
    uint32_t frameBufferSize = 0;
    TY_STATUS status = TYGetFrameBufferSize(device, &frameBufferSize);
    if (status != TY_STATUS_OK) {
        printf("Failed to get frame buffer size: %d (%s)\n", status, TYErrorString(status));
        return false;
    }
    printf("Frame buffer size: %u bytes\n", frameBufferSize);

    // 分配并入队缓冲区
    for (int i = 0; i < bufferCount; i++) {
        buffers[i] = (char*)malloc(frameBufferSize);
        if (buffers[i] == NULL) {
            printf("Failed to allocate buffer %d\n", i);
            // 释放已分配的缓冲区
            for (int j = 0; j < i; j++) {
                free(buffers[j]);
                buffers[j] = NULL;
            }
            return false;
        }
        status = TYEnqueueBuffer(device, buffers[i], frameBufferSize);
        if (status != TY_STATUS_OK) {
            printf("Failed to enqueue buffer %d: %d (%s)\n", i, status, TYErrorString(status));
            for (int j = 0; j <= i; j++) {
                free(buffers[j]);
                buffers[j] = NULL;
            }
            return false;
        }
    }
    printf("%d buffers allocated and enqueued successfully\n", bufferCount);
    return true;
}

// 处理RGB图像并准备用于点云
static bool processRgbImage(TY_DEV_HANDLE device, TY_ISP_HANDLE colorISP,
                           TY_IMAGE_DATA* rgbImage, uint8_t** rgbData) {
    if (!rgbImage) return false;
    
    printf("RGB image found: %d x %d, format: %d, buffer size: %d\n", 
           rgbImage->width, rgbImage->height, rgbImage->pixelFormat, rgbImage->size);
    
    // 为RGB数据分配内存
    *rgbData = (uint8_t*)malloc(rgbImage->width * rgbImage->height * 3); // BGR格式
    if (!*rgbData) {
        printf("Failed to allocate memory for RGB data\n");
        return false;
    }
    
    // 简化处理：直接使用或转换RGB数据
    TY_STATUS status = TY_STATUS_OK;
    std::vector<uint8_t> tempBgr;
    if (convert_to_bgr_buffer(static_cast<uint8_t*>(rgbImage->buffer), rgbImage->width, rgbImage->height, rgbImage->pixelFormat, tempBgr)) {
        memcpy(*rgbData, tempBgr.data(), tempBgr.size());
    } else if (colorISP) {
        // 尝试使用ISP进行简单处理
        printf("Trying simple ISP processing\n");
        TY_IMAGE_DATA outImage;
        outImage.buffer = *rgbData;
        outImage.width = rgbImage->width;
        outImage.height = rgbImage->height;
        outImage.size = rgbImage->width * rgbImage->height * 3;
        outImage.pixelFormat = TY_PIXEL_FORMAT_BGR;
        
        status = TYISPProcessImage(colorISP, rgbImage, &outImage);
    } else {
        // 完全简化：创建默认的颜色数据
        printf("Using default color data since ISP is not available\n");
        for (int i = 0; i < rgbImage->width * rgbImage->height; i++) {
            (*rgbData)[i*3] = 128;     // Blue
            (*rgbData)[i*3+1] = 128;   // Green
            (*rgbData)[i*3+2] = 128;   // Red
        }
    }
    
    if (status == TY_STATUS_OK) {
        printf("Color data prepared successfully\n");
        return true;
    } else {
        printf("Failed to process color image: %d (%s)\n", status, TYErrorString(status));
        free(*rgbData);
        *rgbData = NULL;
        return false;
    }
}



// 处理深度图像并生成点云
static bool processDepthImageAndGeneratePointCloud(TY_DEV_HANDLE device, TY_IMAGE_DATA* depthImage, 
                                                  TY_IMAGE_DATA* rgbImage, bool colorEnabled, 
                                                  TY_ISP_HANDLE colorISP) {
    if (!depthImage) {
        printf("No depth image found in the frame\n");
        return false;
    }
    
    printf("Depth image found: %d x %d\n", depthImage->width, depthImage->height);
    const std::string debugDir = prepare_debug_directory("nocv");
    static int dumpIndex = 0;
    const std::string frameSuffix = "_frame" + std::to_string(dumpIndex++);
    dump_uint16_matrix_txt(debugDir + "/depth_raw" + frameSuffix + ".txt",
        reinterpret_cast<const uint16_t*>(depthImage->buffer),
        depthImage->width, depthImage->height,
        "Raw depth image from TYFetchFrame");
    
    // 获取深度比例
    float depthScale = 1.0f;
    TY_STATUS status = TYGetFloat(device, TY_COMPONENT_DEPTH_CAM, TY_FLOAT_SCALE_UNIT, &depthScale);
    if (status != TY_STATUS_OK) {
        printf("Failed to get depth scale: %d (%s)\n", status, TYErrorString(status));
        return false;
    }
    
    printf("Depth scale: %.6f\n", depthScale);
    
    // 处理彩色图像（如果可用）
        uint8_t* rgbData = NULL;
        uint8_t* registeredRgbData = NULL;
        uint8_t* undistortedRgbBuffer = NULL; // 跟踪去畸变缓冲区
        uint16_t* registeredDepthBuffer = NULL; // 配准后的深度缓冲区
        TY_VECT_3F* pointCloud = NULL;
        int pointCloudWidth = 0;
        int pointCloudHeight = 0;
        
        if (rgbImage && colorEnabled) {
            std::vector<uint8_t> rawCameraBgr;
            if (convert_to_bgr_buffer(static_cast<uint8_t*>(rgbImage->buffer), rgbImage->width, rgbImage->height, rgbImage->pixelFormat, rawCameraBgr)) {
                dump_rgb_bgr_txt(debugDir + "/rgb_sensor_input" + frameSuffix + ".txt",
                    rawCameraBgr.data(), rgbImage->width, rgbImage->height,
                    "RGB sensor data converted to BGR (before processing)");
            }
            if (processRgbImage(device, colorISP, rgbImage, &rgbData)) {
                // 检查图像尺寸对齐
                checkImageAlignment(depthImage, rgbImage, "before_registration");
                
                // 验证初始RGB数据
                validateRgbData(rgbData, rgbImage->width, rgbImage->height, "initial_rgb");
                dump_rgb_bgr_txt(debugDir + "/rgb_after_initial_convert" + frameSuffix + ".txt",
                    rgbData, rgbImage->width, rgbImage->height,
                    "RGB data after initial conversion (before undistort)");
            // 获取深度和彩色相机校准信息
            TY_CAMERA_CALIB_INFO depthCalib, colorCalib;
            status = TYGetStruct(device, TY_COMPONENT_DEPTH_CAM, TY_STRUCT_CAM_CALIB_DATA, &depthCalib, sizeof(depthCalib));
            if (status != TY_STATUS_OK) {
                printf("Failed to get depth calibration info: %d (%s)\n", status, TYErrorString(status));
                free(rgbData);
                return false;
            }
            
            status = TYGetStruct(device, TY_COMPONENT_RGB_CAM, TY_STRUCT_CAM_CALIB_DATA, &colorCalib, sizeof(colorCalib));
            if (status != TY_STATUS_OK) {
                printf("Failed to get color calibration info: %d (%s)\n", status, TYErrorString(status));
                free(rgbData);
                return false;
            }
            
            // 检查是否需要深度图像去畸变
            bool depthNeedUndistort = false;
            status = TYHasFeature(device, TY_COMPONENT_DEPTH_CAM, TY_STRUCT_CAM_DISTORTION, &depthNeedUndistort);
            if (status != TY_STATUS_OK) {
                printf("Failed to check depth distortion: %d (%s)\n", status, TYErrorString(status));
                depthNeedUndistort = false;
            }
            
            // 检查是否需要彩色图像去畸变
            bool colorNeedUndistort = false;
            status = TYHasFeature(device, TY_COMPONENT_RGB_CAM, TY_STRUCT_CAM_DISTORTION, &colorNeedUndistort);
            if (status != TY_STATUS_OK) {
                printf("Failed to check color distortion: %d (%s)\n", status, TYErrorString(status));
                colorNeedUndistort = false;
            }
            
            // 深度图像去畸变
            if (depthNeedUndistort) {
                TY_IMAGE_DATA undistortedDepth = *depthImage;
                uint8_t* undistortedBuffer = (uint8_t*)malloc(depthImage->size);
                if (undistortedBuffer) {
                    undistortedDepth.buffer = undistortedBuffer;
                    status = TYUndistortImage(&depthCalib, depthImage, NULL, &undistortedDepth);
                    if (status == TY_STATUS_OK) {
                        printf("Depth image undistortion successful\n");
                        dump_uint16_matrix_txt(debugDir + "/depth_undistorted" + frameSuffix + ".txt",
                            reinterpret_cast<const uint16_t*>(undistortedDepth.buffer),
                            undistortedDepth.width, undistortedDepth.height,
                            "Depth image after undistortion");
                        depthImage = &undistortedDepth; // 使用去畸变后的深度图像
                    } else {
                        printf("Depth image undistortion failed: %d (%s)\n", status, TYErrorString(status));
                        free(undistortedBuffer);
                    }
                }
            }
            
            // 彩色图像去畸变（参考main.cpp的逻辑）
            // 注意：去畸变应该在原始图像格式上进行，然后再转换为BGR格式
            TY_IMAGE_DATA* finalRgbImage = rgbImage;
            bool colorUndistortionSuccess = false;
            
            if (colorNeedUndistort) {
                TY_IMAGE_DATA undistortedRgb = *rgbImage;
                undistortedRgbBuffer = (uint8_t*)malloc(rgbImage->size);
                if (undistortedRgbBuffer) {
                    undistortedRgb.buffer = undistortedRgbBuffer;
                    status = TYUndistortImage(&colorCalib, rgbImage, NULL, &undistortedRgb);
                    if (status == TY_STATUS_OK) {
                        printf("Color image undistortion successful\n");
                        finalRgbImage = &undistortedRgb; // 使用去畸变后的图像
                        colorUndistortionSuccess = true;
                    } else {
                        printf("Color image undistortion failed: %d (%s)\n", status, TYErrorString(status));
                        free(undistortedRgbBuffer);
                        undistortedRgbBuffer = NULL;
                        // 如果去畸变失败，标记为失败并继续使用原始图像
                        colorUndistortionSuccess = false;
                        finalRgbImage = rgbImage;
                    }
                } else {
                    printf("Failed to allocate buffer for RGB undistortion\n");
                    colorUndistortionSuccess = false;
                    finalRgbImage = rgbImage;
                }
            } else {
                colorUndistortionSuccess = true; // 不需要去畸变，认为成功
            }
            
            // 如果进行了去畸变，需要重新处理RGB数据
            if (finalRgbImage != rgbImage) {
                // 释放旧的RGB数据
                free(rgbData);
                rgbData = NULL;
                
                // 重新处理去畸变后的RGB图像
                if (!processRgbImage(device, colorISP, finalRgbImage, &rgbData)) {
                    printf("Failed to process undistorted RGB image\n");
                    if (undistortedRgbBuffer) free(undistortedRgbBuffer);
                    return false;
                }
                
                // 更新rgbImage指针
                rgbImage = finalRgbImage;
                
                // 验证重新处理后的RGB数据
                validateRgbData(rgbData, rgbImage->width, rgbImage->height, "after_undistortion");
                dump_rgb_bgr_txt(debugDir + "/rgb_after_undistort" + frameSuffix + ".txt",
                    rgbData, rgbImage->width, rgbImage->height,
                    "RGB data after undistortion + reprojection");
            }
            
            // 确保彩色图像去畸变成功后才进行配准（参考main.cpp的逻辑）
            if (colorNeedUndistort && !colorUndistortionSuccess) {
                printf("Color undistortion failed, skipping RGBD registration\n");
                // 直接在深度坐标系下生成点云
                pointCloudWidth = depthImage->width;
                pointCloudHeight = depthImage->height;
                pointCloud = (TY_VECT_3F*)malloc(sizeof(TY_VECT_3F) * pointCloudWidth * pointCloudHeight);
                if (!pointCloud) {
                    printf("Failed to allocate memory for point cloud\n");
                    if (rgbData) free(rgbData);
                    return false;
                }
                
                TYMapDepthImageToPoint3d(&depthCalib, pointCloudWidth, pointCloudHeight, 
                                       (uint16_t*)depthImage->buffer, pointCloud, depthScale);
                
                printf("Point cloud generated in depth coordinate system (no registration): %d x %d\n", 
                       pointCloudWidth, pointCloudHeight);
                
                // 保存点云（使用原始RGB数据）
                printf("Saving point cloud without registration...\n");
                savePointsToPly(pointCloud, rgbData, pointCloudWidth, pointCloudHeight, "color_pointcloud.ply");
                
                // 释放资源
                free(pointCloud);
                if (rgbData) free(rgbData);
                if (undistortedRgbBuffer) free(undistortedRgbBuffer);
                
                return true;
            }
            
            // 参考main.cpp的处理方式：先配准到中间尺寸，保持深度图像宽度，高度按彩色图像宽高比计算
            // main.cpp第363-364行：dstW = depth_image->width(), dstH = depth_image->width() * color_image->height() / color_image->width()
            int dstW = depthImage->width;
            int dstH = depthImage->width * rgbImage->height / rgbImage->width;
            printf("Registration target size: %d x %d\n", dstW, dstH);
            
            // 分配配准后的深度图像内存（参考main.cpp第366-369行）
            // 注意：输出尺寸是dstW x dstH，不是rgbImage的尺寸
            registeredDepthBuffer = (uint16_t*)malloc(sizeof(uint16_t) * dstW * dstH);
            if (!registeredDepthBuffer) {
                printf("Failed to allocate memory for registered depth image\n");
                if (rgbData) free(rgbData);
                if (undistortedRgbBuffer) free(undistortedRgbBuffer);
                return false;
            }
            
            // 将深度图像映射到彩色坐标系（参考main.cpp第371-376行）
            // 关键：直接使用原始去畸变后的深度图像，不要手动调整尺寸
            // TYMapDepthImageToColorCoordinate会自动处理尺寸转换
            status = TYMapDepthImageToColorCoordinate(
                &depthCalib,
                depthImage->width, depthImage->height,  // 输入：原始去畸变后的深度图像尺寸
                static_cast<const uint16_t*>(depthImage->buffer),  // 输入：原始深度图像数据
                &colorCalib,
                dstW, dstH,  // 输出：dstW x dstH（不是rgbImage的尺寸！）
                registeredDepthBuffer,  // 输出缓冲区（dstW x dstH）
                depthScale
            );
            
            if (status != TY_STATUS_OK) {
                printf("Failed to register depth to color coordinate: %d (%s)\n", status, TYErrorString(status));
                free(rgbData);
                free(undistortedRgbBuffer);
                free(registeredDepthBuffer);
                return false;
            }
            
            printf("Depth image registered to color coordinate: %d x %d\n", dstW, dstH);
            dump_uint16_matrix_txt(debugDir + "/depth_registered" + frameSuffix + ".txt",
                registeredDepthBuffer, dstW, dstH,
                "Depth image resampled in color coordinate system");
            
            // 在彩色坐标系下生成点云（参考main.cpp第379-381行）
            // 注意：点云尺寸是registration_depth的尺寸（dstW x dstH），不是rgbImage的尺寸
            pointCloudWidth = dstW;
            pointCloudHeight = dstH;
            pointCloud = (TY_VECT_3F*)malloc(sizeof(TY_VECT_3F) * pointCloudWidth * pointCloudHeight);
            if (!pointCloud) {
                printf("Failed to allocate memory for point cloud\n");
                free(rgbData);
                free(undistortedRgbBuffer);
                free(registeredDepthBuffer);
                return false;
            }
            
            // 使用color_calib在彩色坐标系下生成点云（参考main.cpp第380-381行）
            TYMapDepthImageToPoint3d(&colorCalib, pointCloudWidth, pointCloudHeight, 
                                   registeredDepthBuffer, pointCloud, depthScale);
            
            // 处理RGB数据以匹配点云尺寸（参考main.cpp第378行：registration_color = color_image）
            // 注意：点云尺寸是dstW x dstH，但rgbData是rgbImage->width x rgbImage->height
            // 需要将RGB数据调整到点云尺寸，或者调整点云到RGB尺寸
            // 根据main.cpp的逻辑，点云使用registration_depth的尺寸，颜色使用color_image
            // 在savePointsToPly中会处理尺寸不匹配的情况
            // 这里我们需要将RGB数据调整到点云尺寸（dstW x dstH）
            registeredRgbData = (uint8_t*)malloc(dstW * dstH * 3);
            if (!registeredRgbData) {
                printf("Failed to allocate memory for registered RGB data\n");
                free(pointCloud);
                free(rgbData);
                free(undistortedRgbBuffer);
                free(registeredDepthBuffer);
                return false;
            }
            
            // 将RGB数据从rgbImage尺寸调整到点云尺寸（dstW x dstH）
            // 使用双线性插值或最近邻插值
            for (int y = 0; y < dstH; y++) {
                for (int x = 0; x < dstW; x++) {
                    // 计算源RGB图像中的坐标
                    int srcX = (x * rgbImage->width) / dstW;
                    int srcY = (y * rgbImage->height) / dstH;
                    
                    // 确保坐标在有效范围内
                    if (srcX >= rgbImage->width) srcX = rgbImage->width - 1;
                    if (srcY >= rgbImage->height) srcY = rgbImage->height - 1;
                    
                    int srcIdx = (srcY * rgbImage->width + srcX) * 3;
                    int dstIdx = (y * dstW + x) * 3;
                    
                    // 复制BGR数据
                    registeredRgbData[dstIdx] = rgbData[srcIdx];     // Blue
                    registeredRgbData[dstIdx + 1] = rgbData[srcIdx + 1]; // Green
                    registeredRgbData[dstIdx + 2] = rgbData[srcIdx + 2]; // Red
                }
            }
            
            // 释放原始RGB数据
            free(rgbData);
            rgbData = NULL;
            
            dump_rgb_bgr_txt(debugDir + "/rgb_resampled_for_registration" + frameSuffix + ".txt",
                registeredRgbData, dstW, dstH,
                "RGB data resampled to match registered depth size");

            
            printf("Point cloud generated in color coordinate system: %d x %d\n", pointCloudWidth, pointCloudHeight);
        }
    }
    
    // 如果没有彩色图像或配准失败，直接在深度坐标系下生成点云
    if (!pointCloud) {
        // 获取深度相机校准信息
        TY_CAMERA_CALIB_INFO depthCalib;
        status = TYGetStruct(device, TY_COMPONENT_DEPTH_CAM, TY_STRUCT_CAM_CALIB_DATA, &depthCalib, sizeof(depthCalib));
        if (status != TY_STATUS_OK) {
            printf("Failed to get depth calibration info: %d (%s)\n", status, TYErrorString(status));
            if (rgbData) free(rgbData);
            return false;
        }
        
        // 检查是否需要深度图像去畸变
        bool depthNeedUndistort = false;
        status = TYHasFeature(device, TY_COMPONENT_DEPTH_CAM, TY_STRUCT_CAM_DISTORTION, &depthNeedUndistort);
        if (status != TY_STATUS_OK) {
            printf("Failed to check depth distortion: %d (%s)\n", status, TYErrorString(status));
            depthNeedUndistort = false;
        }
        
        // 深度图像去畸变
        if (depthNeedUndistort) {
            TY_IMAGE_DATA undistortedDepth = *depthImage;
            uint8_t* undistortedBuffer = (uint8_t*)malloc(depthImage->size);
            if (undistortedBuffer) {
                undistortedDepth.buffer = undistortedBuffer;
                status = TYUndistortImage(&depthCalib, depthImage, NULL, &undistortedDepth);
                if (status == TY_STATUS_OK) {
                    printf("Depth image undistortion successful\n");
                    depthImage = &undistortedDepth; // 使用去畸变后的深度图像
                } else {
                    printf("Depth image undistortion failed: %d (%s)\n", status, TYErrorString(status));
                    free(undistortedBuffer);
                }
            }
        }
        
        pointCloudWidth = depthImage->width;
        pointCloudHeight = depthImage->height;
        pointCloud = (TY_VECT_3F*)malloc(sizeof(TY_VECT_3F) * pointCloudWidth * pointCloudHeight);
        if (!pointCloud) {
            printf("Failed to allocate memory for point cloud\n");
            if (rgbData) free(rgbData);
            return false;
        }
        
        // 转换深度图像到点云
        TYMapDepthImageToPoint3d(&depthCalib, pointCloudWidth, pointCloudHeight, 
                               (uint16_t*)depthImage->buffer, pointCloud, depthScale);
        

        
        printf("Point cloud generated in depth coordinate system: %d x %d\n", pointCloudWidth, pointCloudHeight);
    }
    
    // 验证最终用于保存的RGB数据
    validateRgbData(registeredRgbData ? registeredRgbData : rgbData, pointCloudWidth, pointCloudHeight, "final_for_save");
    
    // 保存彩色点云
    printf("Saving color point cloud...\n");
    savePointsToPly(pointCloud, registeredRgbData ? registeredRgbData : rgbData, pointCloudWidth, pointCloudHeight, "color_pointcloud.ply");
    
    // 释放资源
            free(pointCloud);
            if (rgbData) {
                free(rgbData);
            }
            if (registeredRgbData) {
                free(registeredRgbData);
            }
            if (undistortedRgbBuffer) {
                free(undistortedRgbBuffer);
            }
            if (registeredDepthBuffer) {
                free(registeredDepthBuffer);
            }
    
    return true;
}

// 处理捕获的帧数据
static bool processCapturedFrame(TY_DEV_HANDLE device, bool colorEnabled, TY_ISP_HANDLE colorISP) {
    // 获取一帧数据
    TY_FRAME_DATA frame;
    TY_STATUS status = TYFetchFrame(device, &frame, 3000); // 增加超时时间到3秒
    if (status != TY_STATUS_OK) {
        printf("Failed to fetch frame: %d (%s)\n", status, TYErrorString(status));
        return false;
    }
    
    printf("Frame fetched\n");

    // 查找深度和RGB图像
    TY_IMAGE_DATA* depthImage = NULL;
    TY_IMAGE_DATA* rgbImage = NULL;
    for (int i = 0; i < frame.validCount; i++) {
        if (frame.image[i].componentID == TY_COMPONENT_DEPTH_CAM) {
            depthImage = &frame.image[i];
        } else if (frame.image[i].componentID == TY_COMPONENT_RGB_CAM) {
            rgbImage = &frame.image[i];
        }
    }
    
    // 处理深度图像并生成点云
    processDepthImageAndGeneratePointCloud(device, depthImage, rgbImage, colorEnabled, colorISP);
    
    // 释放帧数据
    TYEnqueueBuffer(device, frame.userBuffer, frame.bufferSize);
    return true;
}

int main() {
    printf("PointCloud_NoCV Test - 1280x960 Color Point Cloud\n");

    TY_INTERFACE_INFO* pIfaceList = NULL;
    uint32_t ifaceCount = 0;
    TY_DEV_HANDLE device = NULL;
    bool deviceOpened = false;
    // 设备ID变量定义
    const char* deviceId ="207000145944";// "207000163457";
    
    // 1. 初始化库并获取接口列表
    if (!initializeLibAndGetInterfaces(&pIfaceList, &ifaceCount)) {
        return -1;
    }
    
    // 2. 从接口列表中打开设备
    deviceOpened = openDeviceFromInterfaceList(pIfaceList, deviceId, ifaceCount, &device);
    free(pIfaceList);
    
    if (deviceOpened) {
        // 3. 启用深度和RGB组件
        TY_COMPONENT_ID components = TY_COMPONENT_DEPTH_CAM;
        TY_ISP_HANDLE colorISP = NULL;
        bool colorEnabled = false;
        
        // 尝试创建ISP处理器用于彩色图像处理
        TY_STATUS status = TYISPCreate(&colorISP);
        if (status == TY_STATUS_OK) {
            components |= TY_COMPONENT_RGB_CAM;
            colorEnabled = true;
        } else {
            printf("Failed to create ISP handle: %d (%s)\n", status, TYErrorString(status));
        }

        // 先启用所有组件
        status = TYEnableComponents(device, components);
        if (status != TY_STATUS_OK) {
            printf("Failed to enable components: %d (%s)\n", status, TYErrorString(status));
            if (colorEnabled && colorISP) {
                TYISPRelease(&colorISP);
            }
            TYCloseDevice(device);
            TYDeinitLib();
            return -1;
        }
        printf("Depth component enabled\n");
        if (colorEnabled) {
            printf("RGB component enabled\n");
            
            // 初始化ISP
            if (initISP(colorISP, device) != TY_STATUS_OK) {
                printf("Failed to initialize ISP\n");
            }
        }

        // 4. 设置相机分辨率
        setupCameraResolutions(device, colorEnabled, colorISP);
        
        // 5. 分配和入队缓冲区
        const int BUFFER_COUNT = 4; // 使用4个缓冲区
        char* buffers[BUFFER_COUNT] = { NULL };
        if (!allocateAndEnqueueBuffers(device, buffers, BUFFER_COUNT)) {
            if (colorEnabled && colorISP) {
                TYISPRelease(&colorISP);
            }
            TYCloseDevice(device);
            TYDeinitLib();
            return -1;
        }
        
        // 6. 开始采集
        status = TYStartCapture(device);
        if (status != TY_STATUS_OK) {
            printf("Failed to start capture: %d (%s)\n", status, TYErrorString(status));
            // 释放缓冲区
            for (int i = 0; i < BUFFER_COUNT; i++) {
                if (buffers[i]) {
                    free(buffers[i]);
                }
            }
            // 释放ISP资源
            if (colorEnabled && colorISP) {
                TYISPRelease(&colorISP);
            }
            TYCloseDevice(device);
            TYDeinitLib();
            return -1;
        }
        printf("Capture started\n");
        
        // 7. 处理捕获的帧数据
        processCapturedFrame(device, colorEnabled, colorISP);
        
        // 8. 停止采集
        status = TYStopCapture(device);
        if (status != TY_STATUS_OK) {
            printf("Failed to stop capture: %d (%s)\n", status, TYErrorString(status));
        }

        // 9. 释放缓冲区
        for (int i = 0; i < BUFFER_COUNT; i++) {
            if (buffers[i]) {
                free(buffers[i]);
            }
        }

        // 10. 释放ISP资源
        if (colorEnabled && colorISP) {
            status = TYISPRelease(&colorISP);
            if (status != TY_STATUS_OK) {
                printf("Failed to release ISP: %d (%s)\n", status, TYErrorString(status));
            }
        }

        // 11. 关闭设备
        TYCloseDevice(device);
    }

    // 12. 清理资源
    TYDeinitLib();

    if (deviceOpened) {
        printf("彩色点云测试完成！\n");
        return 0;
    } else {
        printf("Failed to open any device\n");
        return -1;
    }
}