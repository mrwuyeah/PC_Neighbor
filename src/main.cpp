#include "scanner.hpp"
#include "samba_client.hpp"
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>

// 打印设备列表
void print_devices(const std::vector<DeviceInfo>& devices) {
    std::cout << "\n发现 " << devices.size() << " 个设备:\n";
    std::cout << std::left 
              << std::setw(18) << "IP地址"
              << std::setw(20) << "MAC地址"
              << std::setw(15) << "设备名称" << "\n";
    std::cout << std::string(53, '-') << "\n";
    
    for (const auto& device : devices) {
        std::cout << std::left 
                  << std::setw(18) << device.ip
                  << std::setw(20) << device.mac
                  << std::setw(15) << (device.name.empty() ? "Unknown" : device.name) << "\n";
    }
}

// 检查Samba服务并交互
void check_and_interact_with_samba(SambaClient& client, const std::string& ip) {
    // 常见Samba端口列表
    const std::vector<int> samba_ports = {445, 139, 4455, 4456, 4457, 4458, 4459, 4460, 4461, 4464};
    
    std::cout << "\n扫描设备 " << ip << " 的Samba服务...\n";
    
    bool found_samba = false;
    
    for (int port : samba_ports) {
        if (client.check_samba(ip, port)) {
            found_samba = true;
            std::cout << "✔ 发现Samba服务: " << ip << ":" << port << "\n";
            
            auto shares = client.list_shares(ip, port);
            if (!shares.empty()) {
                std::cout << "  可用共享目录:\n";
                for (const auto& share : shares) {
                    std::cout << "    - " << share.name << "\n";
                }
                
                // 用户交互
                std::cout << "\n输入要操作的共享目录名(或回车跳过): ";
                std::string share_name;
                std::getline(std::cin, share_name);
                
                if (!share_name.empty()) {
                    // 验证共享是否存在
                    auto it = std::find_if(shares.begin(), shares.end(), 
                        [&share_name](const SambaShare& s) { return s.name == share_name; });
                    
                    if (it != shares.end()) {
                        std::cout << "\n1. 下载文件\n2. 上传文件\n选择操作(1/2): ";
                        std::string choice;
                        std::getline(std::cin, choice);
                        
                        if (choice == "1" || choice == "2") {
                            std::string remote_path, local_path;
                            
                            if (choice == "1") {
                                std::cout << "输入远程文件路径(相对共享目录): ";
                                std::getline(std::cin, remote_path);
                                std::cout << "输入本地保存路径: ";
                                std::getline(std::cin, local_path);
                                
                                if (client.download(ip, share_name, remote_path, local_path)) {
                                    std::cout << "✓ 下载成功!\n";
                                } else {
                                    std::cerr << "✗ 下载失败!\n";
                                }
                            } else {
                                std::cout << "输入本地文件路径: ";
                                std::getline(std::cin, local_path);
                                std::cout << "输入远程保存路径(相对共享目录): ";
                                std::getline(std::cin, remote_path);
                                
                                if (client.upload(ip, share_name, local_path, remote_path)) {
                                    std::cout << "✓ 上传成功!\n";
                                } else {
                                    std::cerr << "✗ 上传失败!\n";
                                }
                            }
                        } else {
                            std::cout << "无效选择，跳过操作\n";
                        }
                    } else {
                        std::cout << "共享目录不存在，跳过操作\n";
                    }
                }
            } else {
                std::cout << "  该Samba服务没有可用的共享目录\n";
            }
        }
    }
    
    if (!found_samba) {
        std::cout << "✗ 未发现Samba服务\n";
    }
}

int main() {
    try {
        // 1. 扫描局域网设备
        std::cout << "正在扫描局域网设备...\n";
        NetworkScanner scanner;
        auto devices = scanner.scan();
        
        if (devices.empty()) {
            std::cout << "未发现任何设备\n";
            return 0;
        }

        // 2. 添加本地回环地址(127.0.0.1)到扫描列表
        DeviceInfo localhost;
        localhost.ip = "127.0.0.1";
        localhost.mac = "00:00:00:00:00:00";  // 本地回环没有真实MAC
        localhost.name = "localhost";
        devices.push_back(localhost);
        
        print_devices(devices);

        
        // 2. 检查Samba服务
        SambaClient client;
        
        for (const auto& device : devices) {
            check_and_interact_with_samba(client, device.ip);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "发生错误: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}