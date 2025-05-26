#include "scanner.hpp"
#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-common/error.h>
#include <avahi-common/thread-watch.h>
#include <pcap.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/ether.h>
#include <unordered_map>
#include <thread>
#include <mutex>

struct NetworkScanner::Impl {
    AvahiThreadedPoll* poll = nullptr;
    AvahiClient* client = nullptr;
    AvahiServiceBrowser* browser = nullptr;
    
    std::unordered_map<std::string, DeviceInfo> devices;
    std::mutex devices_mutex;
    bool scan_complete = false;
    
    // mDNS回调
    static void avahi_callback(AvahiServiceBrowser* b, AvahiIfIndex interface, 
                              AvahiProtocol protocol, AvahiBrowserEvent event,
                              const char* name, const char* type, 
                              const char* domain, AvahiLookupResultFlags flags, 
                              void* userdata);
    
    // ARP抓包处理
    void start_arp_sniffing(const std::string& interface, int timeout_ms);
    static void packet_handler(u_char* user, const pcap_pkthdr* h, const u_char* bytes);
};

NetworkScanner::NetworkScanner() : pimpl(std::make_unique<Impl>()) {}

NetworkScanner::~NetworkScanner() {
    if (pimpl->browser) avahi_service_browser_free(pimpl->browser);
    if (pimpl->client) avahi_client_free(pimpl->client);
    if (pimpl->poll) avahi_threaded_poll_free(pimpl->poll);
}

std::vector<DeviceInfo> NetworkScanner::scan(int timeout_ms) {
    pimpl->scan_complete = false;
    pimpl->devices.clear();
    
    // 启动mDNS发现
    int error;
    pimpl->poll = avahi_threaded_poll_new();
    pimpl->client = avahi_client_new(avahi_threaded_poll_get(pimpl->poll), 
                                   AVAHI_CLIENT_NO_FAIL, nullptr, nullptr, &error);
    
    // 浏览所有服务类型
    pimpl->browser = avahi_service_browser_new(pimpl->client, AVAHI_IF_UNSPEC, 
                                             AVAHI_PROTO_UNSPEC, "_services._dns-sd._udp", 
                                             nullptr, static_cast<AvahiLookupFlags>(0), Impl::avahi_callback, pimpl.get());
    
    avahi_threaded_poll_start(pimpl->poll);
    
    // 启动ARP嗅探
    std::thread arp_thread(&Impl::start_arp_sniffing, pimpl.get(), "eth1", timeout_ms);
    
    // 等待扫描完成
    std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
    pimpl->scan_complete = true;
    
    arp_thread.join();
    
    // 转换结果
    std::vector<DeviceInfo> result;
    {
        std::lock_guard<std::mutex> lock(pimpl->devices_mutex);
        for (auto& [_, device] : pimpl->devices) {
            result.push_back(device);
        }
    }
    
    return result;
}

// mDNS回调实现
void NetworkScanner::Impl::avahi_callback(AvahiServiceBrowser* b, AvahiIfIndex interface, 
                                        AvahiProtocol protocol, AvahiBrowserEvent event,
                                        const char* name, const char* type, 
                                        const char* domain, AvahiLookupResultFlags flags, 
                                        void* userdata) {
    auto* self = static_cast<Impl*>(userdata);
    
    if (event == AVAHI_BROWSER_NEW) {
        DeviceInfo device;
        device.name = name;
        device.type = type;
        
        std::lock_guard<std::mutex> lock(self->devices_mutex);
        self->devices[device.name] = device;
    }
}

// ARP嗅探实现
void NetworkScanner::Impl::start_arp_sniffing(const std::string& interface, int timeout_ms) {
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t* handle = pcap_open_live(interface.c_str(), BUFSIZ, 1, timeout_ms, errbuf);
    
    if (!handle) return;
    
    struct bpf_program fp;
    if (pcap_compile(handle, &fp, "arp", 0, PCAP_NETMASK_UNKNOWN) == -1) {
        pcap_close(handle);
        return;
    }
    
    if (pcap_setfilter(handle, &fp) == -1) {
        pcap_close(handle);
        return;
    }
    
    pcap_loop(handle, 0, packet_handler, reinterpret_cast<u_char*>(this));
    pcap_close(handle);
}

void NetworkScanner::Impl::packet_handler(u_char* user, const pcap_pkthdr* h, const u_char* bytes) {
    auto* self = reinterpret_cast<Impl*>(user);
    if (self->scan_complete) return;
    
    const struct ether_arp* arp = reinterpret_cast<const struct ether_arp*>(bytes + sizeof(ether_header));
    
    DeviceInfo device;
    device.ip = inet_ntoa(*reinterpret_cast<const in_addr*>(&arp->arp_spa));
    device.mac = ether_ntoa(reinterpret_cast<const ether_addr*>(&arp->arp_sha));
    
    std::lock_guard<std::mutex> lock(self->devices_mutex);
    self->devices[device.ip] = device;
}