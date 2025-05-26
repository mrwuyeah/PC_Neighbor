cmake_minimum_required(VERSION 3.10)
project(pc_Neighbor)

set(CMAKE_CXX_STANDARD 17)

# 查找依赖库
find_package(PkgConfig REQUIRED)
pkg_check_modules(AVAHI REQUIRED avahi-client)
pkg_check_modules(SAMBA REQUIRED smbclient)
pkg_check_modules(PCAP REQUIRED libpcap)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

# 包含目录
include_directories(
    ${AVAHI_INCLUDE_DIRS}
    ${SAMBA_INCLUDE_DIRS}
    ${PCAP_INCLUDE_DIRS}
    ${JSONCPP_INCLUDE_DIRS}
    include
)

# 源文件
add_executable(pc_Neighbor
    src/main.cpp
    src/scanner.cpp
    src/samba_client.cpp
)



# 链接库
target_link_libraries(pc_Neighbor
    ${AVAHI_LIBRARIES}
    ${SAMBA_LIBRARIES}
    ${PCAP_LIBRARIES}
    ${JSONCPP_LIBRARIES}
    pthread
)