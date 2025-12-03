#include "Device.hpp"
#include <sstream>
#include <iostream>
#include <cstring>
#include <cstdlib>

struct to_string
{
    std::ostringstream ss;
    template<class T> to_string & operator << (const T & val) { ss << val; return *this; }
    operator std::string() const { return ss.str(); }
};

// 简化的updateDevicesParallel函数
static void updateDevicesParallel(const std::vector<TY_INTERFACE_HANDLE>& hIfaces) {
    // 这里实现设备列表的并行更新逻辑
    // 简化实现，实际项目中可能需要更复杂的处理
}

// CHECK_RET宏定义
#define CHECK_RET(expr) \
    do { \
        TY_STATUS _status = (expr); \
        if (_status != TY_STATUS_OK) { \
            std::cerr << "Error: " << #expr << " failed with status " << _status << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            return _status; \
        } \
    } while(0)

// ASSERT_OK宏定义 - 如果不是TY_STATUS_OK则直接返回false
#define ASSERT_OK(expr) \
    do { \
        TY_STATUS _status = (expr); \
        if (_status != TY_STATUS_OK) { \
            std::cerr << "Error: " << #expr << " failed with status " << _status << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            return false; \
        } \
    } while(0)

// ASSERT宏定义 - 如果条件不满足则终止程序
#define ASSERT(condition) \
    do { \
        if (!(condition)) { \
            std::cerr << "Assertion failed: " << #condition << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::exit(1); \
        } \
    } while(0)

// LOGD宏定义  
#define LOGD(msg) std::cout << "[DEBUG] " << msg << std::endl

static std::string TY_ERROR(TY_STATUS status)
{
    return to_string() << status << "(" << TYErrorString(status) << ").";
}

static inline TY_STATUS searchDevice(std::vector<TY_DEVICE_BASE_INFO>& out, const char *inf_id = nullptr, TY_INTERFACE_TYPE type = TY_INTERFACE_ALL)
{
    out.clear();
    ASSERT_OK( TYUpdateInterfaceList() );

    uint32_t n = 0;
    ASSERT_OK( TYGetInterfaceNumber(&n) );
    if(n == 0) return TY_STATUS_ERROR;

    std::vector<TY_INTERFACE_INFO> ifaces(n);
    ASSERT_OK( TYGetInterfaceList(&ifaces[0], n, &n) );

    bool found = false;
    std::vector<TY_INTERFACE_HANDLE> hIfaces;
    for(uint32_t i = 0; i < ifaces.size(); i++){
        TY_INTERFACE_HANDLE hIface;
        if(type & ifaces[i].type) {
                //Interface Not setted
            if (nullptr == inf_id ||
                //Interface been setted and matched
                strcmp(inf_id, ifaces[i].id) == 0) {
                ASSERT_OK( TYOpenInterface(ifaces[i].id, &hIface) );
                hIfaces.push_back(hIface);
                found = true;
                //Interface been setted, found and just break
                if(nullptr != inf_id) {
                    break;
                }
            }
        }

    }
    if(!found) return TY_STATUS_ERROR;
    updateDevicesParallel(hIfaces);

    for (uint32_t i = 0; i < hIfaces.size(); i++) {
        TY_INTERFACE_HANDLE hIface = hIfaces[i];
        uint32_t n = 0;
        TYGetDeviceNumber(hIface, &n);
        if(n > 0){
            std::vector<TY_DEVICE_BASE_INFO> devs(n);
            TYGetDeviceList(hIface, &devs[0], n, &n);
            for(uint32_t j = 0; j < n; j++) {
                out.push_back(devs[j]);
            }
        }
        TYCloseInterface(hIface);
    }

    if(out.size() == 0){
      std::cout << "not found any device" << std::endl;
      return TY_STATUS_ERROR;
    }

    return TY_STATUS_OK;
}

namespace percipio_layer {

TYDeviceInfo::TYDeviceInfo(const DeviceBaseInfo& info)
{
    _info = info;
}

TYDeviceInfo::~TYDeviceInfo()
{

}

const char* TYDeviceInfo::mac()
{
    if(!TYIsNetworkInterface(_info.iface.type)) {
        return nullptr;
    }
    return _info.netInfo.mac;
}

const char* TYDeviceInfo::ip()
{
    if(!TYIsNetworkInterface(_info.iface.type))
        return nullptr;
    return _info.netInfo.ip;
}

const char* TYDeviceInfo::netmask()
{
    if(!TYIsNetworkInterface(_info.iface.type))
        return nullptr;
    return _info.netInfo.netmask;
}

const char* TYDeviceInfo::gateway()
{
    if(!TYIsNetworkInterface(_info.iface.type))
        return nullptr;
    return _info.netInfo.gateway;
}

const char* TYDeviceInfo::broadcast()
{
    if(!TYIsNetworkInterface(_info.iface.type))
        return nullptr;
    return _info.netInfo.broadcast;
}

void eventCallback(TY_EVENT_INFO *event_info, void *userdata) {
    TYDevice* handle = (TYDevice*)userdata;
    handle->onDeviceEventCallback(event_info);
}

 TYCamInterface::TYCamInterface()
 {
    TYContext::getInstance();
    Reset();
 }

TYCamInterface::~TYCamInterface()
{

}

TY_STATUS TYCamInterface::Reset()
{
    TY_STATUS status;
    status = TYUpdateInterfaceList();
    if(status != TY_STATUS_OK) return status;

    uint32_t n = 0;
    status = TYGetInterfaceNumber(&n);
    if(status != TY_STATUS_OK) return status;

    if(n == 0) return TY_STATUS_OK;

    ifaces.resize(n);
    status = TYGetInterfaceList(&ifaces[0], n, &n);
    return status;
}

void TYCamInterface::List(std::vector<std::string>& interfaces)
{
    for(auto& iter : ifaces) {
        std::cout << iter.id << std::endl;
        interfaces.push_back(iter.id);
    }
}

TYDevice::TYDevice(const TY_DEV_HANDLE handle, const DeviceBaseInfo& info)
{
    _handle = handle;
    _dev_info = info;
    TYRegisterEventCallback(_handle, eventCallback, this);
}

TYDevice::~TYDevice()
{
    TYCloseDevice(_handle);
}

void  TYDevice::registerEventCallback(const TY_EVENT eventID, void* data, EventCallback cb)
{
    _eventCallbackMap[eventID] = std::make_pair(data, cb);
}

void TYDevice::onDeviceEventCallback(const TY_EVENT_INFO *event_info)
{
    auto it = _eventCallbackMap.find(event_info->eventId);
    if (it != _eventCallbackMap.end() && it->second.second != nullptr) {
        it->second.second(it->second.first);
    }
}

std::shared_ptr<TYDeviceInfo> TYDevice::getDeviceInfo()
{
    return std::shared_ptr<TYDeviceInfo>(new TYDeviceInfo(_dev_info));
}

std::set<TY_INTERFACE_HANDLE> DeviceList::gifaces;
DeviceList::DeviceList(std::vector<DeviceBaseInfo>& devices)
{
    devs = devices;
}

DeviceList::~DeviceList()
{
    for (TY_INTERFACE_HANDLE iface : gifaces) {
        TYCloseInterface(iface);
    }
    gifaces.clear();
}

int DeviceList::devCount() const
{
    return devs.size();
}

bool DeviceList::empty() const
{
    return devs.empty();
}

std::shared_ptr<TYDeviceInfo> DeviceList::getDeviceInfo(int idx)
{
    if((idx < 0) || (idx > devCount())) {
        std::cout << "idx out of range" << std::endl;
        return nullptr;
    }
    
    return std::shared_ptr<TYDeviceInfo>(new TYDeviceInfo(devs[idx]));
}

std::shared_ptr<TYDevice> DeviceList::getDevice(int idx)
{
    if((idx < 0) || (idx > devCount())) {
        std::cout << "idx out of range" << std::endl;
        return nullptr;
    }

    TY_INTERFACE_HANDLE hIface = NULL;
    TY_DEV_HANDLE hDevice = NULL;

    TY_STATUS status = TY_STATUS_OK;
    status = TYOpenInterface(devs[idx].iface.id, &hIface);
    if(status != TY_STATUS_OK)  {
        std::cout << "Open interface failed with error code: " << TY_ERROR(status) << std::endl;
        return nullptr;
    }

    gifaces.insert(hIface);
    std::string ifaceId = devs[idx].iface.id;
    std::string open_log = std::string("open device ") + devs[idx].id +
        "\non interface " + parseInterfaceID(ifaceId);
    std::cout << open_log << std::endl;
    status = TYOpenDevice(hIface, devs[idx].id, &hDevice);
    if(status != TY_STATUS_OK) {
        std::cout << "Open device < " << devs[idx].id << "> failed with error code: " << TY_ERROR(status) << std::endl;
        return nullptr;
    }

    TY_DEVICE_BASE_INFO ty_info;
            status = TYGetDeviceInfo(hDevice, &ty_info);
            if(status != TY_STATUS_OK) {
                std::cout << "Get device info failed with error code: " << TY_ERROR(status) << std::endl;
                return nullptr;
            }

            // 转换为DeviceBaseInfo格式
            DeviceBaseInfo info;
            memset(&info, 0, sizeof(info));
            strncpy(info.id, ty_info.id, sizeof(info.id) - 1);
            strncpy(info.name, ty_info.vendorName, sizeof(info.name) - 1);
            strncpy(info.modelName, ty_info.modelName, sizeof(info.modelName) - 1);
            strncpy(info.serialNumber, ty_info.id, sizeof(info.serialNumber) - 1);
            info.nChipID = 0;
            info.iface = ty_info.iface;
            info.netInfo = ty_info.netInfo;

    return std::shared_ptr<TYDevice>(new TYDevice(hDevice, info));
}

std::shared_ptr<TYDevice> DeviceList::getDeviceBySN(const char* sn)
{
    TY_STATUS status = TY_STATUS_OK;
    TY_INTERFACE_HANDLE hIface = NULL;
    TY_DEV_HANDLE hDevice = NULL;
    
    if(!sn) {
        std::cout << "Invalid parameters" << std::endl;
        return nullptr;
    }

    for(size_t i = 0; i < devs.size(); i++) {
        if(strcmp(devs[i].id, sn) == 0) {
            status = TYOpenInterface(devs[i].iface.id, &hIface);
            if(status != TY_STATUS_OK)  continue;

            gifaces.insert(hIface);
            std::string ifaceId = devs[i].iface.id;
            std::string open_log = std::string("open device ") + devs[i].id +
                "\non interface " + parseInterfaceID(ifaceId);
            std::cout << open_log << std::endl;
            status = TYOpenDevice(hIface, devs[i].id, &hDevice);
            if(status != TY_STATUS_OK) continue;

            TY_DEVICE_BASE_INFO ty_info;
            status = TYGetDeviceInfo(hDevice, &ty_info);
            if(status != TY_STATUS_OK) {
                TYCloseDevice(hDevice);
                continue;
            }
            
            // 转换为DeviceBaseInfo格式
            DeviceBaseInfo info;
            memset(&info, 0, sizeof(info));
            strncpy(info.id, ty_info.id, sizeof(info.id) - 1);
            strncpy(info.name, ty_info.vendorName, sizeof(info.name) - 1);
            strncpy(info.modelName, ty_info.modelName, sizeof(info.modelName) - 1);
            strncpy(info.serialNumber, ty_info.id, sizeof(info.serialNumber) - 1);
            info.nChipID = 0;
            info.iface = ty_info.iface;
            info.netInfo = ty_info.netInfo;
            
            return std::shared_ptr<TYDevice>(new TYDevice(hDevice, info));
        }
    }

    std::cout << "Device <sn:" << sn << "> not found!" << std::endl;
    return nullptr;
}

std::shared_ptr<TYDevice> DeviceList::getDeviceByIP(const char* ip)
{
    TY_STATUS status = TY_STATUS_OK;
    TY_INTERFACE_HANDLE hIface = NULL;
    TY_DEV_HANDLE hDevice = NULL;

    if(!ip) {
        std::cout << "Invalid parameters" << std::endl;
        return nullptr;
    }

    for(size_t i = 0; i < devs.size(); i++) {        
        if(TYIsNetworkInterface(devs[i].iface.type)) {
            status = TYOpenInterface(devs[i].iface.id, &hIface);
            if(status != TY_STATUS_OK)  continue;
            std::string open_log = "open device ";
            if(ip && strlen(ip)) {
                open_log += ip;
                status = TYOpenDeviceWithIP(hIface, ip, &hDevice);
            } else {
                open_log += devs[i].id;
                status = TYOpenDevice(hIface, devs[i].id, &hDevice);
            }
            std::string ifaceId = devs[i].iface.id;
            open_log += "\non interface " + parseInterfaceID(ifaceId);
            std::cout << open_log << std::endl;

            if(status != TY_STATUS_OK) continue;

            TY_DEVICE_BASE_INFO ty_info;
            status = TYGetDeviceInfo(hDevice, &ty_info);
            if(status != TY_STATUS_OK) {
                TYCloseDevice(hDevice);
                continue;
            }
            
            // 转换为DeviceBaseInfo格式
            DeviceBaseInfo info;
            memset(&info, 0, sizeof(info));
            strncpy(info.id, ty_info.id, sizeof(info.id) - 1);
            strncpy(info.name, ty_info.vendorName, sizeof(info.name) - 1);
            strncpy(info.modelName, ty_info.modelName, sizeof(info.modelName) - 1);
            strncpy(info.serialNumber, ty_info.id, sizeof(info.serialNumber) - 1);
            info.nChipID = 0;
            info.iface = ty_info.iface;
            info.netInfo = ty_info.netInfo;

            return std::shared_ptr<TYDevice>(new TYDevice(hDevice, info));
        }
    }

    std::cout << "Device <ip:" << ip << "> not found!" << std::endl;
    return nullptr;
}

std::shared_ptr<DeviceList> TYContext::queryDeviceList(const char *iface)
{
    std::vector<TY_DEVICE_BASE_INFO> ty_devs;
    searchDevice(ty_devs, iface);
    
    // 转换为DeviceBaseInfo格式
    std::vector<DeviceBaseInfo> devs;
    for (const auto& ty_dev : ty_devs) {
        DeviceBaseInfo info;
        memset(&info, 0, sizeof(info));
        strncpy(info.id, ty_dev.id, sizeof(info.id) - 1);
        strncpy(info.name, ty_dev.vendorName, sizeof(info.name) - 1);
        strncpy(info.modelName, ty_dev.modelName, sizeof(info.modelName) - 1);
        strncpy(info.serialNumber, ty_dev.id, sizeof(info.serialNumber) - 1);
        info.nChipID = 0;
        info.iface = ty_dev.iface;
        info.netInfo = ty_dev.netInfo;
        devs.push_back(info);
    }
    
    return std::shared_ptr<DeviceList>(new DeviceList(devs));
}

std::shared_ptr<DeviceList> TYContext::queryNetDeviceList(const char *iface)
{
    std::vector<TY_DEVICE_BASE_INFO> ty_devs;
    searchDevice(ty_devs, iface, TY_INTERFACE_ETHERNET | TY_INTERFACE_IEEE80211);
    
    // 转换为DeviceBaseInfo格式
    std::vector<DeviceBaseInfo> devs;
    for (const auto& ty_dev : ty_devs) {
        DeviceBaseInfo info;
        memset(&info, 0, sizeof(info));
        strncpy(info.id, ty_dev.id, sizeof(info.id) - 1);
        strncpy(info.name, ty_dev.vendorName, sizeof(info.name) - 1);
        strncpy(info.modelName, ty_dev.modelName, sizeof(info.modelName) - 1);
        strncpy(info.serialNumber, ty_dev.id, sizeof(info.serialNumber) - 1);
        info.nChipID = 0;
        info.iface = ty_dev.iface;
        info.netInfo = ty_dev.netInfo;
        devs.push_back(info);
    }
    
    return std::shared_ptr<DeviceList>(new DeviceList(devs));
}

bool TYContext::ForceNetDeviceIP(const ForceIPStyle style, const std::string& mac, const std::string& ip, const std::string& mask, const std::string& gateway)
{
    ASSERT_OK( TYUpdateInterfaceList() );

    uint32_t n = 0;
    ASSERT_OK( TYGetInterfaceNumber(&n) );
    if(n == 0) return false;
    
    std::vector<TY_INTERFACE_INFO> ifaces(n);
    ASSERT_OK( TYGetInterfaceList(&ifaces[0], n, &n) );
    ASSERT( n == ifaces.size() );

    bool   open_needed  = false;
    const char * ip_save      = ip.c_str();
    const char * netmask_save = mask.c_str();
    const char * gateway_save = gateway.c_str();
    switch(style)
    {
        case ForceIPStyleDynamic:
            if(strcmp(ip_save, "0.0.0.0") != 0) {
                open_needed = true;
            }
            ip_save      = "0.0.0.0";
            netmask_save = "0.0.0.0";
            gateway_save = "0.0.0.0";
            break;
        case ForceIPStyleForce:
            open_needed = true;
            break;
        case ForceIPStyleStatic:
            open_needed = true;
            break;
        default:
            return false;
    }

    bool result = false;
    for(uint32_t i = 0; i < n; i++) {
        if(TYIsNetworkInterface(ifaces[i].type)) {
            TY_INTERFACE_HANDLE hIface;
            ASSERT_OK( TYOpenInterface(ifaces[i].id, &hIface) );
            if (TYForceDeviceIP(hIface, mac.c_str(), ip.c_str(), mask.c_str(), gateway.c_str()) == TY_STATUS_OK) {
                LOGD("**** Set Temporary IP/Netmask/Gateway ...Done! ****");
                if(open_needed) {
                    TYUpdateDeviceList(hIface);
                    TY_DEV_HANDLE hDev;
                    if(TYOpenDeviceWithIP(hIface, ip.c_str(), &hDev) == TY_STATUS_OK){
                        int32_t ip_i[4];
                        uint8_t ip_b[4];
                        int32_t ip32;
                        sscanf(ip_save, "%d.%d.%d.%d", &ip_i[0], &ip_i[1], &ip_i[2], &ip_i[3]);
                        ip_b[0] = ip_i[0];ip_b[1] = ip_i[1];ip_b[2] = ip_i[2];ip_b[3] = ip_i[3];
                        ip32 = TYIPv4ToInt(ip_b);
                        ASSERT_OK( TYSetInt(hDev, TY_COMPONENT_DEVICE, TY_INT_PERSISTENT_IP, ip32) );
                        sscanf(netmask_save, "%d.%d.%d.%d", &ip_i[0], &ip_i[1], &ip_i[2], &ip_i[3]);
                        ip_b[0] = ip_i[0];ip_b[1] = ip_i[1];ip_b[2] = ip_i[2];ip_b[3] = ip_i[3];
                        ip32 = TYIPv4ToInt(ip_b);
                        ASSERT_OK( TYSetInt(hDev, TY_COMPONENT_DEVICE, TY_INT_PERSISTENT_SUBMASK, ip32) );
                        sscanf(gateway_save, "%d.%d.%d.%d", &ip_i[0], &ip_i[1], &ip_i[2], &ip_i[3]);
                        ip_b[0] = ip_i[0];ip_b[1] = ip_i[1];ip_b[2] = ip_i[2];ip_b[3] = ip_i[3];
                        ip32 = TYIPv4ToInt(ip_b);
                        ASSERT_OK( TYSetInt(hDev, TY_COMPONENT_DEVICE, TY_INT_PERSISTENT_GATEWAY, ip32) );

                        result = true;
                        std::cout << "**** Set Persistent IP/Netmask/Gateway ...Done! ****" <<std::endl;
                    } else {
                        result = false;
                    }
                } else {
                    result = true;
                }
            }
            ASSERT_OK( TYCloseInterface(hIface));        
        }
    }
    return result;
}

TY_STATUS TYContext::updateInterfaceList()
{
    return TYUpdateInterfaceList();
}
}