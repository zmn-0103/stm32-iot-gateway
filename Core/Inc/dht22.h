/**
 * @file    dht22.h
 * @brief   DHT22 (AM2302) 温湿度传感器驱动
 * @note    单总线协议，使用 DWT 周期计数器做微秒级延时
 */

#ifndef __DHT22_H
#define __DHT22_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>

/* 返回状态码 */
typedef enum {
    DHT22_OK = 0,           /* 读取成功 */
    DHT22_ERR_TIMEOUT,      /* 超时：DHT22 未响应 */
    DHT22_ERR_CHECKSUM,     /* 校验和错误 */
    DHT22_ERR_BUSY,         /* 传感器忙（间隔不足 2s） */
} DHT22_Status;

/* 传感器数据 */
typedef struct {
    float    temperature;   /* 温度 (°C)，范围 -40 ~ 80 */
    float    humidity;      /* 湿度 (%RH)，范围 0 ~ 100 */
    uint8_t  valid;         /* 1=数据有效，0=上次读取失败 */
} DHT22_Data;

/**
 * @brief  初始化 DWT 延时系统
 * @note   必须在使用 DHT22_Read 之前调用一次
 */
void DHT22_Init(void);

/**
 * @brief  读取 DHT22 温湿度数据
 * @param  data  输出数据指针
 * @retval DHT22_Status 状态码
 * @note   两次调用间隔必须 >= 2s，否则返回 DHT22_ERR_BUSY
 *         读取期间会短暂关中断 (~5ms, 使用 __disable_irq)
 */
DHT22_Status DHT22_Read(DHT22_Data *data);

#ifdef __cplusplus
}
#endif

#endif /* __DHT22_H */
