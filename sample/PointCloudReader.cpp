#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include "TYApi.h"
#include "TYCoordinateMapper.h"

// 点云读取类
class PointCloudReader {
private:
    TY_DEV_HANDLE m_deviceHandle;          // 设备句柄
    bool m_isOpened;                       // 设备是否已打开
    bool m_isCapturing;                    // 是否正在捕获数据
    TY_CAMERA_CALIB_INFO m_calibInfo;      // 相机标定信息
    std::vector<uint8_t> m_buffer;         // 数据缓冲区
    float m_depthScale;                    // 深度缩放因子

    // 释放资源
    void release() {
        if (m_isCapturing) {
            TYStopCapture(m_deviceHandle);
            m_isCapturing = false;
        }
        
        if (m_isOpened) {
            TYCloseDevice(m_deviceHandle);
            m_isOpened = false;
        }
    }

public:
    // 构造函数
    PointCloudReader() : m_isOpened(false), m_isCapturing(false), m_depthScale(1.0f) {
        // 初始化SDK
        TYInitLib();
    }

    // 析构函数
    ~PointCloudReader() {
        release();
        TYDeinitLib();
    }

    // 打开设备
    bool open(const std::string& deviceID) {
        // 关闭之前可能打开的设备
        release();

        // 打开设备
        TY_STATUS status = TYOpenDevice(deviceID.c_str(), &m_deviceHandle);
        if (status != TY_STATUS_OK) {
            std::cerr << "无法打开设备: " << deviceID << "，错误码: " << status << std::endl;
            return false;
        }

        m_isOpened = true;
        std::cout << "成功打开设备: " << deviceID << std::endl;

        // 获取设备标定信息
        status = TYGetStruct(m_deviceHandle, TY_COMPONENT_DEPTH_CAM, TY_STRUCT_CAM_CALIB_DATA, &m_calibInfo, sizeof(m_calibInfo));
        if (status != TY_STATUS_OK) {
            std::cerr << "无法获取标定信息，错误码: " << status << std::endl;
            release();
            return false;
        }

        // 获取深度缩放因子
        int32_t depthScaleValue = 0;
        status = TYGetInt(m_deviceHandle, TY_COMPONENT_DEPTH_CAM, TY_INT_DEPTH_SCALE, &depthScaleValue);
        if (status == TY_STATUS_OK) {
            m_depthScale = 1.0f / depthScaleValue;
        }
        
        std::cout << "深度缩放因子: " << m_depthScale << std::endl;

        // 启用深度组件
        status = TYEnableComponents(m_deviceHandle, TY_COMPONENT_DEPTH_CAM);
        if (status != TY_STATUS_OK) {
            std::cerr << "无法启用深度组件，错误码: " << status << std::endl;
            release();
            return false;
        }

        // 获取帧缓冲区大小
        uint32_t frameSize = 0;
        status = TYGetFrameBufferSize(m_deviceHandle, &frameSize);
        if (status != TY_STATUS_OK) {
            std::cerr << "无法获取帧缓冲区大小，错误码: " << status << std::endl;
            release();
            return false;
        }

        // 分配缓冲区
        m_buffer.resize(frameSize);
        
        // 将缓冲区加入队列
        status = TYEnqueueBuffer(m_deviceHandle, m_buffer.data(), frameSize);
        if (status != TY_STATUS_OK) {
            std::cerr << "无法加入缓冲区，错误码: " << status << std::endl;
            release();
            return false;
        }

        return true;
    }

    // 开始捕获数据
    bool startCapture() {
        if (!m_isOpened || m_isCapturing) {
            return false;
        }

        TY_STATUS status = TYStartCapture(m_deviceHandle);
        if (status != TY_STATUS_OK) {
            std::cerr << "无法开始捕获数据，错误码: " << status << std::endl;
            return false;
        }

        m_isCapturing = true;
        std::cout << "开始捕获数据" << std::endl;
        return true;
    }

    // 停止捕获数据
    bool stopCapture() {
        if (!m_isCapturing) {
            return false;
        }

        TY_STATUS status = TYStopCapture(m_deviceHandle);
        if (status != TY_STATUS_OK) {
            std::cerr << "无法停止捕获数据，错误码: " << status << std::endl;
            return false;
        }

        m_isCapturing = false;
        std::cout << "停止捕获数据" << std::endl;
        return true;
    }

    // 获取一帧点云数据
    bool getPointCloud(std::vector<TY_VECT_3F>& pointCloud) {
        if (!m_isCapturing) {
            std::cerr << "未开始捕获数据" << std::endl;
            return false;
        }

        TY_FRAME_DATA frame;
        TY_STATUS status = TYFetchFrame(m_deviceHandle, &frame, 1000); // 1000ms超时
        if (status != TY_STATUS_OK) {
            std::cerr << "获取帧数据失败，错误码: " << status << std::endl;
            return false;
        }

        // 检查是否有深度数据
        if (!(frame.validMask & TY_COMPONENT_DEPTH_CAM)) {
            std::cerr << "没有有效的深度数据" << std::endl;
            TYEnqueueBuffer(m_deviceHandle, frame.userBuffer, frame.bufferSize);
            return false;
        }

        // 获取深度图像信息
        TY_IMAGE_DATA* depthImage = (TY_IMAGE_DATA*)frame.image;
        int width = depthImage->width;
        int height = depthImage->height;
        uint16_t* depthData = (uint16_t*)depthImage->buffer;

        // 调整点云数组大小
        pointCloud.resize(width * height);

        // 将深度图像转换为点云
        status = TYMapDepthImageToPoint3d(&m_calibInfo, width, height, depthData, pointCloud.data(), m_depthScale);
        if (status != TY_STATUS_OK) {
            std::cerr << "转换深度图像到点云失败，错误码: " << status << std::endl;
            TYEnqueueBuffer(m_deviceHandle, frame.userBuffer, frame.bufferSize);
            return false;
        }

        // 将缓冲区重新加入队列
        TYEnqueueBuffer(m_deviceHandle, frame.userBuffer, frame.bufferSize);
        
        std::cout << "成功获取点云数据，点数量: " << pointCloud.size() << std::endl;
        return true;
    }

    // 保存点云为PLY文件
    bool savePointCloudToPLY(const std::vector<TY_VECT_3F>& pointCloud, const std::string& filename) {
        if (pointCloud.empty()) {
            std::cerr << "点云数据为空" << std::endl;
            return false;
        }

        // 过滤掉无效点（NaN值）
        int validPointCount = 0;
        for (const auto& point : pointCloud) {
            if (!isnan(point.x) && !isnan(point.y) && !isnan(point.z)) {
                validPointCount++;
            }
        }

        FILE* fp = fopen(filename.c_str(), "wb");
        if (!fp) {
            std::cerr << "无法创建文件: " << filename << std::endl;
            return false;
        }

        // 写入PLY文件头
        fprintf(fp, "ply\n");
        fprintf(fp, "format ascii 1.0\n");
        fprintf(fp, "element vertex %d\n", validPointCount);
        fprintf(fp, "property float x\n");
        fprintf(fp, "property float y\n");
        fprintf(fp, "property float z\n");
        fprintf(fp, "end_header\n");

        // 写入点云数据
        for (const auto& point : pointCloud) {
            if (!isnan(point.x) && !isnan(point.y) && !isnan(point.z)) {
                fprintf(fp, "%f %f %f\n", point.x, point.y, point.z);
            }
        }

        fclose(fp);
        std::cout << "成功保存点云数据到文件: " << filename << "，有效点数量: " << validPointCount << std::endl;
        return true;
    }
};

// 主函数示例
int main() {
    PointCloudReader reader;
    
    // 打开设备（需要替换为实际的设备ID）
    std::string deviceID = "USB_DEVICE_ID";
    if (!reader.open(deviceID)) {
        std::cerr << "程序退出" << std::endl;
        return -1;
    }

    // 开始捕获数据
    if (!reader.startCapture()) {
        std::cerr << "程序退出" << std::endl;
        return -1;
    }

    // 获取点云数据
    std::vector<TY_VECT_3F> pointCloud;
    if (reader.getPointCloud(pointCloud)) {
        // 保存点云到PLY文件
        reader.savePointCloudToPLY(pointCloud, "point_cloud.ply");
    }

    // 停止捕获并关闭设备
    reader.stopCapture();
    
    std::cout << "程序执行完毕" << std::endl;
    return 0;
}