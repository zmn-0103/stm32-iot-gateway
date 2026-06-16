# 项目约束 - STM32 IoT Gateway

> **执行摘要**：本文件约束 AI 在本项目中的行为。优先级降序：代码安全 > CubeMX 规则 > FreeRTOS 约束 > 编码规范。

STM32F407VET6 + W5500 + DHT22 + OLED，CubeMX + CMake + FreeRTOS，C11。

## 绝对禁止

- 编辑 `Drivers/` 和 `Middlewares/` 目录下的任何文件（CubeMX 生成，只读）
- 在 `Core/Src/` 和 `Core/Inc/` 的 CubeMX 生成文件中，将代码写在 `USER CODE BEGIN` / `USER CODE END` 标记之外（重新生成时会被覆盖）
- 使用 C++ 特性（RTTI、异常、class、template）
- 修改 FreeRTOS 堆大小（30720B，由 `.ioc` 锁定）
- 改变 `main()` 中外设初始化顺序（`MX_GPIO_Init` → `MX_I2C1_Init` → `MX_SPI1_Init` → `MX_USART3_UART_Init`，CubeMX 管理）
- 占用 SWD 调试引脚（PA13、PA14）

## 必须先问我的操作

- 修改 `.ioc` 文件（CubeMX 工程配置）
- 修改 `CMakePresets.json` 或 `cmake/gcc-arm-none-eabi.cmake`
- 修改 `FreeRTOSConfig.h` 中的任何配置
- 添加新的 FreeRTOS 任务
- 修改中断优先级或 NVIC 配置
- 添加新依赖或新外设驱动

## 代码安全

### CubeMX 代码生成规则

所有用户代码必须写在 `/* USER CODE BEGIN xxx */` 和 `/* USER CODE END xxx */` 之间。示例：

```c
/* USER CODE BEGIN Includes */
#include "my_header.h"
/* USER CODE END Includes */
```

重新生成代码前，确认用户代码在正确区域内。丢失用户代码 = 丢失功能。

### FreeRTOS 约束

- 任务延迟用 `osDelay()`，不用 `HAL_Delay()`（后者阻塞整个系统）
- 共享资源（全局变量、外设句柄）必须用互斥锁保护
- 任务栈余量必须 > 20%，开发中用 `uxTaskGetStackHighWaterMark()` 验证
- 堆分配总和不可接近 30720B
- 新任务在 `MX_FREERTOS_Init()` 的 `USER CODE BEGIN RTOS_THREADS` 区域创建
- 任务函数放在文件底部的 `USER CODE BEGIN` 区域

### 外设约束

| 外设 | 引脚 | HAL 句柄 | 用途 |
| --- | --- | --- | --- |
| SPI1 | PA5/PA6/PA7 | `hspi1` | W5500 通信 |
| I2C1 | PB6/PB7 | `hi2c1` | OLED 显示 |
| USART3 | PB10/PB11 | `huart3` | 调试串口 |
| GPIO | PA4 | - | W5500 CS |
| GPIO | PB0 | - | W5500 RST |
| GPIO | PB1 | - | DHT22 DATA |
| GPIO | PC13 | - | LED |

引脚标签定义在 `main.h`：`W5500_CS_Pin`、`W5500_RST_Pin`、`DHT22_DATA_Pin`、`LED_Pin`。

### SPI 通信规则

- W5500 SPI 控制字节格式：`BSB[4:1]|R/W|OM[1:0]`
  - 读通用寄存器：`0x00`（BSB=00000, R=0, OM=00）
  - 写通用寄存器：`0x04`（BSB=00000, W=1, OM=00）
  - TX Buffer 访问：控制字节 `0x08`，16 位地址（`ptr & 0x7FFF`）
  - RX Buffer 访问：控制字节 `0x18`，16 位地址（`ptr & 0x7FFF`）
- 所有 SPI 操作必须有超时退出，不得死锁
- CS 引脚软件控制（`HAL_GPIO_WritePin`），不用硬件 NSS

### I2C 通信规则

- OLED 地址 `0x78`（7 位地址 `0x3C` 左移一位）
- 所有 I2C 操作必须有超时（HAL 层面已支持）
- 多任务访问 I2C 时需要互斥锁

### DHT22 时序规则

- 使用 DWT 周期计数器做微秒级延时（不用 HAL_Delay）
- 主机拉低 >= 2ms 启动信号
- 采样间隔 >= 2s
- 读取失败时返回错误码，不阻塞

## 编码规范

### 代码风格

- 遵循 CubeMX 生成代码的风格（HAL 回调命名 `HAL_*_Callback`）
- 错误处理用 `Error_Handler()`（死循环 + 关中断），不静默忽略错误
- 中断处理函数在 `stm32f4xx_it.c` 中，不在 CubeMX 框架外添加自定义 ISR
- 变量命名遵循 HAL 风格：`hxxx` 为句柄，`xxxHandle` 为 FreeRTOS 任务句柄

### 添加新功能的流程

1. 新源文件放 `Core/Src/`，头文件放 `Core/Inc/`
2. 在 `cmake/stm32cubemx/CMakeLists.txt` 的 `MX_Application_Src` 中注册新 `.c` 文件
3. 新 FreeRTOS 任务在 `freertos.c` 的 `MX_FREERTOS_Init()` 中创建（USER CODE 区域内）
4. 外设配置变更必须先通过 CubeMX `.ioc` 文件修改，再重新生成代码
5. 新增共享资源时，在 dev-doc.md 的资源跟踪表中记录

### 构建

```bash
cmake --preset Debug
cmake --build build/Debug
```

改完代码后必须跑构建验证。构建失败不提交。

### 提交规范

- 格式：`<type>(<scope>): <description>`
- 类型：feat, fix, docs, style, refactor, test, chore
- scope 示例：w5500, dht22, oled, http, freertos, gpio
- 描述用英文，简洁说明改动目的
- 不自动 push，除非明确要求

## 输出格式

- 默认中文，代码、命令、变量名、技术术语用英文
- 结论先行，再给理由
- 只输出改动部分，不重复完整文件
- 代码输出用代码块，标注语言类型
- 嵌入式代码审查重点关注：内存越界、栈溢出、中断安全、外设超时、时序正确性
