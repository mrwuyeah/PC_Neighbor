#!/bin/bash
# 增强版Samba多实例配置脚本
# 作者：wjj
# 功能：创建10个可被网络发现的Samba共享实例

# 停止所有现有Samba服务
echo "正在停止现有Samba服务..."
sudo systemctl stop smbd nmbd 2>/dev/null
sudo pkill -9 smbd 2>/dev/null
sudo rm -f /run/samba/*.pid 2>/dev/null

# 创建主共享目录结构
BASE_DIR="/home/wjj/pc_Neighbor"
echo "创建共享目录结构..."
sudo mkdir -p "$BASE_DIR" 2>/dev/null
sudo chown -R wjj:wjj "$BASE_DIR"

# 设置Samba用户认证
echo "配置Samba用户认证..."
(echo "20030509a"; echo "20030509a") | sudo smbpasswd -a wjj -s 2>/dev/null

# 生成并启动10个独立实例
for i in {1..10}; do
  PORT=$((4454 + $i))
  CONFIG_FILE="/etc/samba/smb_$i.conf"
  INSTANCE_DIR="$BASE_DIR/pc_neighbor_$i"
  NETBIOS_NAME="PC_NEIGHBOR_$i"
  
  # 创建实例专属目录
  sudo mkdir -p "$INSTANCE_DIR"
  sudo chown wjj:wjj "$INSTANCE_DIR"
  sudo chmod 775 "$INSTANCE_DIR"
  
  # 生成增强版配置文件
  sudo bash -c "cat > $CONFIG_FILE" <<EOF
[global]
    # 网络发现配置
    netbios name = $NETBIOS_NAME
    workgroup = WORKGROUP
    server string = Samba Server %v (Instance $i)
    local master = yes
    preferred master = yes
    os level = 65
    domain master = no
    
    # 端口配置
    smb ports = $PORT
    disable netbios = no
    
    # 安全设置
    security = user
    map to guest = bad user
    guest account = nobody
    
    # 性能优化
    socket options = TCP_NODELAY SO_RCVBUF=8192 SO_SNDBUF=8192
    use sendfile = yes
    
    # 实例隔离
    pid directory = /run/samba/instance_$i
    lock directory = /var/run/samba/instance_$i
    private dir = /var/lib/samba/instance_$i
    state directory = /var/lib/samba/instance_$i
    cache directory = /var/cache/samba/instance_$i
    
    # 日志记录
    log file = /var/log/samba/instance_$i.log
    log level = 1
    max log size = 50

[Shared_$i]
    path = $INSTANCE_DIR
    comment = PC Neighbor Share $i
    valid users = wjj
    read only = no
    browseable = yes
    create mask = 0664
    directory mask = 0775
    force user = wjj
    force group = wjj
    inherit permissions = yes
EOF

  # 创建实例专属目录结构
  sudo mkdir -p \
    "/run/samba/instance_$i" \
    "/var/run/samba/instance_$i" \
    "/var/lib/samba/instance_$i" \
    "/var/cache/samba/instance_$i" \
    "/var/log/samba"
  
  sudo chown -R root:root "/run/samba/instance_$i" "/var/run/samba/instance_$i"
  
  # 启动实例（带调试日志）
  echo "启动实例 $i (端口: $PORT)"
  sudo smbd -D -s "$CONFIG_FILE" --debuglevel=1
  
  # 创建测试文件
  echo "这是实例 $i 的测试文件" | sudo tee "$INSTANCE_DIR/test_$i.txt" >/dev/null
done

# 验证输出
echo -e "\n\033[32m部署完成！验证信息：\033[0m"
echo -e "----------------------------------------"
echo "运行中的Samba实例:"
sudo ps aux | grep smbd | grep -v grep | awk '{print $11,$12}'
echo -e "\n监听的端口:"
sudo netstat -tulnp | grep smbd | awk '{print $4,$7}'
echo -e "\n共享目录结构:"
tree -L 2 "$BASE_DIR"
echo -e "\n访问示例:"
echo "smb://<本机IP>:4455/Shared_1"
echo "用户名: wjj"
echo "密码: 20030509a"
