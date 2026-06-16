# STM32 IoT Gateway - 开发文档

## 一、硬件约束

### MCU

- STM32F407VET6, Cortex-M4F, 168MHz, LQFP100
- HSE = 25MHz, SYSCLK = 168MHz, APB1 = 42MHz, APB2 = 84MHz
- FreeRTOS heap = 30720B (`.ioc` 锁定，不可改)

### 外设引脚分配

| 外设 | 引脚 | 用途 | 状态 |
| --- | --- | --- | --- |
| SPI1 | SCK=PA5, MISO=PA6, MOSI=PA7 | W5500 数据通信 | 已用 |
| GPIO | PA4 | W5500 CS (软件控制) | 已用 |
| GPIO | PB0 | W5500 RST | 已用 |
| GPIO | PB1 | DHT22 DATA (上拉输入) | 已用 |
| I2C1 | SCL=PB6, SDA=PB7 | 预留 | 已用 |
| USART3 | TX=PB10, RX=PB11 | 调试串口 | 已用 |
| GPIO | PC13 | LED 指示灯 | 已用 |
| SWD | PA13, PA14 | 调试烧录 | 不可占用 |

### 中断优先级

| 中断 | 优先级 | 说明 |
| --- | --- | --- |
| PendSV | 15 (最低) | FreeRTOS 任务切换 |
| SysTick | 15 | FreeRTOS 时基 |
| TIM6_DAC | 15 | CubeMX 时基 |

NVIC Priority Group = 4 (4位抢占优先级，0位子优先级)。FreeRTOS 任务优先级范围 0-31，映射到 NVIC 抢占优先级。

### FreeRTOS 任务栈分配

| 任务 | 优先级 | 栈大小 | 用途 |
| --- | --- | --- | --- |
| NetworkTask | AboveNormal (6) | 4096B | W5500 以太网通信 |
| SensorTask | Normal (4) | 1024B | DHT22 传感器读取 |
| DisplayTask | BelowNormal (2) | 1024B | 显示输出 |
| LEDTask | Low (1) | 512B | LED 心跳 |

栈总分配: 4096 + 1024 + 1024 + 512 = 6656B (不含 FreeRTOS 自身开销)

---

## 二、功能模块

### 模块 1: DHT22 温湿度传感器

- **任务**: SensorTask
- **接口**: PB1, 单总线协议 (需要精确时序)
- **数据**: 温度 (16bit), 湿度 (16bit), 校验和 (8bit)
- **时序要求**: 主机拉低 >= 1ms 启动, DHT22 响应 80us 低 + 80us 高, 数据 50us 低 + 26/70us 高
- **轮询间隔**: >= 2s (DHT22 最小采样间隔)
- **输出**: 温湿度数据结构体，通过队列传递给 DisplayTask/NetworkTask
- **状态**: 待开发

### 模块 2: W5500 以太网

- **任务**: NetworkTask
- **接口**: SPI1 (5.25MHz), CS=PA4, RST=PB0
- **协议**: TCP/IP (W5500 内置硬件协议栈)
- **功能**: DHCP 获取 IP, 建立 TCP/UDP 连接, 上传传感器数据
- **依赖**: 无 (SPI1 已由 CubeMX 初始化)
- **状态**: 待开发

### 模块 3: 显示输出

- **任务**: DisplayTask
- **接口**: 待定 (可能是 I2C1 接 OLED, 或 USART3 输出)
- **功能**: 显示温湿度、网络状态、系统运行时间
- **状态**: 待开发

### 模块 4: LED 心跳

- **任务**: LEDTask
- **接口**: PC13
- **功能**: 500ms 翻转, 指示系统运行
- **状态**: 已完成 (CubeMX 生成, 已有实现)

---

## 三、模块间通信

| 数据流 | 方式 | 说明 |
| --- | --- | --- |
| SensorTask -> DisplayTask | 队列 | 温湿度数据 |
| SensorTask -> NetworkTask | 队列 | 温湿度数据 |
| 所有任务 -> LEDTask | 无 | LED 仅做心跳, 不依赖其他任务 |

队列在 `MX_FREERTOS_Init()` 的 `USER CODE BEGIN RTOS_QUEUES` 区域创建。

---

## 四、资源占用跟踪

每个模块开发完成后填写此表。集成前检查资源冲突。

| 模块 | GPIO | SPI/I2C/UART | 定时器 | 实际栈峰值 | 堆分配 | 备注 |
| --- | --- | --- | --- | --- | --- | --- |
| LEDTask | PC13 | - | - | - | 0 | 填写后删除此行 |
| DHT22 | PB1 | - | - | - | 0 | 填写后删除此行 |
| W5500 | PA4, PB0 | SPI1 | - | - | 0 | 填写后删除此行 |
| Display | - | 待定 | - | - | 0 | 填写后删除此行 |

**检查规则**:
- GPIO 不可重复分配 (SWD 引脚 PA13/PA14 绝对不可占用)
- SPI/I2C 不可同时被两个模块使用
- 栈峰值不可超过分配值的 80%
- 堆分配总和不可接近 30720B

---

## 五、冒烟测试

所有模块开发完成后，集成前执行。只验证稳定性，不验证功能正确性。

### 测试项

- [ ] 连续运行 10 分钟无 HardFault
- [ ] `uxTaskGetStackHighWaterMark()` 每个任务栈余量 > 20%
- [ ] `xPortGetFreeHeapSize()` 连续读取 5 分钟无持续下降 (无内存泄漏)
- [ ] SPI 通信有超时退出 (不会死锁)
- [ ] I2C 通信有超时退出 (不会死锁)
- [ ] DHT22 读取失败时不会阻塞其他任务
- [ ] W5500 网络异常时不会阻塞其他任务
- [ ] LED 心跳稳定跳动

### 测试方法

在 NetworkTask 中添加周期性日志输出 (USART3), 记录:
```c
// 在 NetworkTask 主循环中, 每 30s 输出一次
char buf[128];
snprintf(buf, sizeof(buf),
    "Heap:%lu Stack[Net:%lu Sen:%lu Disp:%lu LED:%lu]\r\n",
    xPortGetFreeHeapSize(),
    uxTaskGetStackHighWaterMark(NetworkTaskHandle),
    uxTaskGetStackHighWaterMark(SensorTaskHandle),
    uxTaskGetStackHighWaterMark(DisplayTaskHandle),
    uxTaskGetStackHighWaterMark(LEDTaskHandle));
HAL_UART_Transmit(&huart3, (uint8_t*)buf, strlen(buf), 100);
```

### 通过标准

| 指标 | 阈值 |
| --- | --- |
| HardFault 次数 | 0 |
| 最小栈余量 | > 20% 分配值 |
| 堆变化幅度 | 连续 5min 内波动 < 512B |
| 外设超时触发 | 记录, 不阻塞 |

---

## 六、开发进度

| 模块 | 文档 | 编码 | 单元验证 | 资源记录 | 冒烟测试 |
| --- | --- | --- | --- | --- | --- |
| DHT22 | - | - | - | - | - |
| W5500 | - | - | - | - | - |
| Display | - | - | - | - | - |
| LED | - | - | - | - | - |

状态标记: `-` 未开始, `WIP` 进行中, `OK` 完成, `BLOCK` 阻塞
