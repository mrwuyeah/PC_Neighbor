#include "samba_client.hpp"
#include <samba-4.0/libsmbclient.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

static SMBCCTX* context = nullptr;

static void auth_fn(const char* server, const char* share,
                   char* workgroup, int wgmaxlen,
                   char* username, int unmaxlen,
                   char* password, int pwmaxlen) {
    strncpy(username, "wjj", unmaxlen-1);
    strncpy(password, "20030509a", pwmaxlen-1);
    strncpy(workgroup, "WORKGROUP", wgmaxlen-1);
}

SambaClient::SambaClient() {
    context = smbc_new_context();
    if (!context) {
        throw std::runtime_error("Failed to create SMB context");
    }
    
    smbc_setOptionUserData(context, this);
    smbc_setFunctionAuthData(context, auth_fn);
    
    if (!smbc_init_context(context)) {
        smbc_free_context(context, 1);
        throw std::runtime_error("Failed to initialize SMB context");
    }
    smbc_set_context(context);
}

SambaClient::~SambaClient() {
    if (context) {
        smbc_free_context(context, 1);
    }
}

bool SambaClient::check_samba(const std::string& ip, int port) {
    std::string url = "smb://" + ip + ":" + std::to_string(port) + "/";
    int fd = smbc_opendir(url.c_str());
    if (fd >= 0) {
        smbc_closedir(fd);
        return true;
    }
    return false;
}

// 新增函数：扫描所有可能的Samba端口
std::vector<int> SambaClient::find_samba_ports(const std::string& ip) {
    std::vector<int> active_ports;
    
    // 先检查标准端口445
    if (this->check_samba(ip, 445)) {
        active_ports.push_back(445);
    }
    
    // 检查4455-4464端口范围
    for (int port = 4455; port <= 4464; ++port) {
        if (check_samba(ip, port)) {
            active_ports.push_back(port);
        }
    }
    
    return active_ports;
}

std::vector<SambaShare> SambaClient::list_shares(const std::string& ip, int port) {
    std::vector<SambaShare> shares;
    std::string url = "smb://" + ip + ":" + std::to_string(port) + "/";
    
    int fd = smbc_opendir(url.c_str());
    if (fd < 0) return shares;
    
    smbc_dirent* dir;
    while ((dir = smbc_readdir(fd)) != nullptr) {
        if (dir->name[0] != '.') {
            SambaShare share;
            share.name = dir->name;
            share.path = url + share.name;
            shares.push_back(share);
        }
    }
    
    smbc_closedir(fd);
    return shares;
}

bool SambaClient::download(const std::string& ip, const std::string& share,
                          const std::string& remote_path, const std::string& local_path) {
    std::string src = "smb://" + ip + "/" + share + "/" + remote_path;
    int src_fd = smbc_open(src.c_str(), O_RDONLY, 0);
    if (src_fd < 0) return false;
    
    int dst_fd = open(local_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dst_fd < 0) {
        smbc_close(src_fd);
        return false;
    }
    
    char buf[1024];
    ssize_t n;
    bool success = true;
    
    while ((n = smbc_read(src_fd, buf, sizeof(buf))) > 0){
        if (write(dst_fd, buf, n) != n) {
            success = false;
            break;
        }
    }
    
    close(dst_fd);
    smbc_close(src_fd);
    return success;
}

bool SambaClient::upload(const std::string& ip, const std::string& share,
                        const std::string& local_path, const std::string& remote_path) {
    int src_fd = open(local_path.c_str(), O_RDONLY);
    if (src_fd < 0) return false;
    
    std::string dst = "smb://" + ip + "/" + share + "/" + remote_path;
    int dst_fd = smbc_open(dst.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dst_fd < 0) {
        close(src_fd);
        return false;
    }
    
    char buf[1024];
    ssize_t n;
    bool success = true;
    
    while ((n = read(src_fd, buf, sizeof(buf))) > 0){
        if (smbc_write(dst_fd, buf, n) != n) {
            success = false;
            break;
        }
    }
    
    smbc_close(dst_fd);
    close(src_fd);
    return success;
}