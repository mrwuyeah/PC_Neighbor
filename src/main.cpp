#include "scanner.hpp"
#include "samba_client.hpp"
#include <iostream>
#include <iomanip>
#include <vector>
#include <map>

using namespace std;

struct SambaService {
    string ip;
    int port;
    vector<SambaShare> shares;
};

void scan_all_ports(SambaClient& client, const string& ip, vector<SambaService>& results) {
    // 标准Samba端口 + 4455-4464
    vector<int> ports_to_scan = {445, 139};
    for (int i = 4455; i <= 4464; i++) {
        ports_to_scan.push_back(i);
    }

    for (int port : ports_to_scan) {
        if (client.check_samba(ip, port)) {
            auto shares = client.list_shares(ip, port);
            if (!shares.empty()) {
                results.push_back({ip, port, shares});
            }
        }
    }
}

void print_services(const vector<SambaService>& services) {
    if (services.empty()) {
        cout << "未发现任何Samba服务\n";
        return;
    }

    cout << "\n发现 " << services.size() << " 个Samba服务:\n";
    cout << left << setw(5) << "ID" 
         << setw(18) << "IP地址" 
         << setw(8) << "端口" 
         << "共享目录\n";
    cout << string(50, '-') << endl;

    for (size_t i = 0; i < services.size(); i++) {
        cout << left << setw(5) << i+1
             << setw(18) << services[i].ip
             << setw(8) << services[i].port;

        if (!services[i].shares.empty()) {
            cout << services[i].shares[0].name;
            for (size_t j = 1; j < services[i].shares.size(); j++) {
                cout << ", " << services[i].shares[j].name;
            }
        }
        cout << endl;
    }
}

void perform_operation(SambaClient& client, const SambaService& service) {
    cout << "\n选择操作:\n1. 下载文件\n2. 上传文件\n选择(1/2): ";
    int action;
    cin >> action;
    cin.ignore();

    string remote_path, local_path;
    if (action == 1) {
        cout << "输入远程文件路径(相对共享目录): ";
        getline(cin, remote_path);
        cout << "输入本地保存路径: ";
        getline(cin, local_path);

        if (client.download(service.ip, service.shares[0].name, remote_path, local_path)) {
            cout << "下载成功!\n";
        } else {
            cout << "下载失败!\n";
        }
    } else if (action == 2) {
        cout << "输入本地文件路径: ";
        getline(cin, local_path);
        cout << "输入远程保存路径(相对共享目录): ";
        getline(cin, remote_path);

        if (client.upload(service.ip, service.shares[0].name, local_path, remote_path)) {
            cout << "上传成功!\n";
        } else {
            cout << "上传失败!\n";
        }
    }
}

int main() {
    try {
        SambaClient client;
        vector<string> ips = {"127.0.0.1"}; // 可以添加更多IP
        vector<SambaService> found_services;

        cout << "正在扫描Samba服务..." << endl;
        
        for (const auto& ip : ips) {
            scan_all_ports(client, ip, found_services);
        }

        print_services(found_services);

        if (!found_services.empty()) {
            while (true) {
                cout << "\n输入要操作的服务ID(1-" << found_services.size() 
                     << ")，或0退出: ";
                int choice;
                cin >> choice;
                cin.ignore();

                if (choice == 0) break;
                if (choice < 1 || choice > static_cast<int>(found_services.size())) {
                    cout << "无效选择!\n";
                    continue;
                }

                perform_operation(client, found_services[choice-1]);
            }
        }

    } catch (const exception& e) {
        cerr << "错误: " << e.what() << endl;
        return 1;
    }

    return 0;
}