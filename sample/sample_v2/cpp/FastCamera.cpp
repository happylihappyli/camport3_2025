#include "../hpp/Device.hpp"
#include "../hpp/Frame.hpp"
#include "../../../include/TYApi.h"
#include "../../../include/TYDefs.h"

namespace percipio_layer {

FastCamera::FastCamera()
{
    // 构造函数实现
}

FastCamera::FastCamera(const char* sn)
{
    // 带参数构造函数实现
    if (sn) {
        mIfaceId = sn;
    }
}

FastCamera::~FastCamera()
{
    // 析构函数实现
    if (isRuning) {
        stop();
        close();
    }
}

TY_STATUS FastCamera::open(const char* sn)
{
    std::lock_guard<std::mutex> lock(_dev_lock);
    
    if (!sn) {
        return TY_STATUS_INVALID_PARAMETER;
    }
    
    // 获取设备列表
    auto& context = TYContext::getInstance();
    auto deviceList = context.queryDeviceList();
    
    if (!deviceList || deviceList->empty()) {
        return TY_STATUS_ERROR;
    }
    
    // 查找指定序列号的设备
    device = deviceList->getDeviceBySN(sn);
    if (!device) {
        return TY_STATUS_ERROR;
    }
    
    return TY_STATUS_OK;
}

TY_STATUS FastCamera::openByIP(const char* ip)
{
    std::lock_guard<std::mutex> lock(_dev_lock);
    
    if (!ip) {
        return TY_STATUS_INVALID_PARAMETER;
    }
    
    auto& context = TYContext::getInstance();
    auto deviceList = context.queryNetDeviceList();
    
    if (!deviceList || deviceList->empty()) {
        return TY_STATUS_ERROR;
    }
    
    device = deviceList->getDeviceByIP(ip);
    if (!device) {
        return TY_STATUS_ERROR;
    }
    
    return TY_STATUS_OK;
}

TY_STATUS FastCamera::openWithHandle(TY_DEV_HANDLE handle)
{
    std::lock_guard<std::mutex> lock(_dev_lock);
    
    if (!handle) {
        return TY_STATUS_INVALID_PARAMETER;
    }
    
    // 从设备句柄获取设备信息
    TY_DEVICE_BASE_INFO tyDevInfo;
    memset(&tyDevInfo, 0, sizeof(tyDevInfo));
    TY_STATUS status = TYGetDeviceInfo(handle, &tyDevInfo);
    if (status != TY_STATUS_OK) {
        return status;
    }
    
    // 转换为DeviceBaseInfo格式
    DeviceBaseInfo devInfo;
    memset(&devInfo, 0, sizeof(devInfo));
    strncpy(devInfo.id, tyDevInfo.id, sizeof(devInfo.id) - 1);
    strncpy(devInfo.name, tyDevInfo.vendorName, sizeof(devInfo.name) - 1);
    strncpy(devInfo.modelName, tyDevInfo.modelName, sizeof(devInfo.modelName) - 1);
    strncpy(devInfo.serialNumber, tyDevInfo.id, sizeof(devInfo.serialNumber) - 1);
    devInfo.nChipID = 0; // 没有对应的字段，使用默认值
    devInfo.iface = tyDevInfo.iface;
    devInfo.netInfo = tyDevInfo.netInfo;
    
    // 创建TYDevice实例
    device = std::shared_ptr<TYDevice>(new TYDevice(handle, devInfo));
    
    // 分配帧缓冲区
    bool color, ir, depth;
    color = ir = depth = false;
    
    // 获取组件信息
    TYHasFeature(handle, TY_COMPONENT_RGB_CAM, 0, &color);
    TYHasFeature(handle, TY_COMPONENT_IR_CAM_LEFT, 0, &ir);
    TYHasFeature(handle, TY_COMPONENT_DEPTH_CAM, 0, &depth);
    
    // 分配缓冲区
    const int buffer_size = 10;  // 分配10个缓冲区
    for (int i = 0; i < buffer_size; i++) {
        char* buffer = NULL;
        int size = 0;
        
        if (depth) {
            int w = 640, h = 480;  // 默认深度图尺寸
            TYGetInt(handle, TY_COMPONENT_DEPTH_CAM, TY_INT_WIDTH, &w);
            TYGetInt(handle, TY_COMPONENT_DEPTH_CAM, TY_INT_HEIGHT, &h);
            size = w * h * 2;  // 16位深度
        } else if (color) {
            int w = 640, h = 480;  // 默认彩色图尺寸
            TYGetInt(handle, TY_COMPONENT_RGB_CAM, TY_INT_WIDTH, &w);
            TYGetInt(handle, TY_COMPONENT_RGB_CAM, TY_INT_HEIGHT, &h);
            size = w * h * 3;  // RGB888
        } else if (ir) {
            int w = 640, h = 480;  // 默认IR图尺寸
            TYGetInt(handle, TY_COMPONENT_IR_CAM_LEFT, TY_INT_WIDTH, &w);
            TYGetInt(handle, TY_COMPONENT_IR_CAM_LEFT, TY_INT_HEIGHT, &h);
            size = w * h;  // 8位IR
        }
        
        if (size > 0) {
            buffer = (char*)malloc(size);
            if (buffer) {
                status = TYEnqueueBuffer(handle, buffer, size);
                if (status != TY_STATUS_OK) {
                    free(buffer);
                }
            }
        }
    }
    
    return TY_STATUS_OK;
}

bool FastCamera::has_stream(stream_idx idx)
{
    if (!device) {
        return false;
    }
    
    // 检查设备是否支持指定的流
    // 这里需要根据实际设备能力进行检查
    switch (idx) {
        case stream_depth:
            return true; // 假设支持深度流
        case stream_color:
            return true; // 假设支持彩色流
        case stream_ir:
            return true; // 假设支持IR流（默认）
        case stream_ir_left:
            return true; // 假设支持左侧IR流
        case stream_ir_right:
            return true; // 假设支持右侧IR流
        default:
            return false;
    }
}

TY_STATUS FastCamera::stream_enable(stream_idx idx)
{
    if (!device) {
        return TY_STATUS_INVALID_HANDLE;
    }
    
    TY_STATUS status = TY_STATUS_OK;
    
    switch (idx) {
        case stream_depth:
            status = TYEnableComponents(device->_handle, TY_COMPONENT_DEPTH_CAM);
            break;
        case stream_color:
            status = TYEnableComponents(device->_handle, TY_COMPONENT_RGB_CAM);
            break;
        case stream_ir_left:
            status = TYEnableComponents(device->_handle, TY_COMPONENT_IR_CAM_LEFT);
            break;
        case stream_ir_right:
            status = TYEnableComponents(device->_handle, TY_COMPONENT_IR_CAM_RIGHT);
            break;
        case stream_ir:
            // IR流同时启用左右摄像头
            status = TYEnableComponents(device->_handle, TY_COMPONENT_IR_CAM_LEFT);
            if (status == TY_STATUS_OK) {
                status = TYEnableComponents(device->_handle, TY_COMPONENT_IR_CAM_RIGHT);
            }
            break;
        default:
            return TY_STATUS_INVALID_PARAMETER;
    }
    
    if (status == TY_STATUS_OK) {
        components |= idx;
    }
    
    return status;
}

TY_STATUS FastCamera::stream_disable(stream_idx idx)
{
    if (!device) {
        return TY_STATUS_INVALID_HANDLE;
    }
    
    TY_STATUS status = TY_STATUS_OK;
    
    switch (idx) {
        case stream_depth:
            status = TYDisableComponents(device->_handle, TY_COMPONENT_DEPTH_CAM);
            break;
        case stream_color:
            status = TYDisableComponents(device->_handle, TY_COMPONENT_RGB_CAM);
            break;
        case stream_ir_left:
            status = TYDisableComponents(device->_handle, TY_COMPONENT_IR_CAM_LEFT);
            break;
        case stream_ir_right:
            status = TYDisableComponents(device->_handle, TY_COMPONENT_IR_CAM_RIGHT);
            break;
        case stream_ir:
            // IR流同时禁用左右摄像头
            status = TYDisableComponents(device->_handle, TY_COMPONENT_IR_CAM_LEFT);
            if (status == TY_STATUS_OK) {
                status = TYDisableComponents(device->_handle, TY_COMPONENT_IR_CAM_RIGHT);
            }
            break;
        default:
            return TY_STATUS_INVALID_PARAMETER;
    }
    
    if (status == TY_STATUS_OK) {
        components &= ~idx;
    }
    
    return status;
}

TY_STATUS FastCamera::start()
{
    if (!device) {
        return TY_STATUS_INVALID_HANDLE;
    }
    
    // 启动数据采集
    TY_STATUS status = TYStartCapture(device->_handle);
    if (status == TY_STATUS_OK) {
        isRuning = true;
    }
    return status;
}

TY_STATUS FastCamera::stop()
{
    if (!device || !isRuning) {
        return TY_STATUS_OK; // 已经停止或未启动
    }
    
    // 停止数据采集
    TY_STATUS status = doStop();
    if (status == TY_STATUS_OK) {
        isRuning = false;
    }
    return status;
}

TY_STATUS FastCamera::doStop()
{
    if (!device) {
        return TY_STATUS_INVALID_HANDLE;
    }
    
    return TYStopCapture(device->_handle);
}

void FastCamera::close()
{
    std::lock_guard<std::mutex> lock(_dev_lock);
    
    if (isRuning) {
        stop();
    }
    
    if (device) {
        device.reset();
    }
    
    components = 0;
    isRuning = false;
}

std::shared_ptr<TYFrame> FastCamera::tryGetFrames(uint32_t timeout_ms)
{
    if (!device || !isRuning) {
        return nullptr;
    }
    
    return fetchFrames(timeout_ms);
}

std::shared_ptr<TYFrame> FastCamera::fetchFrames(uint32_t timeout_ms)
{
    if (!device) {
        return nullptr;
    }
    
    // 调用TY SDK API获取实际帧数据
    TY_FRAME_DATA frameData;
    int ret = TYFetchFrame(device->_handle, &frameData, timeout_ms);
    
    if (ret != TY_STATUS_OK) {
        // 获取帧失败
        return nullptr;
    }
    
    // 创建共享的帧对象
    auto frame = std::make_shared<TYFrame>(frameData);
    
    // 重要：将缓冲区重新入队以供下次使用
    if (frameData.userBuffer && frameData.bufferSize > 0) {
        TYEnqueueBuffer(device->_handle, frameData.userBuffer, frameData.bufferSize);
    }
    
    return frame;
}

TY_STATUS FastCamera::setIfaceId(const char* inf)
{
    if (inf) {
        mIfaceId = inf;
        return TY_STATUS_OK;
    }
    return TY_STATUS_INVALID_PARAMETER;
}

}