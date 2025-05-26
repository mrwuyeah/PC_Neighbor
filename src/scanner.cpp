// scanner.cpp
#include "scanner.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <netinet/if_ether.h>
#include <unistd.h>
#include <cstring>
#include <fstream>

std::vector<DeviceInfo> NetworkScanner::scan(int timeout_ms) {
    // 简化实现：直接读取ARP缓存
    std::vector<DeviceInfo> devices;
    std::ifstream arp_file("/proc/net/arp");
    
    if (arp_file) {
        std::string line;
        std::getline(arp_file, line); // 跳过标题行
        
        while (std::getline(arp_file, line)) {
            DeviceInfo device;
            char ip[16], mac[18], dummy[256];
            
            if (sscanf(line.c_str(), "%15s %*s %*s %17s %255s", 
                      ip, mac, dummy) >= 2) {
                device.ip = ip;
                device.mac = mac;
                device.name = "Unknown"; // 简化处理
                devices.push_back(device);
            }
        }
    }
    
    return devices;
}