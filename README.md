# STM32 IoT Gateway

基于 STM32F407VET6 的工业物联网网关，集成温湿度采集、以太网通信和 Web Dashboard。

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
- SSD1306 OLED 实时显示
- HTTP Web Dashboard (温湿度实时展示)
- JSON API 接口 (`/api/sensor`)
- SSE 实时数据推送 (`/sse`)
- FreeRTOS 多任务调度 (4 个任务)

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

VSCode 调试配置 (`.vscode/launch.json`):

```json
{
  "version": "0.2.0",
  "configurations": [{
    "name": "Cortex Debug",
    "executable": "./build/Debug/stm32-iot-gateway.elf",
    "request": "launch",
    "type": "cortex-debug",
    "serverType": "openocd",
    "configFiles": ["interface/stlink.cfg", "target/stm32f4x.cfg"],
    "runToEntryPoint": "main"
  }]
}
```

## FreeRTOS 任务

| 任务 | 优先级 | 栈大小 | 职责 |
| --- | --- | --- | --- |
| NetworkTask | AboveNormal | 4096B | W5500 以太网通信, HTTP 服务 |
| SensorTask | Normal | 1024B | DHT22 温湿度采集 |
| DisplayTask | BelowNormal | 1024B | OLED 显示更新 |
| LEDTask | Low | 512B | 系统心跳指示灯 |

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

## API 接口

| 端点 | 方法 | 说明 |
| --- | --- | --- |
| `/` | GET | Web Dashboard 页面 |
| `/api/sensor` | GET | JSON 格式温湿度数据 |
| `/sse` | GET | SSE 实时数据推送 |

JSON 响应示例:

```json
{
  "temperature": 25.6,
  "humidity": 62.3,
  "status": 0,
  "timestamp": 12345
}
```

## 技术栈

- **MCU**: STM32F407VET6 (ARM Cortex-M4F, 168MHz)
- **RTOS**: FreeRTOS (CMSIS-RTOS V2 API)
- **构建**: CMake + arm-none-eabi-gcc
- **代码生成**: STM32CubeMX 6.15.0
- **固件包**: STM32Cube FW_F4 V1.28.3

## License

MIT
