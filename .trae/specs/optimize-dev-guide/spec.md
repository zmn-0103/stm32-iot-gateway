# STM32 IoT Gateway 开发指南优化 Spec

## Why
当前开发指南存在 W5500 SPI 协议实现错误、FreeRTOS 多任务资源竞争、OLED 驱动 Bug 等代码缺陷，同时缺少工业网关必备的看门狗、DHCP、MQTT 等功能，以及 BOM、原理图等文档内容，需要系统性优化。

## What Changes

### P0 - 代码 Bug 修复（必须修复）
- 修复 W5500 SPI 写帧格式（写操作首字节 0x00 → 0x04）
- 修复 W5500 TX/RX Buffer 读写地址计算（使用 OM=1 模式）
- 修复 SSD1306 列地址命令（0x0F → 0x10）
- 修复 SSD1306_DrawFloat 负数小数计算 Bug
- 修复 HTTP_ParseRequest 中 strtok 非重入问题
- 拆分 SN_MR_TCP 宏定义（0x01 / 0x21）

### P1 - 架构设计优化（强烈建议）
- 为 g_sensor_data 增加 osMutex 互斥保护
- 为 OLED I2C 操作增加互斥锁
- HTTP 服务器支持多 Socket 多连接
- SSE 改为真正的实时推送（消息队列驱动）
- 增加 IWDG 看门狗（5 秒超时 + 任务喂狗）
- 启用 FreeRTOS 栈溢出检测（configCHECK_FOR_STACK_OVERFLOW=2）
- printf 增加 mutex 保护或改用队列+DMA
- 网络任务栈从 512*4 增大到 1024*4

### P2 - 文档结构增强（建议增加）
- 增加 BOM 物料清单表格
- 增加原理图/面包板接线图说明
- 增加代码目录结构说明
- 增加版本变更记录表
- 增加量化测试标准
- 增加安全注意事项
- 增加 Makefile 配置说明

### P3 - 缺失功能（未来扩展）
- DHCP 客户端
- MQTT 客户端
- 配置持久化（Flash/EEPROM）
- 心跳/掉线检测与自动重连
- OTA 固件升级
- Modbus RTU/TCP
- 数据本地缓存与断网补传
- HTTPS/TLS

### 细节优化
- DHT22 起始信号 HAL_Delay(1) → HAL_Delay(2)
- W5500 缓冲区分配优化（2 Socket 时 2KB→8KB）
- OLED 地址说明统一（0x78 vs 0x3C）

## Impact
- Affected specs: W5500 驱动、SSD1306 驱动、DHT22 驱动、HTTP 服务器、FreeRTOS 任务架构
- Affected code: w5500.c/h, ssd1306.c/h, dht22.c, http_server.c/h, main.c

## ADDED Requirements

### Requirement: W5500 SPI 协议帧格式
系统 SHALL 按照 W5500 数据手册实现 SPI 读写帧格式：
- 读操作首字节：BSB|R/W=0|OM=0
- 写操作首字节：BSB|R/W=1|OM=0
- TX/RX Buffer 访问使用 OM=1 模式

#### Scenario: W5500 寄存器写入
- **WHEN** 调用 W5500_WriteReg 写入任意寄存器
- **THEN** SPI 首字节为 0x04（通用寄存器），数据成功写入

#### Scenario: W5500 TCP 数据收发
- **WHEN** 调用 W5500_Send/W5500_Recv 收发 TCP 数据
- **THEN** Buffer 地址帧使用 OM=1 模式，数据正确读写

### Requirement: FreeRTOS 共享资源互斥保护
系统 SHALL 对所有跨任务共享资源提供互斥访问保护：
- g_sensor_data 使用 osMutex 保护
- OLED I2C 操作使用 osMutex 保护
- printf 输出使用 osMutex 保护

#### Scenario: 多任务并发访问传感器数据
- **WHEN** SensorTask 写入 g_sensor_data 同时 NetworkTask 读取
- **THEN** 数据一致性得到保证，不会读到部分更新的字段

### Requirement: IWDG 看门狗
系统 SHALL 配置独立看门狗，超时时间 5 秒，各任务定期喂狗

#### Scenario: 任务卡死
- **WHEN** 任何任务卡死超过 5 秒未喂狗
- **THEN** 系统自动复位重启

### Requirement: HTTP 多连接支持
系统 SHALL 支持至少 2 个并发 HTTP 连接，使用 W5500 多 Socket

#### Scenario: 多客户端同时访问
- **WHEN** 两个浏览器同时访问网关
- **THEN** 两个请求都能正常响应

## MODIFIED Requirements

### Requirement: SSD1306 列地址设置
SSD1306_Update 中列地址设置命令 SHALL 使用 0x10（高4位列地址），而非 0x0F

### Requirement: SSD1306_DrawFloat 负数处理
SSD1306_DrawFloat SHALL 正确处理负数的小数部分，使用 fabs 或单独处理符号位

### Requirement: DHT22 起始信号时长
DHT22_Read 中主机拉低时间 SHALL 为 2ms（HAL_Delay(2)），而非 1ms

### Requirement: 网络任务栈大小
vNetworkTask 栈大小 SHALL 为 1024*4=4096 字节，而非 512*4=2048 字节

## REMOVED Requirements
（无删除项）
