#ifndef SAMBA_CLIENT_HPP
#define SAMBA_CLIENT_HPP

#include <vector>
#include <string>
#include <memory>
#include <fstream>
#include <jsoncpp/json/json.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <cstring>

struct SambaShare {
    std::string name;
    std::string path;
};

struct FileInfo {
    std::string name;
    std::string size;
};

class SambaClient {
private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
    std::string server_;

public:
    SambaClient();
    ~SambaClient();

    // 禁止拷贝
    SambaClient(const SambaClient&) = delete;
    SambaClient& operator=(const SambaClient&) = delete;

    // 服务器设置
    void setServer(const std::string& server) { server_ = server; }

    // Samba操作接口
    std::vector<FileInfo> list_files(const std::string& share_name);
    std::vector<std::string> scan_configured_nodes();
    bool connect(const std::string& ip,
                const std::string& username,
                const std::string& password);
    std::vector<SambaShare> list_shares();
    bool download(const std::string& share,
                 const std::string& remote_path,
                 const std::string& local_path);
    bool upload(const std::string& share,
               const std::string& local_path,
               const std::string& remote_path);
};

#endif // SAMBA_CLIENT_HPP