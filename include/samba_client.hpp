// samba_client.hpp
#pragma once
#include <vector>
#include <string>
#include <stdexcept>  // 添加这行以包含runtime_error

struct SambaShare {
    std::string name;
    std::string path;
};

class SambaClient {
public:
    SambaClient();
    ~SambaClient();

    std::vector<int> find_samba_ports(const std::string& ip);
    
    // 检查Samba服务是否可用
    bool check_samba(const std::string& ip, int port = 445);
    
    // 列出共享目录
    std::vector<SambaShare> list_shares(const std::string& ip, int port = 445);
    
    // 文件操作
    bool download(const std::string& ip, const std::string& share,
                 const std::string& remote_path, const std::string& local_path);
                 
    bool upload(const std::string& ip, const std::string& share,
               const std::string& local_path, const std::string& remote_path);
    
private:
    std::string username = "wjj";
    std::string password = "20030509a";
};