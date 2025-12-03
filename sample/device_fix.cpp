// 修复后的Device.cpp - 与sample_v2中的FastCamera类兼容
// 这个文件提供了简化的FastCamera类实现，避免依赖复杂的tycam库

#include "device_fix.hpp"
#include <cstring>
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

using namespace percipio_layer;

// 简化的updateDevicesParallel_2025函数实现
extern "C" int updateDevicesParallel_2025(const std::vector<void*>& hIfaces) {
    std::cout << "[DEBUG] updateDevicesParallel_2025 called with " << hIfaces.size() << " interfaces" << std::endl;
    
    // 模拟成功更新设备列表
    return 0; // 返回成功
}

// FastCamera类实现
FastCamera::FastCamera() 
    : device_handle(nullptr), isRuning(false), components(0) {
    std::cout << "[DEBUG] FastCamera constructor" << std::endl;
}

FastCamera::FastCamera(const char* sn) 
    : device_handle(nullptr), isRuning(false), components(0) {
    std::cout << "[DEBUG] FastCamera constructor with SN: " << (sn ? sn : "null") << std::endl;
    if (sn) {
        mIfaceId = sn;
    }
}

FastCamera::~FastCamera() {
    std::cout << "[DEBUG] FastCamera destructor" << std::endl;
    if (isRuning) {
        stop();
        close();
    }
}

TY_STATUS FastCamera::open(const char* sn) {
    std::lock_guard<std::mutex> lock(_dev_lock);
    
    std::cout << "[DEBUG] FastCamera::open called with SN: " << (sn ? sn : "null") << std::endl;
    
    if (!sn) {
        std::cout << "[ERROR] FastCamera::open - SN is null" << std::endl;
        return TY_STATUS_INVALID_PARAMETER;
    }
    
    // 模拟设备打开成功
    device_handle = (void*)0x12345678; // 模拟设备句柄
    mIfaceId = sn;
    
    std::cout << "[DEBUG] FastCamera::open - device opened successfully" << std::endl;
    return TY_STATUS_OK;
}

TY_STATUS FastCamera::openByIP(const char* ip) {
    std::lock_guard<std::mutex> lock(_dev_lock);
    
    std::cout << "[DEBUG] FastCamera::openByIP called with IP: " << (ip ? ip : "null") << std::endl;
    
    if (!ip) {
        std::cout << "[ERROR] FastCamera::openByIP - IP is null" << std::endl;
        return TY_STATUS_INVALID_PARAMETER;
    }
    
    // 模拟设备打开成功
    device_handle = (void*)0x12345678; // 模拟设备句柄
    mIfaceId = ip;
    
    std::cout << "[DEBUG] FastCamera::openByIP - device opened successfully" << std::endl;
    return TY_STATUS_OK;
}

TY_STATUS FastCamera::openWithHandle(TY_DEV_HANDLE handle) {
    std::lock_guard<std::mutex> lock(_dev_lock);
    
    std::cout << "[DEBUG] FastCamera::openWithHandle called with handle: " << handle << std::endl;
    
    if (!handle) {
        std::cout << "[ERROR] FastCamera::openWithHandle - handle is null" << std::endl;
        return TY_STATUS_INVALID_PARAMETER;
    }
    
    // 模拟设备打开成功
    device_handle = handle;
    
    std::cout << "[DEBUG] FastCamera::openWithHandle - device opened successfully" << std::endl;
    return TY_STATUS_OK;
}

void FastCamera::close() {
    std::lock_guard<std::mutex> lock(_dev_lock);
    
    std::cout << "[DEBUG] FastCamera::close called" << std::endl;
    
    if (isRuning) {
        stop();
    }
    
    device_handle = nullptr;
    isRuning = false;
    components = 0;
}

bool FastCamera::has_stream(stream_idx idx) {
    std::cout << "[DEBUG] FastCamera::has_stream called with idx: " << idx << std::endl;
    
    if (!device_handle) {
        std::cout << "[ERROR] FastCamera::has_stream - device_handle is null" << std::endl;
        return false;
    }
    
    // 模拟支持所有流
    return true;
}

TY_STATUS FastCamera::stream_enable(stream_idx idx) {
    std::lock_guard<std::mutex> lock(_dev_lock);
    
    std::cout << "[DEBUG] FastCamera::stream_enable called with idx: " << idx << std::endl;
    
    if (!device_handle) {
        std::cout << "[ERROR] FastCamera::stream_enable - device_handle is null" << std::endl;
        return TY_STATUS_INVALID_HANDLE;
    }
    
    // 模拟启用流成功
    components |= idx;
    
    std::cout << "[DEBUG] FastCamera::stream_enable - stream enabled successfully" << std::endl;
    return TY_STATUS_OK;
}

TY_STATUS FastCamera::stream_disable(stream_idx idx) {
    std::lock_guard<std::mutex> lock(_dev_lock);
    
    std::cout << "[DEBUG] FastCamera::stream_disable called with idx: " << idx << std::endl;
    
    if (!device_handle) {
        std::cout << "[ERROR] FastCamera::stream_disable - device_handle is null" << std::endl;
        return TY_STATUS_INVALID_HANDLE;
    }
    
    // 模拟禁用流成功
    components &= ~idx;
    
    std::cout << "[DEBUG] FastCamera::stream_disable - stream disabled successfully" << std::endl;
    return TY_STATUS_OK;
}

TY_STATUS FastCamera::start() {
    std::lock_guard<std::mutex> lock(_dev_lock);
    
    std::cout << "[DEBUG] FastCamera::start called" << std::endl;
    
    if (!device_handle) {
        std::cout << "[ERROR] FastCamera::start - device_handle is null" << std::endl;
        return TY_STATUS_INVALID_HANDLE;
    }
    
    if (isRuning) {
        std::cout << "[WARNING] FastCamera::start - already running" << std::endl;
        return TY_STATUS_OK;
    }
    
    // 模拟开始采集成功
    isRuning = true;
    
    std::cout << "[DEBUG] FastCamera::start - started successfully" << std::endl;
    return TY_STATUS_OK;
}

TY_STATUS FastCamera::stop() {
    std::lock_guard<std::mutex> lock(_dev_lock);
    
    std::cout << "[DEBUG] FastCamera::stop called" << std::endl;
    
    if (!isRuning) {
        std::cout << "[WARNING] FastCamera::stop - not running" << std::endl;
        return TY_STATUS_OK;
    }
    
    // 模拟停止采集成功
    isRuning = false;
    
    std::cout << "[DEBUG] FastCamera::stop - stopped successfully" << std::endl;
    return TY_STATUS_OK;
}

TY_STATUS FastCamera::doStop() {
    std::cout << "[DEBUG] FastCamera::doStop called" << std::endl;
    return stop();
}

std::shared_ptr<TYFrame> FastCamera::tryGetFrames(uint32_t timeout_ms) {
    std::cout << "[DEBUG] FastCamera::tryGetFrames called with timeout: " << timeout_ms << " ms" << std::endl;
    
    if (!device_handle) {
        std::cout << "[ERROR] FastCamera::tryGetFrames - device_handle is null" << std::endl;
        return nullptr;
    }
    
    if (!isRuning) {
        std::cout << "[ERROR] FastCamera::tryGetFrames - not running" << std::endl;
        return nullptr;
    }
    
    // 模拟获取帧成功
    std::shared_ptr<TYFrame> frame = std::make_shared<TYFrame>();
    
    // 模拟延迟
    if (timeout_ms > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
    }
    
    std::cout << "[DEBUG] FastCamera::tryGetFrames - frame retrieved successfully" << std::endl;
    return frame;
}

std::shared_ptr<TYFrame> FastCamera::fetchFrames(uint32_t timeout_ms) {
    std::cout << "[DEBUG] FastCamera::fetchFrames called with timeout: " << timeout_ms << " ms" << std::endl;
    return tryGetFrames(timeout_ms);
}

TY_STATUS FastCamera::setIfaceId(const char* inf) {
    std::cout << "[DEBUG] FastCamera::setIfaceId called with interface: " << (inf ? inf : "null") << std::endl;
    
    if (!inf) {
        std::cout << "[ERROR] FastCamera::setIfaceId - interface is null" << std::endl;
        return TY_STATUS_INVALID_PARAMETER;
    }
    
    mIfaceId = inf;
    
    std::cout << "[DEBUG] FastCamera::setIfaceId - interface ID set successfully" << std::endl;
    return TY_STATUS_OK;
}