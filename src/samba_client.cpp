#include "samba_client.hpp"
#include <samba-4.0/libsmbclient.h>
#include <jsoncpp/json/json.h>
#include <fstream>
#include <cstring>
#include <sys/stat.h>
#include <fcntl.h>

struct SambaClient::Impl {
    SMBCCTX* context = nullptr;
    std::string current_ip;
    
    static void auth_fn(const char* server, const char* share,
                      char* workgroup, int wgmaxlen,
                      char* username, int unmaxlen,
                      char* password, int pwmaxlen);
};

std::vector<FileInfo> SambaClient::list_files(const std::string& share_name) {
    std::vector<FileInfo> files;
    std::string path = "smb://" + server_ + "/" + share_name + "/";
    
    int dir_fd = smbc_opendir(path.c_str());
    if (dir_fd < 0) {
        throw std::runtime_error("无法打开共享目录: " + path);
    }

    struct smbc_dirent* dirent;
    while ((dirent = smbc_readdir(dir_fd)) != nullptr) {
        if (strcmp(dirent->name, ".") == 0 || strcmp(dirent->name, "..") == 0) {
            continue;
        }

        std::string full_path = path + dirent->name;
        struct stat st;
        if (smbc_stat(full_path.c_str(), &st) == 0) {
            FileInfo info;
            info.name = dirent->name;
            info.size = std::to_string(st.st_size);
            files.push_back(info);
        }
    }

    smbc_closedir(dir_fd);
    return files;
}

SambaClient::SambaClient() : pImpl(std::make_unique<Impl>()) {
    pImpl->context = smbc_new_context();
    if (!pImpl->context) {
        throw std::runtime_error("无法创建SMB上下文");
    }

    smbc_setOptionUserData(pImpl->context, this);
    smbc_setFunctionAuthData(pImpl->context, Impl::auth_fn);
    smbc_init_context(pImpl->context);
    smbc_set_context(pImpl->context);
}

SambaClient::~SambaClient() {
    if (pImpl->context) {
        smbc_free_context(pImpl->context, 1);
    }
}

bool SambaClient::connect(const std::string& ip, 
                         const std::string& username,
                         const std::string& password) {
    pImpl->current_ip = ip;
    std::string url = "smb://" + ip + "/";
    int fd = smbc_opendir(url.c_str());
    if (fd < 0) return false;
    smbc_closedir(fd);
    return true;
}

std::vector<SambaShare> SambaClient::list_shares() {
    std::vector<SambaShare> shares;
    std::string url = "smb://" + pImpl->current_ip + "/";
    
    int fd = smbc_opendir(url.c_str());
    if (fd < 0) return shares;

    struct smbc_dirent* dir;
    while ((dir = smbc_readdir(fd)) != nullptr) {
        if (dir->name[0] != '.' && 
            (dir->smbc_type == SMBC_FILE_SHARE || dir->smbc_type == SMBC_DIR)) {
            SambaShare share;
            share.name = dir->name;
            share.path = url + share.name;
            shares.push_back(share);
        }
    }

    smbc_closedir(fd);
    return shares;
}

bool SambaClient::download(const std::string& share, 
                          const std::string& remote_path,
                          const std::string& local_path) {
    std::string src = "smb://" + pImpl->current_ip + "/" + share + "/" + remote_path;
    int src_fd = smbc_open(src.c_str(), O_RDONLY, 0);
    if (src_fd < 0) return false;

    std::ofstream dst(local_path, std::ios::binary);
    if (!dst) {
        smbc_close(src_fd);
        return false;
    }

    char buf[1024];
    int n;
    while ((n = smbc_read(src_fd, buf, sizeof(buf))) > 0) {
        dst.write(buf, n);
    }

    smbc_close(src_fd);
    return n == 0;
}

bool SambaClient::upload(const std::string& share,
                        const std::string& local_path,
                        const std::string& remote_path) {
    std::ifstream src(local_path, std::ios::binary);
    if (!src) return false;

    std::string dst_url = "smb://" + pImpl->current_ip + "/" + share + "/" + remote_path;
    int dst_fd = smbc_open(dst_url.c_str(), O_WRONLY | O_CREAT, 0666);
    if (dst_fd < 0) return false;

    char buf[1024];
    while (src.read(buf, sizeof(buf))) {
        if (smbc_write(dst_fd, buf, src.gcount()) < 0) {
            smbc_close(dst_fd);
            return false;
        }
    }

    smbc_close(dst_fd);
    return true;
}

void SambaClient::Impl::auth_fn(const char* server, const char* share,
                               char* workgroup, int wgmaxlen,
                               char* username, int unmaxlen,
                               char* password, int pwmaxlen) {
    Json::Value root;
    std::ifstream config("config.json");
    if (config.is_open()) {
        config >> root;
        strncpy(username, root["username"].asString().c_str(), unmaxlen-1);
        strncpy(password, root["password"].asString().c_str(), pwmaxlen-1);
    } else {
        strncpy(username, "guest", unmaxlen-1);
        strncpy(password, "", pwmaxlen-1);
    }
    strncpy(workgroup, "WORKGROUP", wgmaxlen-1);
}

std::vector<std::string> SambaClient::scan_configured_nodes() {
    std::vector<std::string> nodes;
    Json::Value root;
    std::ifstream config("config.json");
    
    if (config.is_open()) {
        config >> root;
        for (const auto& node : root["nodes"]) {
            nodes.push_back(node.asString());
        }
    }
    
    return nodes;
}