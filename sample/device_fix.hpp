// 修复后的Device.hpp - 与sample_v2中的FastCamera类兼容
// 这个文件提供了简化的FastCamera类实现，避免依赖复杂的tycam库

#pragma once

#include <string>
#include <vector>
#include <cstring>
#include <iostream>
#include <memory>
#include <mutex>

// 模拟TY_STATUS枚举
enum TY_STATUS {
    TY_STATUS_OK = 0,
    TY_STATUS_ERROR = -1,
    TY_STATUS_INVALID_PARAMETER = -2,
    TY_STATUS_INVALID_HANDLE = -3
};

// 模拟TY_DEV_HANDLE类型
typedef void* TY_DEV_HANDLE;

namespace percipio_layer {

// 模拟TYFrame类
class TYFrame {
public:
    TYFrame() {}
    virtual ~TYFrame() {}
};

// FastCamera类 - 与sample_v2中的类兼容
class __declspec(dllexport) FastCamera {
public:
    // 流索引枚举
    enum stream_idx : uint32_t {
        stream_depth = 0x01,
        stream_color = 0x02,
        stream_ir_left = 0x04,
        stream_ir_right = 0x08,
        stream_ir = stream_ir_left | stream_ir_right,
    };
    
    FastCamera();
    FastCamera(const char* sn);
    virtual ~FastCamera();
    
    // 设备操作
    TY_STATUS open(const char* sn);
    TY_STATUS openByIP(const char* ip);
    TY_STATUS openWithHandle(TY_DEV_HANDLE handle);
    void close();
    
    // 流操作
    bool has_stream(stream_idx idx);
    TY_STATUS stream_enable(stream_idx idx);
    TY_STATUS stream_disable(stream_idx idx);
    
    // 采集控制
    TY_STATUS start();
    TY_STATUS stop();
    
    // 帧获取
    std::shared_ptr<TYFrame> tryGetFrames(uint32_t timeout_ms = 1000);
    
    // 设备句柄访问
    void* handle() { return device_handle; }
    
    // 设置接口ID
    TY_STATUS setIfaceId(const char* inf);
    
protected:
    TY_STATUS doStop();
    std::shared_ptr<TYFrame> fetchFrames(uint32_t timeout_ms);
    
protected:
    void* device_handle;
    std::mutex _dev_lock;
    bool isRuning;
    uint32_t components; // 已启用的组件位图
    std::string mIfaceId;
};

} // namespace percipio_layer

// 前向声明update函数
extern "C" int updateDevicesParallel_2025(const std::vector<void*>& hIfaces);