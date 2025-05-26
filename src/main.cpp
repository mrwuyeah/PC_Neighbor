#include "../include/scanner.hpp"
#include "../include/samba_client.hpp"
#include <iostream>
#include <jsoncpp/json/json.h>
#include <fstream>
#include <iomanip>
#include <vector>
#include <string>

void print_devices(const std::vector<DeviceInfo>& devices) {
    std::cout << "发现 " << devices.size() << " 个设备:\n";
    std::cout << std::left << std::setw(15) << "IP地址"
              << std::setw(20) << "MAC地址"
              << std::setw(20) << "设备名称"
              << std::setw(15) << "类型" << "\n";
    for (const auto& device : devices) {
        std::cout << std::left << std::setw(15) << device.ip
                  << std::setw(20) << device.mac
                  << std::setw(20) << device.name
                  << std::setw(15) << device.type << "\n";
    }
}

void list_remote_files(SambaClient& client, const std::string& share_name) {
    try {
        auto files = client.list_files(share_name);
        if (files.empty()) {
            std::cout << "共享目录中没有文件可下载。\n";
            return;
        }
        std::cout << "\n可下载的文件列表:\n";
        std::cout << std::left << std::setw(5) << "ID"
                  << std::setw(40) << "文件名"
                  << std::setw(15) << "大小" << "\n";
        for (size_t i = 0; i < files.size(); ++i) {
            std::cout << std::left << std::setw(5) << i+1
                      << std::setw(40) << files[i].name
                      << std::setw(15) << files[i].size << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "获取文件列表失败: " << e.what() << "\n";
    }
}

void samba_interaction() {
    SambaClient client;
    
    // 1. 扫描所有配置的Samba节点
    auto nodes = client.scan_configured_nodes();
    if (nodes.empty()) {
        // 如果没有配置节点，尝试扫描本地网络
        NetworkScanner scanner;
        auto devices = scanner.scan(1000);
        for (const auto& device : devices) {
            if (device.type.find("smb") != std::string::npos) {
                nodes.push_back(device.ip);
            }
        }
    }

    std::cout << "\n发现 " << nodes.size() << " 个Samba节点:\n";
    for (size_t i = 0; i < nodes.size(); ++i) {
        std::cout << i+1 << ". " << nodes[i] << "\n";
    }
    if (nodes.empty()) return;

    // 2. 选择要操作的节点
    std::cout << "选择节点(1-" << nodes.size() << "): ";
    size_t node_choice;
    std::cin >> node_choice;
    if (node_choice < 1 || node_choice > nodes.size()) return;

    // 3. 获取共享目录
    Json::Value root;
    std::ifstream config("config.json");
    if (config.is_open()) {
        config >> root;
    } else {
        // 如果没有配置文件，使用默认凭证
        root["username"] = "guest";
        root["password"] = "";
    }

    if (client.connect(nodes[node_choice-1],
                      root["username"].asString(),
                      root["password"].asString())) {
        auto shares = client.list_shares();
        std::cout << "可用共享目录:\n";
        for (size_t i = 0; i < shares.size(); ++i) {
            std::cout << i+1 << ". " << shares[i].name << "\n";
        }

        // 4. 文件操作
        if (!shares.empty()) {
            std::cout << "选择共享目录(1-" << shares.size() << "): ";
            size_t share_choice;
            std::cin >> share_choice;
            if (share_choice >= 1 && share_choice <= shares.size()) {
                // 显示可下载文件列表
                list_remote_files(client, shares[share_choice-1].name);
                
                std::cout << "\n1. 下载文件\n2. 上传文件\n选择操作: ";
                int op;
                std::cin >> op;
                std::string remote, local;
                
                if (op == 1) {
                    std::cout << "输入远程文件路径: ";
                    std::cin >> remote;
                    std::cout << "输入本地保存路径: ";
                    std::cin >> local;
                    if (client.download(shares[share_choice-1].name, remote, local)) {
                        std::cout << "下载成功! 文件已保存到: " << local << "\n";
                    } else {
                        std::cerr << "下载失败! 请检查路径和权限。\n";
                    }
                } else if (op == 2) {
                    std::cout << "输入本地文件路径: ";
                    std::cin >> local;
                    std::cout << "输入远程保存路径: ";
                    std::cin >> remote;
                    if (client.upload(shares[share_choice-1].name, local, remote)) {
                        std::cout << "上传成功! 文件已保存到共享目录。\n";
                    } else {
                        std::cerr << "上传失败! 请检查路径和权限。\n";
                    }
                }
            }
        }
    } else {
        std::cerr << "连接Samba服务器失败! 请检查用户名和密码。\n";
    }
}

int main() {
    // 1. 网络设备扫描 (保持原样)
    NetworkScanner scanner;
    auto devices = scanner.scan(1000);
    print_devices(devices);
    
    // 2. Samba交互
    samba_interaction();
    
    return 0;
}