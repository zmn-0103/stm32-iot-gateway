/**
 * @file    dht22.c
 * @brief   DHT22 (AM2302) 温湿度传感器驱动实现
 *
 * 协议时序 (DHT22 datasheet):
 *   1. 主机拉低 >= 2ms (启动信号)
 *   2. 主机释放 (拉高 20~40us)
 *   3. DHT22 响应: 低 80us + 高 80us
 *   4. 数据位: 低 50us + 高 26~28us (0) 或 高 ~70us (1)
 *   5. 共 40 位: 湿度高8 + 湿度低8 + 温度高8 + 温度低8 + 校验和
 *
 * STM32F407 @ 168MHz: 1us = 168 个 DWT 周期
 */

#include "dht22.h"
#include "main.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>

/* 引脚定义 (来自 main.h) */
#define DHT22_PORT   DHT22_DATA_GPIO_Port   /* GPIOB */
#define DHT22_PIN    DHT22_DATA_Pin          /* GPIO_PIN_1 */

/* DWT 延时常数: 168MHz → 1us = 168 cycles */
#define CYCLES_PER_US   (SystemCoreClock / 1000000)

/* 超时阈值 (微秒) */
#define TIMEOUT_US      200     /* 通用超时 */
#define START_LOW_MS    2       /* 主机拉低时间 >= 2ms */
#define RESPONSE_WAIT   100     /* 等待 DHT22 响应 */

/* 最小读取间隔 (毫秒) */
#define MIN_INTERVAL_MS 2000

/* 上次读取时间戳，用于间隔控制 */
static uint32_t last_read_tick = 0;

/* ------------------------------------------------------------------ */
/* DWT 微秒级延时                                                     */
/* ------------------------------------------------------------------ */

/**
 * @brief  初始化 DWT 周期计数器
 */
void DHT22_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

/**
 * @brief  微秒级延时 (忙等)
 * @param  us  延时微秒数
 */
static inline void delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * CYCLES_PER_US;
    while ((DWT->CYCCNT - start) < ticks)
        ;
}

/* ------------------------------------------------------------------ */
/* GPIO 模式切换                                                       */
/* ------------------------------------------------------------------ */

/**
 * @brief  将 DHT22 数据引脚切换为推挽输出
 */
static void DHT22_SetOutput(void)
{
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin   = DHT22_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DHT22_PORT, &gpio);
}

/**
 * @brief  将 DHT22 数据引脚切换为浮空输入
 */
static void DHT22_SetInput(void)
{
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin  = DHT22_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DHT22_PORT, &gpio);
}

/**
 * @brief  读取数据引脚电平
 */
static inline uint8_t DHT22_ReadPin(void)
{
    return (uint8_t)HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN);
}

/* ------------------------------------------------------------------ */
/* 超时等待辅助函数                                                    */
/* ------------------------------------------------------------------ */

/**
 * @brief  等待引脚变为指定电平
 * @param  level  期望电平 (0 或 1)
 * @param  timeout_us  超时时间 (微秒)
 * @retval 0=成功, -1=超时
 */
static int wait_for_level(uint8_t level, uint32_t timeout_us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = timeout_us * CYCLES_PER_US;
    while (DHT22_ReadPin() != level) {
        if ((DWT->CYCCNT - start) >= ticks)
            return -1;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* 核心读取函数                                                        */
/* ------------------------------------------------------------------ */

DHT22_Status DHT22_Read(DHT22_Data *data)
{
    uint8_t raw[5] = {0};  /* 5 字节原始数据 */
    uint32_t start, ticks;

    if (data == NULL)
        return DHT22_ERR_TIMEOUT;

    data->valid = 0;

    /* 检查读取间隔 (>= 2s) */
    uint32_t now = HAL_GetTick();
    if ((now - last_read_tick) < MIN_INTERVAL_MS && last_read_tick != 0) {
        return DHT22_ERR_BUSY;
    }

    /* ============================================================
     * 1. 发送启动信号: 主机拉低 >= 2ms
     * ============================================================ */
    __disable_irq();  /* 进入临界区，禁止中断 */

    DHT22_SetOutput();
    HAL_GPIO_WritePin(DHT22_PORT, DHT22_PIN, GPIO_PIN_RESET);
    delay_us(2000);  /* 拉低 2ms (用 DWT, 关中断下可用) */

    /* 释放总线 (拉高 20~40us) */
    HAL_GPIO_WritePin(DHT22_PORT, DHT22_PIN, GPIO_PIN_SET);
    delay_us(30);

    /* 切换为输入，等待 DHT22 响应 */
    DHT22_SetInput();

    /* ============================================================
     * 2. 等待 DHT22 响应: 低 80us + 高 80us
     * ============================================================ */
    /* 等待 DHT22 拉低 (响应开始) */
    if (wait_for_level(0, RESPONSE_WAIT) != 0) {
        __enable_irq();
        last_read_tick = HAL_GetTick();
        return DHT22_ERR_TIMEOUT;
    }

    /* 等待 DHT22 拉高 (响应结束) */
    if (wait_for_level(1, RESPONSE_WAIT) != 0) {
        __enable_irq();
        last_read_tick = HAL_GetTick();
        return DHT22_ERR_TIMEOUT;
    }

    /* 等待 DHT22 再次拉高结束 (数据传输开始) */
    if (wait_for_level(0, RESPONSE_WAIT) != 0) {
        __enable_irq();
        last_read_tick = HAL_GetTick();
        return DHT22_ERR_TIMEOUT;
    }

    /* ============================================================
     * 3. 读取 40 位数据
     *    每位: 低 50us + 高 x us
     *    高电平持续时间: ~28us = 0, ~70us = 1
     * ============================================================ */
    for (int i = 0; i < 40; i++) {
        /* 等待低电平结束 (每位开始的 50us 低电平) */
        if (wait_for_level(1, TIMEOUT_US) != 0) {
            __enable_irq();
            last_read_tick = HAL_GetTick();
            return DHT22_ERR_TIMEOUT;
        }

        /* 测量高电平持续时间 */
        start = DWT->CYCCNT;

        /* 等待高电平结束 */
        if (wait_for_level(0, TIMEOUT_US) != 0) {
            __enable_irq();
            last_read_tick = HAL_GetTick();
            return DHT22_ERR_TIMEOUT;
        }

        /* 高电平持续时间 > 40us → 该位为 1 */
        ticks = DWT->CYCCNT - start;
        if (ticks > (40 * CYCLES_PER_US)) {
            raw[i / 8] |= (1 << (7 - (i % 8)));
        }
    }

    __enable_irq();  /* 退出临界区 */

    /* ============================================================
     * 4. 校验和验证
     *    校验和 = (湿度高 + 湿度低 + 温度高 + 温度低) & 0xFF
     * ============================================================ */
    uint8_t checksum = (raw[0] + raw[1] + raw[2] + raw[3]) & 0xFF;
    if (checksum != raw[4]) {
        last_read_tick = HAL_GetTick();
        return DHT22_ERR_CHECKSUM;
    }

    /* ============================================================
     * 5. 解析数据
     *    湿度: raw[0]*256 + raw[1] (单位 0.1%RH)
     *    温度: raw[2]*256 + raw[3] (单位 0.1°C, bit15=符号位)
     * ============================================================ */
    uint16_t raw_humidity = ((uint16_t)raw[0] << 8) | raw[1];
    uint16_t raw_temp = ((uint16_t)raw[2] << 8) | raw[3];

    data->humidity = (float)raw_humidity / 10.0f;

    if (raw_temp & 0x8000) {
        /* 负温度: 清除符号位 */
        raw_temp &= 0x7FFF;
        data->temperature = -(float)raw_temp / 10.0f;
    } else {
        data->temperature = (float)raw_temp / 10.0f;
    }

    data->valid = 1;
    last_read_tick = HAL_GetTick();

    /* 调试: 打印原始字节 */
    {
        extern UART_HandleTypeDef huart3;
        char dbg[32];
        int n = snprintf(dbg, sizeof(dbg), "raw:%02X%02X%02X%02X%02X\r\n",
                         raw[0], raw[1], raw[2], raw[3], raw[4]);
        HAL_UART_Transmit(&huart3, (uint8_t *)dbg, n, 100);
    }

    return DHT22_OK;
}
