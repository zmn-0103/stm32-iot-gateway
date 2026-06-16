# Checklist

## P0 - 代码 Bug 修复
- [ ] W5500_WriteReg 首字节为 0x04，寄存器可正确写入
- [ ] W5500_Send/W5500_Recv 的 TX/RX Buffer 地址帧使用 OM=1 模式
- [ ] SSD1306_Update 中列地址高4位命令为 0x10
- [ ] SSD1306_DrawFloat 正确显示负数小数（如 -1.5 → "-1.5"）
- [ ] HTTP_ParseRequest 不使用 strtok，改为安全解析方式
- [ ] SN_MR_TCP 拆分为 0x01 和 SN_MR_TCP_ND 0x21

## P1 - 架构设计优化
- [ ] g_sensor_data 读写均有 osMutex 保护
- [ ] OLED I2C 操作有 osMutex 保护
- [ ] printf 有 mutex 保护或使用队列+DMA
- [ ] vNetworkTask 栈大小为 1024*4=4096 字节
- [ ] HTTP 服务器支持至少 2 个并发连接
- [ ] SSE 由消息队列驱动，保持长连接持续推送
- [ ] IWDG 看门狗已配置，各任务有喂狗逻辑
- [ ] FreeRTOS configCHECK_FOR_STACK_OVERFLOW = 2 已启用

## P2 - 文档结构增强
- [ ] BOM 物料清单表格完整（型号、数量、价格）
- [ ] 原理图/面包板接线图说明已增加
- [ ] 代码目录结构说明已增加
- [ ] 版本变更记录表已增加
- [ ] 量化测试标准已增加（Ping延迟、DHT22成功率、HTTP响应时间）
- [ ] 安全注意事项已增加（网络安全、电气安全、数据安全）
- [ ] Makefile 配置说明已增加

## P3 - 缺失功能
- [ ] DHCP 客户端可动态获取 IP
- [ ] MQTT 客户端可连接 Broker 并发布/订阅
- [ ] 配置参数可掉电保存和读取
- [ ] 网络断开可检测并自动重连
- [ ] OTA 固件升级功能可用
- [ ] Modbus RTU/TCP 协议可用
- [ ] 数据断网缓存，恢复后补传

## 细节优化
- [ ] DHT22 起始信号为 HAL_Delay(2)
- [ ] W5500 缓冲区分配已优化（2 Socket × 8KB）
- [ ] OLED 地址 0x78 与 0x3C 关系已统一说明
