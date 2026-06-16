/**
 * @file    ssd1306.h
 * @brief   SSD1306 0.96" OLED 驱动 (128x64, I2C, 硬件 I2C + 8x16 字体)
 */

#ifndef __SSD1306_H
#define __SSD1306_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include <stdint.h>

/**
 * @brief  初始化 SSD1306 OLED
 * @param  hi2c  I2C 句柄指针
 */
void OLED_Init(I2C_HandleTypeDef *hi2c);

/**
 * @brief  清屏
 */
void OLED_Clear(void);

/**
 * @brief  将缓冲区数据刷新到屏幕
 */
void OLED_Update(void);

/**
 * @brief  显示一个字符
 * @param  Line   行位置，范围：1~4
 * @param  Column 列位置，范围：1~16
 * @param  Char   要显示的字符，范围：ASCII 可见字符
 */
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);

/**
 * @brief  显示字符串
 * @param  Line   起始行位置，范围：1~4
 * @param  Column 起始列位置，范围：1~16
 * @param  String 要显示的字符串
 */
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String);

/**
 * @brief  显示数字（十进制，正数）
 * @param  Line   行位置，范围：1~4
 * @param  Column 列位置，范围：1~16
 * @param  Number 数值，范围：0~4294967295
 * @param  Length 数字位数，范围：1~10
 */
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);

/**
 * @brief  显示数字（十进制，带符号）
 * @param  Line   行位置，范围：1~4
 * @param  Column 列位置，范围：1~16
 * @param  Number 数值，范围：-2147483648~2147483647
 * @param  Length 数字位数（不含符号），范围：1~10
 */
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length);

/**
 * @brief  显示浮点数（保留 1 位小数）
 * @param  Line   行位置，范围：1~4
 * @param  Column 列位置，范围：1~16
 * @param  value  浮点数值
 */
void OLED_ShowFloat(uint8_t Line, uint8_t Column, float value);

#ifdef __cplusplus
}
#endif

#endif /* __SSD1306_H */
