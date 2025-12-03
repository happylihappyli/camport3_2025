#ifndef DEVICE_HPP
#define DEVICE_HPP

#include <vector>
#include <string>
#include <memory>
#include <map>
#include <mutex>
#include <set>
#include <functional>

// 包含Frame.hpp以获取TYFrame定义
#include "Frame.hpp"

// SDK类型前向声明 - 避免重复包含
#ifndef TY_SDK_TYPES_DEFINED
struct TY_EVENT_INFO;
struct TY_INTERFACE_INFO;
struct TY_DEVICE_NET_INFO;
typedef void* TY_DEV_HANDLE;
typedef void* TY_INTERFACE_HANDLE;
typedef void* TY_STREAM_HANDLE;
typedef int32_t TY_EVENT;    // 与SDK保持一致
// TY_STATUS 已在 common.hpp 中定义，无需重复定义
#define TY_SDK_TYPES_DEFINED
#endif

// 注意：以下结构体已在TYApi.h中定义，不需要重复定义
// struct TY_INTERFACE_INFO {
//     char id[64];
//     char name[64];
//     int type;
// };

// struct TY_DEVICE_NET_INFO {
//     char mac[32];
//     char ip[32];
//     char netmask[32];
//     char gateway[32];
//     char broadcast[32];
// };

// SDK常量定义 - 已在 common.hpp 中定义，无需重复定义
#ifndef TY_STATUS_CODES_DEFINED
#define TY_STATUS_CODES_DEFINED
#endif

// SDK接口类型常量 - 已在 common.hpp 中定义，无需重复定义
#ifndef TY_INTERFACE_CONSTANTS_DEFINED
#define TY_INTERFACE_CONSTANTS_DEFINED
#endif

// SDK函数前向声明（简化版本）- 已在 common.hpp 中声明，无需重复声明
#ifndef USE_FULL_SDK_HEADERS
extern "C" {
    // 注意：以下函数已在TYApi.h中定义，不需要重复声明
    // const char* TYErrorString(int32_t status);
    // int32_t TYUpdateInterfaceList(void);
    // 注意：以下函数已在TYApi.h中定义，不需要重复声明
    // int32_t TYGetInterfaceNumber(uint32_t* number);
    // int32_t TYGetInterfaceList(TY_INTERFACE_INFO* list, uint32_t size, uint32_t* actual);
    // 注意：以下函数已在TYApi.h中定义，不需要重复声明
    // int32_t TYOpenInterface(const char* id, TY_INTERFACE_HANDLE* handle);
    // int32_t TYCloseInterface(TY_INTERFACE_HANDLE handle);
    // 注意：以下函数已在TYApi.h中定义，不需要重复声明
    // int32_t TYGetDeviceNumber(TY_INTERFACE_HANDLE handle, uint32_t* number);
    // 注意：以下函数已在TYApi.h中定义，不需要重复声明
    // int32_t TYGetDeviceList(TY_INTERFACE_HANDLE handle, void* list, uint32_t size, uint32_t* actual);
    
    // 注意：以下函数已在TYApi.h中定义，不需要重复声明
    // int32_t TYUpdateDeviceList();
    // int32_t TYOpenDevice(const char* id, TY_DEV_HANDLE* handle);
    // int32_t TYCloseDevice(TY_DEV_HANDLE handle);
    // 注意：以下函数已在TYApi.h中定义，不需要重复声明
    // int32_t TYGetDeviceInfo(TY_DEV_HANDLE handle, void* info);
    // int32_t TYIsNetworkInterface(int32_t type);
    // 注意：以下函数已在TYApi.h中定义，不需要重复声明
    // int32_t TYRegisterEventCallback(TY_DEV_HANDLE handle, void (*callback)(TY_EVENT_INFO*, void*), void* userdata);
    // int32_t TYUnregisterEventCallback(TY_DEV_HANDLE handle);
    // 注意：以下函数已在TYApi.h中定义，不需要重复声明
    // int32_t TYEnableComponents(TY_DEV_HANDLE handle, TY_COMPONENT_ID components);
    // int32_t TYDisableComponents(TY_DEV_HANDLE handle, TY_COMPONENT_ID components);
    // int32_t TYGetComponentIDs(TY_DEV_HANDLE handle, TY_COMPONENT_ID* components);
    // 注意：以下函数已在TYApi.h中定义，不需要重复声明
    // int32_t TYSetEnum(TY_DEV_HANDLE handle, TY_COMPONENT_ID component, TY_FEATURE_ID feature, int32_t value);
    // int32_t TYGetEnum(TY_DEV_HANDLE handle, TY_COMPONENT_ID component, TY_FEATURE_ID feature, int32_t* value);
    // int32_t TYSetInt(TY_DEV_HANDLE handle, TY_COMPONENT_ID component, TY_FEATURE_ID feature, int32_t value);
    // int32_t TYGetInt(TY_DEV_HANDLE handle, TY_COMPONENT_ID component, TY_FEATURE_ID feature, int32_t* value);
    // int32_t TYSetFloat(TY_DEV_HANDLE handle, TY_COMPONENT_ID component, TY_FEATURE_ID feature, float value);
    // int32_t TYGetFloat(TY_DEV_HANDLE handle, TY_COMPONENT_ID component, TY_FEATURE_ID feature, float* value);
    // int32_t TYSetBool(TY_DEV_HANDLE handle, TY_COMPONENT_ID component, TY_FEATURE_ID feature, bool value);
    // int32_t TYGetBool(TY_DEV_HANDLE handle, TY_COMPONENT_ID component, TY_FEATURE_ID feature, bool* value);
    // int32_t TYSetString(TY_DEV_HANDLE handle, TY_COMPONENT_ID component, TY_FEATURE_ID feature, const char* value);
    // int32_t TYGetString(TY_DEV_HANDLE handle, TY_COMPONENT_ID component, TY_FEATURE_ID feature, char* buffer, uint32_t bufferSize);
    // 注意：以下函数已在TYApi.h中定义，不需要重复声明
    // int32_t TYSetStruct(TY_DEV_HANDLE handle, TY_COMPONENT_ID component, TY_FEATURE_ID feature, void* buffer, uint32_t bufferSize);
    // int32_t TYGetStruct(TY_DEV_HANDLE handle, TY_COMPONENT_ID component, TY_FEATURE_ID feature, void* buffer, uint32_t bufferSize);
    // 注意：以下函数已在TYApi.h中定义，不需要重复声明
    // int32_t TYStartCapture(TY_DEV_HANDLE handle);
    // int32_t TYStopCapture(TY_DEV_HANDLE handle);
    // int32_t TYEnqueueBuffer(TY_DEV_HANDLE handle, void* buffer, uint32_t size);
    
    // 以下函数是自定义封装函数，不在TYApi.h中
    int32_t TYOpenStream(TY_DEV_HANDLE handle, TY_COMPONENT_ID component, TY_STREAM_HANDLE* stream_handle);
    int32_t TYCloseStream(TY_STREAM_HANDLE stream_handle);
    int32_t TYDequeueBuffer(TY_DEV_HANDLE handle, void** buffer, uint32_t* size, void** userdata, uint32_t timeout);
    int32_t TYQueueBuffer(TY_DEV_HANDLE handle, void* buffer, uint32_t size, void* userdata);
    int32_t TYGetFrameBuffer(TY_DEV_HANDLE handle, void** buffer, uint32_t* size, void** userdata, uint32_t timeout);
}
#endif

namespace percipio_layer {

// 前向声明
class TYDeviceInfo;
class TYDevice;
class DeviceList;
class TYContext;
class TYCamInterface;
class TYFrame;

// 设备基础信息结构 - 避免与TY SDK冲突，使用自定义命名
struct DeviceBaseInfo {
    DeviceBaseInfo() {
        memset(this, 0, sizeof(*this));
    }
    
    char id[64];
    char name[64];
    char modelName[64];
    char serialNumber[64];
    uint32_t  nChipID;
    TY_INTERFACE_INFO iface;
    TY_DEVICE_NET_INFO   netInfo;
};

// 设备信息类
class TYDeviceInfo {
public:
    TYDeviceInfo(const DeviceBaseInfo& info);
    ~TYDeviceInfo();
    
    const char* mac();
    const char* ip();
    const char* netmask();
    const char* gateway();
    const char* broadcast();
    
    const DeviceBaseInfo& info() const { return _info; }
    
private:
    DeviceBaseInfo _info;
};

// 事件回调类型定义
typedef std::function<void(void*)> EventCallback;

// 设备类
class TYDevice {
public:
    TYDevice(const TY_DEV_HANDLE handle, const DeviceBaseInfo& info);
    ~TYDevice();
    
    void registerEventCallback(const TY_EVENT eventID, void* data, EventCallback cb);
    std::shared_ptr<TYDeviceInfo> getDeviceInfo();
    
    TY_DEV_HANDLE _handle;
    
private:
    void onDeviceEventCallback(const TY_EVENT_INFO *event_info);
    static void _event_callback(TY_EVENT_INFO *event_info, void *userdata);
    friend void eventCallback(TY_EVENT_INFO *event_info, void *userdata);
    
    DeviceBaseInfo _dev_info;
    std::map<TY_EVENT, std::pair<void*, EventCallback>> _eventCallbackMap;
};

// 设备列表类
class DeviceList {
public:
    DeviceList(std::vector<DeviceBaseInfo>& devices);
    ~DeviceList();
    
    int devCount() const;
    bool empty() const;
    
    std::shared_ptr<TYDeviceInfo> getDeviceInfo(int idx);
    std::shared_ptr<TYDevice> getDevice(int idx);
    std::shared_ptr<TYDevice> getDeviceBySN(const char* sn);
    std::shared_ptr<TYDevice> getDeviceByIP(const char* ip);
    
private:
    std::vector<DeviceBaseInfo> devs;
    static std::set<TY_INTERFACE_HANDLE> gifaces;
};

// 相机接口类
class TYCamInterface {
public:
    TYCamInterface();
    ~TYCamInterface();
    
    TY_STATUS Reset();
    void List(std::vector<std::string>& interfaces);
    
private:
    std::vector<TY_INTERFACE_INFO> ifaces;
};

// 上下文类
class TYContext {
public:
    static TYContext& getInstance() {
        static TYContext instance;
        return instance;
    }
    
    std::shared_ptr<DeviceList> queryDeviceList(const char *iface = nullptr);
    std::shared_ptr<DeviceList> queryNetDeviceList(const char *iface = nullptr);
    
    // 获取全局接口列表的函数
    static TY_STATUS updateInterfaceList();
    
    enum ForceIPStyle {
        ForceIPStyleDynamic = 0,
        ForceIPStyleForce = 1,
        ForceIPStyleStatic = 2,
    };
    
    bool ForceNetDeviceIP(const ForceIPStyle style, const std::string& mac, 
                         const std::string& ip, const std::string& mask, 
                         const std::string& gateway);
    
private:
    TYContext() {}
    TYContext(const TYContext&) = delete;
    TYContext& operator=(const TYContext&) = delete;
};

// FastCamera类 - 简化相机操作
class FastCamera {
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
    TY_DEV_HANDLE handle() { return device ? device->_handle : nullptr; }
    
    // 设置接口ID
    TY_STATUS setIfaceId(const char* inf);
    
protected:
    TY_STATUS doStop();
    std::shared_ptr<TYFrame> fetchFrames(uint32_t timeout_ms);
    
protected:
    std::shared_ptr<TYDevice> device;
    std::mutex _dev_lock;
    bool isRuning;
    uint32_t components; // 已启用的组件位图
    std::string mIfaceId;
};

// 辅助函数
static inline std::string parseInterfaceID(const std::string& ifaceId) {
    if (ifaceId.find("usb") != std::string::npos) {
        return "USB";
    } else if (ifaceId.find("eth") != std::string::npos) {
        return "Ethernet";
    } else if (ifaceId.find("wlan") != std::string::npos) {
        return "WiFi";
    }
    return "Unknown";
}

} // namespace percipio_layer

#endif // DEVICE_HPP