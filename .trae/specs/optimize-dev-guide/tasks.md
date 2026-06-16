# Tasks

## P0 - 代码 Bug 修复
- [ ] Task 1: 修复 W5500 SPI 写帧格式
  - [ ] 1.1: W5500_WriteReg 首字节从 0x00 改为 0x04
  - [ ] 1.2: W5500_ReadBuf/W5500_WriteBuf 增加读写方向参数
  - [ ] 1.3: 修复 W5500_Send/W5500_Recv 的 TX/RX Buffer 地址帧（使用 OM=1 模式）
- [ ] Task 2: 修复 SSD1306 列地址命令（0x0F → 0x10）
- [ ] Task 3: 修复 SSD1306_DrawFloat 负数小数计算
- [ ] Task 4: 修复 HTTP_ParseRequest 中 strtok 非重入问题
- [ ] Task 5: 拆分 SN_MR_TCP 宏定义（0x01 基础 / 0x21 含 ND）

## P1 - 架构设计优化
- [ ] Task 6: 为 g_sensor_data 增加 osMutex 互斥保护
- [ ] Task 7: 为 OLED I2C 操作增加 osMutex 互斥锁
- [ ] Task 8: printf 增加 mutex 保护
- [ ] Task 9: 网络任务栈从 512*4 增大到 1024*4
- [ ] Task 10: HTTP 服务器支持多 Socket 多连接
- [ ] Task 11: SSE 改为消息队列驱动的实时推送
- [ ] Task 12: 增加 IWDG 看门狗配置和任务喂狗
- [ ] Task 13: 启用 FreeRTOS 栈溢出检测

## P2 - 文档结构增强
- [ ] Task 14: 增加 BOM 物料清单表格
- [ ] Task 15: 增加原理图/面包板接线图说明
- [ ] Task 16: 增加代码目录结构说明
- [ ] Task 17: 增加版本变更记录表
- [ ] Task 18: 增加量化测试标准
- [ ] Task 19: 增加安全注意事项
- [ ] Task 20: 增加 Makefile 配置说明

## P3 - 缺失功能
- [ ] Task 21: DHCP 客户端
- [ ] Task 22: MQTT 客户端
- [ ] Task 23: 配置持久化（Flash/EEPROM）
- [ ] Task 24: 心跳/掉线检测与自动重连
- [ ] Task 25: OTA 固件升级
- [ ] Task 26: Modbus RTU/TCP
- [ ] Task 27: 数据本地缓存与断网补传

## 细节优化
- [ ] Task 28: DHT22 起始信号 HAL_Delay(1) → HAL_Delay(2)
- [ ] Task 29: W5500 缓冲区分配优化（2 Socket 时 2KB→8KB）
- [ ] Task 30: OLED 地址说明统一（0x78 vs 0x3C 关系说明）

# Task Dependencies
- Task 6/7/8 依赖 CubeMX 中 FreeRTOS 已配置 mutex 支持
- Task 10 依赖 Task 1（W5500 SPI 修复后才能可靠多 Socket）
- Task 11 依赖 Task 6（互斥保护）和 Task 10（多 Socket）
- Task 12 依赖 CubeMX 中 IWDG 配置
- Task 21/22 依赖 Task 1（W5500 驱动修复）
- P3 所有任务依赖 P0 和 P1 完成
