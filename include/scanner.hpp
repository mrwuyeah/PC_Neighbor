#pragma once
#include <vector>
#include <string>
#include <memory>

struct DeviceInfo {
    std::string name;
    std::string ip;
    std::string mac;
    std::string type;  // PC/Phone/Tablet等
};

class NetworkScanner {
public:
    NetworkScanner();
    ~NetworkScanner();
    
    // 扫描局域网设备
    std::vector<DeviceInfo> scan(int timeout_ms = 1000);
    
private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};