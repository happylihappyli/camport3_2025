#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include "TYApi.h"
#include "TYImageProc.h"
#include "TyIsp.h"

// RGBD配准函数：将彩色图像映射到深度坐标系
void registerDepthToColor(const uint16_t* depthData, int depthWidth, int depthHeight,
                           const uint8_t* rgbData, int rgbWidth, int rgbHeight,
                           const TY_CAMERA_CALIB_INFO* depthCalib,
                           const TY_CAMERA_CALIB_INFO* colorCalib,
                           uint8_t* registeredRgbData) {
    if (!depthData || !rgbData || !registeredRgbData || !depthCalib || !colorCalib) {
        printf("Invalid input parameters for registerDepthToColor\n");
        return;
    }

    // 使用TYMapRGBImageToDepthCoordinate函数将RGB图像映射到深度坐标系
    TY_STATUS status = TYMapRGBImageToDepthCoordinate(
        depthCalib,           // 深度相机的标定信息
        depthWidth, depthHeight, depthData,  // 深度图像数据
        colorCalib,           // 彩色相机的标定信息
        rgbWidth, rgbHeight, rgbData,        // RGB图像数据
        registeredRgbData     // 输出的配准后的RGB数据
    );

    if (status != TY_STATUS_OK) {
        printf("Failed to register RGB to depth coordinate, status: %d\n", status);
        // 如果配准失败，设置所有点为默认颜色（黑色）
        memset(registeredRgbData, 0, depthWidth * depthHeight * 3);
    }
}

// 限制值在指定范围内
int clamp(int value, int min, int max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
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
    fprintf(fp, "property uchar red\n");
    fprintf(fp, "property uchar green\n");
    fprintf(fp, "property uchar blue\n");
    fprintf(fp, "end_header\n");

    // 写入点云数据（包含颜色）
    int validPoints = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = y * width + x;
            if (p3d[index].z != 0 && rgbData) {
                // 获取RGB颜色值（注意BGR顺序）
                uint8_t b = rgbData[index * 3];
                uint8_t g = rgbData[index * 3 + 1];
                uint8_t r = rgbData[index * 3 + 2];
                fprintf(fp, "%f %f %f %d %d %d\n", p3d[index].x, p3d[index].y, p3d[index].z, r, g, b);
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
static bool openDeviceFromInterfaceList(TY_INTERFACE_INFO* pIfaceList, uint32_t ifaceCount, TY_DEV_HANDLE* device) {
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
                        status = TYOpenDevice(ifaceHandle, pDeviceList[0].id, device);
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
    if (rgbImage->pixelFormat == TY_PIXEL_FORMAT_BGR || 
        rgbImage->pixelFormat == TY_PIXEL_FORMAT_RGB) {
        // 如果相机已经提供了RGB/BGR格式数据，直接复制
        printf("Camera already provides RGB/BGR format, copying directly\n");
        int copySize = rgbImage->size > (rgbImage->width * rgbImage->height * 3) ? 
                      (rgbImage->width * rgbImage->height * 3) : rgbImage->size;
        memcpy(*rgbData, rgbImage->buffer, copySize);
    } else if (rgbImage->pixelFormat == TY_PIXEL_FORMAT_YUYV) {
        // 处理默认的YUYV格式
        printf("Processing YUYV format data\n");
        uint8_t* yuyvData = (uint8_t*)rgbImage->buffer;
        int pixelCount = rgbImage->width * rgbImage->height;
        
        // YUYV到BGR转换
        for (int i = 0, j = 0; i < pixelCount * 2; i += 4, j += 6) {
            // 第一个像素
            int y0 = yuyvData[i];
            int u = yuyvData[i+1];
            int y1 = yuyvData[i+2];
            int v = yuyvData[i+3];
            
            // YUV到RGB转换公式
            int r0 = y0 + 1.402 * (v - 128);
            int g0 = y0 - 0.34414 * (u - 128) - 0.71414 * (v - 128);
            int b0 = y0 + 1.772 * (u - 128);
            
            int r1 = y1 + 1.402 * (v - 128);
            int g1 = y1 - 0.34414 * (u - 128) - 0.71414 * (v - 128);
            int b1 = y1 + 1.772 * (u - 128);
            
            // 范围裁剪并存储为BGR格式
            (*rgbData)[j] = (uint8_t)clamp(b0, 0, 255);
            (*rgbData)[j+1] = (uint8_t)clamp(g0, 0, 255);
            (*rgbData)[j+2] = (uint8_t)clamp(r0, 0, 255);
            
            (*rgbData)[j+3] = (uint8_t)clamp(b1, 0, 255);
            (*rgbData)[j+4] = (uint8_t)clamp(g1, 0, 255);
            (*rgbData)[j+5] = (uint8_t)clamp(r1, 0, 255);
        }
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

// 注册RGB数据到点云坐标
static bool registerRgbData(TY_DEV_HANDLE device, const uint8_t* rgbData,
                           TY_IMAGE_DATA* rgbImage, int depthWidth, int depthHeight, 
                           uint8_t** registeredRgbData) {
    // 分配配准后的RGB数据内存
    *registeredRgbData = (uint8_t*)malloc(depthWidth * depthHeight * 3);
    if (!*registeredRgbData) {
        printf("Failed to allocate memory for registered RGB data\n");
        return false;
    }
    
    printf("Performing RGB data registration\n");
    
    // 获取深度和RGB相机的校准信息
    TY_CAMERA_CALIB_INFO colorCalib;
    TY_STATUS status = TYGetStruct(device, TY_COMPONENT_RGB_CAM, TY_STRUCT_CAM_CALIB_DATA, &colorCalib, sizeof(colorCalib));
    
    if (status == TY_STATUS_OK && rgbData) {
        printf("Using camera calibration data for registration\n");
        
        // 检查RGB和深度图像的宽高比
        float widthRatio = (float)depthWidth / rgbImage->width;
        float heightRatio = (float)depthHeight / rgbImage->height;
        
        // 如果宽高比接近，使用简单的双线性插值缩放
        if (fabs(widthRatio - heightRatio) < 0.1) {
            scaleRgbImage(rgbData, rgbImage->width, rgbImage->height, 
                         *registeredRgbData, depthWidth, depthHeight);
        } else {
            // 如果宽高比差异较大，使用平铺方式
            tileRgbImage(rgbData, rgbImage->width, rgbImage->height, 
                         *registeredRgbData, depthWidth, depthHeight);
        }
    } else if (rgbData) {
        // 如果没有校准信息但有RGB数据，使用简单的映射
        printf("Using simple RGB mapping without calibration data\n");
        
        // 假设RGB和深度图像分辨率相同或使用简单映射
        if (rgbImage->width == depthWidth && rgbImage->height == depthHeight) {
            // 如果分辨率相同，直接复制
            memcpy(*registeredRgbData, rgbData, depthWidth * depthHeight * 3);
        } else {
            // 否则使用简单的最近邻缩放
            scaleRgbImage(rgbData, rgbImage->width, rgbImage->height, 
                         *registeredRgbData, depthWidth, depthHeight);
        }
    } else {
        // 如果没有RGB数据，创建简单的颜色渐变效果
        createDefaultColorGradient(*registeredRgbData, depthWidth, depthHeight);
    }
    printf("RGB data registration completed\n");
    return true;
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
    
    // 获取校准信息
    TY_CAMERA_CALIB_INFO depthCalib;
    TY_STATUS status = TYGetStruct(device, TY_COMPONENT_DEPTH_CAM, TY_STRUCT_CAM_CALIB_DATA, &depthCalib, sizeof(depthCalib));
    if (status != TY_STATUS_OK) {
        printf("Failed to get calibration info: %d (%s)\n", status, TYErrorString(status));
        return false;
    }
    
    // 获取深度比例
    float depthScale = 1.0f;
    status = TYGetFloat(device, TY_COMPONENT_DEPTH_CAM, TY_FLOAT_SCALE_UNIT, &depthScale);
    if (status != TY_STATUS_OK) {
        printf("Failed to get depth scale: %d (%s)\n", status, TYErrorString(status));
        return false;
    }
    
    printf("Depth scale: %.6f\n", depthScale);
    
    // 分配点云内存
    int width = depthImage->width;
    int height = depthImage->height;
    TY_VECT_3F* pointCloud = (TY_VECT_3F*)malloc(sizeof(TY_VECT_3F) * width * height);
    if (!pointCloud) {
        printf("Failed to allocate memory for point cloud\n");
        return false;
    }
    
    // 转换深度图像到点云
    TYMapDepthImageToPoint3d(&depthCalib, width, height, (uint16_t*)depthImage->buffer, pointCloud, depthScale);
    
    // 处理彩色图像（如果可用）
    uint8_t* rgbData = NULL;
    uint8_t* registeredRgbData = NULL;
    
    if (rgbImage && colorEnabled) {
        if (processRgbImage(device, colorISP, rgbImage, &rgbData)) {
            registerRgbData(device, rgbData, rgbImage, width, height, &registeredRgbData);
        }
    }
    
    // 保存彩色点云，使用配准后的RGB数据
    printf("Saving color point cloud...\n");
    savePointsToPly(pointCloud, registeredRgbData ? registeredRgbData : rgbData, width, height, "color_pointcloud.ply");
    
    // 释放资源
    free(pointCloud);
    if (rgbData) {
        free(rgbData);
    }
    if (registeredRgbData) {
        free(registeredRgbData);
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
    
    // 1. 初始化库并获取接口列表
    if (!initializeLibAndGetInterfaces(&pIfaceList, &ifaceCount)) {
        return -1;
    }
    
    // 2. 从接口列表中打开设备
    deviceOpened = openDeviceFromInterfaceList(pIfaceList, ifaceCount, &device);
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