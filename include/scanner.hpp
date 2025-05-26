// scanner.hpp
#pragma once
#include <vector>
#include <string>

struct DeviceInfo {
    std::string name;
    std::string ip;
    std::string mac;
};

class NetworkScanner {
public:
    // 扫描局域网设备(简化版)
    std::vector<DeviceInfo> scan(int timeout_ms = 1000);
    
private:
    // 使用ARP扫描发现设备
    std::vector<DeviceInfo> arp_scan(const std::string& interface);
};