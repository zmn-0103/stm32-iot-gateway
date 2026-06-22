# STM32 IoT Gateway

基于 STM32F407VET6 的物联网网关，集成温湿度采集、以太网通信和 OLED 实时显示。

## 硬件平台

| 组件 | 型号 | 接口 |
| --- | --- | --- |
| MCU | STM32F407VET6 (Cortex-M4F, 168MHz) | - |
| 以太网 | W5500 | SPI1 |
| 温湿度传感器 | DHT22 (AM2302) | GPIO (PB1) |
| 显示屏 | 0.96" OLED (SSD1306, 128x64) | I2C1 |
| 调试串口 | USART3 | PB10/PB11 |

## 功能

- DHT22 温湿度数据采集 (2s 周期)
- W5500 有线以太网通信 (硬件 TCP/IP 协议栈)
- TCP Client 定时上报温湿度数据到上位机
- SSD1306 OLED 实时显示 (网络状态、温湿度)
- FreeRTOS 多任务调度 (3 个任务)

## 项目结构

```
stm32-iot-gateway/
├── Core/
│   ├── Inc/            # 头文件
│   └── Src/            # 源文件 (main.c, freertos.c, 外设驱动)
├── Drivers/            # STM32 HAL & CMSIS (CubeMX 生成, 只读)
├── Middlewares/        # FreeRTOS (CubeMX 生成, 只读)
├── cmake/              # CMake 工具链配置
├── stm32-iot-gateway.ioc   # STM32CubeMX 工程文件
├── CMakeLists.txt      # 顶层 CMake 配置
└── dev-doc.md          # 开发文档
```

## 构建

### 环境要求

- [GNU Arm Embedded Toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm) (arm-none-eabi-gcc)
- CMake >= 3.22
- Ninja 或 Make
- STM32CubeMX (修改外设配置时需要)

### 编译

```bash
# 配置
cmake --preset Debug

# 编译
cmake --build build/Debug
```

产物: `build/Debug/stm32-iot-gateway.elf`

### Release 构建

```bash
cmake --preset Release
cmake --build build/Release
```

## 烧录与调试

使用 ST-Link 或兼容调试器:

```bash
# OpenOCD 烧录
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
  -c "program build/Debug/stm32-iot-gateway.elf verify reset exit"
```

## FreeRTOS 任务

| 任务 | 优先级 | 栈大小 | 职责 |
| --- | --- | --- | --- |
| NetworkTask | AboveNormal | 4096B | W5500 以太网通信, TCP Client 上报 |
| SensorTask | Normal | 1024B | DHT22 温湿度采集 |
| DisplayTask | BelowNormal | 1024B | OLED 显示更新 |

## 外设引脚

| 功能 | 引脚 | 说明 |
| --- | --- | --- |
| SPI1_SCK | PA5 | W5500 时钟 |
| SPI1_MISO | PA6 | W5500 数据输入 |
| SPI1_MOSI | PA7 | W5500 数据输出 |
| W5500_CS | PA4 | W5500 片选 |
| W5500_RST | PB0 | W5500 复位 |
| DHT22_DATA | PB1 | 温湿度数据 |
| I2C1_SCL | PB6 | OLED 时钟 |
| I2C1_SDA | PB7 | OLED 数据 |
| USART3_TX | PB10 | 调试串口发送 |
| USART3_RX | PB11 | 调试串口接收 |
| LED | PC13 | 状态指示灯 |

## 网络配置

默认网络参数 (在 `freertos.c` 中配置):

| 参数 | 值 |
| --- | --- |
| 本机 IP | 192.168.1.100 |
| 子网掩码 | 255.255.255.0 |
| 网关 | 192.168.1.1 |
| MAC | 02:08:DC:01:02:03 |
| 上位机 IP | 192.168.1.1 |
| 上位机端口 | 5000 |
| 上报间隔 | 5s |

上报数据格式: `T:25.6 H:62.3\r\n`

## 技术栈

- **MCU**: STM32F407VET6 (ARM Cortex-M4F, 168MHz)
- **RTOS**: FreeRTOS (CMSIS-RTOS V2 API)
- **构建**: CMake + arm-none-eabi-gcc
- **代码生成**: STM32CubeMX

## License

MIT