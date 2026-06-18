# W5500 TCP CLI 调试记录

## 问题总览

在实现 W5500 TCP Server + 命令解析功能过程中，遇到了以下问题：

| 问题 | 根因 | 解决方案 |
|------|------|----------|
| FreeRTOS 调度器启动失败 | 缺少 `osKernelInitialize()` | 在 `MX_FREERTOS_Init()` 开头调用 |
| 串口乱码 | HSE 晶振频率不匹配（25MHz vs 8MHz） | 修改 HSE_VALUE 和 PLLM |
| Socket 1 无法打开 | BSB 编码错误，未左移 3 位 | 宏改为 `(3*n+1) << 3` |
| Tera Term 输入命令报错 | 终端本地回显导致数据重复 | 关闭 Local echo |

---

## 问题 1：FreeRTOS 调度器启动失败

### 现象
- `osKernelStart()` 返回 `-1`（osError）
- 串口只输出 `BOOT OK`，FreeRTOS 任务不执行

### 根因
CMSIS-RTOS v2 的 `osKernelStart()` 检查 `KernelState == osKernelReady`，如果没调用 `osKernelInitialize()`，状态不是 Ready，直接返回 osError。

```c
// cmsis_os2.c 源码
if (KernelState == osKernelReady) {
    vTaskStartScheduler();
    stat = osOK;
} else {
    stat = osError;  // ← 走到这里
}
```

### 解决方案
在 `MX_FREERTOS_Init()` 开头调用 `osKernelInitialize()`：

```c
void MX_FREERTOS_Init(void) {
  osKernelInitialize();  // ← 必须加
  // ... 创建任务 ...
}
```

---

## 问题 2：串口乱码（HSE 晶振频率不匹配）

### 现象
- 串口收到乱码（中文字符、\0 字节）
- 改变波特率无效

### 根因
- CubeMX .ioc 配置 HSE = 25MHz
- 实际板载晶振 = 8MHz
- PLL 输出频率偏差 3.125 倍，USART 波特率完全错误

### 解决方案
修改三处：
1. `stm32f4xx_hal_conf.h`：`HSE_VALUE` 改为 `8000000U`
2. `system_stm32f4xx.c`：`HSE_VALUE` 改为 `((uint32_t)8000000)`
3. `main.c` SystemClock_Config：`PLLM` 改为 `8`

PLL 计算：8MHz / 8 × 336 / 2 = 168MHz

### 知识点
- STM32F4 的 USART 时钟 = PCLKx，**不享受**定时器的 2×倍频
- HAL 的 `HAL_UART_Init()` 用 `HAL_RCC_GetPCLK1Freq()` 计算 BRR 是正确的
- 不要手动覆写 BRR

---

## 问题 3：W5500 Socket 1 无法打开

### 现象
- Socket 0（TCP Server）正常
- Socket 1（TCP Client）所有寄存器读出 0
- `W5500_TCP_Open()` 返回 0（失败）

### 诊断过程
添加诊断代码打印 `OPEN`、`Listen` 返回值和回读的 `PORT`、`SR` 寄存器：
```
OPEN=0 LIS=0 PORT=5000 SR=0x01
```
关键线索：**SR=0x01 不是合法的 Socket 状态**（合法值：0x00=CLOSED, 0x13=INIT, 0x14=LISTEN, 0x17=ESTABLISHED）。

0x01 恰好等于 BSB_SOCK_REG(0) 的未移位值。说明 W5500 把控制字节 `0x01` 解析为 BSB=0（Common 寄存器）+ OPM=01，读到的 0x01 实际是 Common 寄存器 GAR[2]（网关地址第 3 字节 = 192.168.**1**.1 → 0x01）。

回读 PORT=5000 正确，是因为 `W5500_WriteReg16` 写端口时 BSB 同样错误，写入了 Common 寄存器区域，恰好覆盖了某些地址，回读时又从同一错误地址读回，所以"看起来正确"。

### 根因
W5500 SPI 控制字节格式：`BSB[7:3] | RWB[2] | OM[1:0]`

BSB 值在 datasheet 中是 5 位（bit[4:0]），需要**左移 3 位**放入控制字节的 bit[7:3]。

错误的宏（BSB 值未移位）：
```c
#define W5500_BSB_SOCK_REG(s)  (3 * (s) + 1)      // Socket 0 = 0x01, Socket 1 = 0x04
// 实际发送: 0x01 → W5500 解析为 BSB=0 (Common) + OPM=01，读写的是 Common 寄存器
```

正确的宏（左移 3 位）：
```c
#define W5500_BSB_SOCK_REG(s)  ((3 * (s) + 1) << 3)  // Socket 0 = 0x08, Socket 1 = 0x20
```

### BSB 编码表

| Socket | 类型 | BSB 值 (datasheet) | 控制字节 (<<3) |
|--------|------|-------------------|----------------|
| 0 | REG | 0x01 | 0x08 |
| 0 | TX | 0x02 | 0x10 |
| 0 | RX | 0x03 | 0x18 |
| 1 | REG | 0x04 | 0x20 |
| 1 | TX | 0x05 | 0x28 |
| 1 | RX | 0x06 | 0x30 |
| n | REG | 3n+1 | (3n+1)<<3 |

### 解决方案
```c
// w5500.h
#define W5500_BSB_COMMON       (0x00 << 3)
#define W5500_BSB_SOCK_REG(s)  ((3 * (s) + 1) << 3)
#define W5500_BSB_SOCK_TX(s)   ((3 * (s) + 2) << 3)
#define W5500_BSB_SOCK_RX(s)   ((3 * (s) + 3) << 3)
```

---

## 问题 4：Tera Term 输入命令报错

### 现象
- Python 脚本发 `temp` → 正确返回 `T:26.5`
- Tera Term 输入 `temp` → 返回 `ERR: unknown cmd`

### 根因
Tera Term 默认开启 **Local echo**，输入的字符会回显到 W5500。W5500 收到的数据是 `temp` + 回显的 `temp` = `temptemp`，strcmp 匹配失败。

### 解决方案
Tera Term 菜单：**Setup** → **Terminal** → 取消勾选 **Local echo**

或使用 Python 脚本测试（无回显问题）：
```python
import socket
s = socket.socket()
s.connect(('192.168.1.100', 5000))
print(s.recv(1024).decode())  # 收欢迎信息
s.send(b'temp\r\n')
print(s.recv(1024).decode())  # 收温度数据
```

---

## 网络配置

| 参数 | 值 |
|------|-----|
| W5500 IP | 192.168.1.100 |
| W5500 子网 | 255.255.255.0 |
| W5500 网关 | 192.168.1.1 |
| W5500 MAC | 02:08:DC:01:02:03 |
| TCP Server 端口 | 5000 |
| PC 以太网 IP | 192.168.1.1 |

### 接线

| W5500IO-P | STM32F407VET6 | 功能 |
|-----------|---------------|------|
| VIN | 3.3V | 电源 |
| GND | GND | 共地 |
| SCS | PA4 | SPI 片选 |
| CLK | PA5 | SPI 时钟 |
| MISO | PA6 | SPI 数据入 |
| MOSI | PA7 | SPI 数据出 |
| RST | PB0 | 复位 |

---

## TCP CLI 支持的命令

| 命令 | 响应 | 说明 |
|------|------|------|
| `temp` | `T:26.5` | 温度 |
| `humi` | `H:60.4` | 湿度 |
| `data` | `T:26.5 H:60.8` | 温湿度 |
| `led on` | `LED ON` | 点亮 LED |
| `led off` | `LED OFF` | 熄灭 LED |
| `help` | 命令列表 | 显示帮助 |
