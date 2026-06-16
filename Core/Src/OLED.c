/**
 * @file    OLED.c
 * @brief   SSD1306 OLED 驱动 (硬件 I2C1, PB6 SCL / PB7 SDA, 8x16 字体)
 * @note    移植自江科大源码，改用 STM32F4 HAL 硬件 I2C
 */

#include "OLED.h"
#include "OLED_Font.h"
#include "main.h"
#include <stdio.h>

/* SSD1306 I2C 地址 (7-bit: 0x3C, HAL 自动左移) */
#define OLED_I2C_ADDR   0x3C

/* 超时 (ms) */
#define OLED_I2C_TIMEOUT  100

extern I2C_HandleTypeDef hi2c1;

static void OLED_WriteCommand(uint8_t Command)
{
    HAL_I2C_Mem_Write(&hi2c1, OLED_I2C_ADDR << 1, 0x00,
                      I2C_MEMADD_SIZE_8BIT, &Command, 1, OLED_I2C_TIMEOUT);
}

static void OLED_WriteData(uint8_t Data)
{
    HAL_I2C_Mem_Write(&hi2c1, OLED_I2C_ADDR << 1, 0x40,
                      I2C_MEMADD_SIZE_8BIT, &Data, 1, OLED_I2C_TIMEOUT);
}

static void OLED_SetCursor(uint8_t Y, uint8_t X)
{
    OLED_WriteCommand(0xB0 | Y);
    OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4));
    OLED_WriteCommand(0x00 | (X & 0x0F));
}

void OLED_Clear(void)
{
    uint8_t i, j;
    for (j = 0; j < 8; j++)
    {
        OLED_SetCursor(j, 0);
        for (i = 0; i < 128; i++)
        {
            OLED_WriteData(0x00);
        }
    }
}

void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{
    uint8_t i;
    OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8);
    for (i = 0; i < 8; i++)
    {
        OLED_WriteData(OLED_F8x16[Char - ' '][i]);
    }
    OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8);
    for (i = 0; i < 8; i++)
    {
        OLED_WriteData(OLED_F8x16[Char - ' '][i + 8]);
    }
}

void OLED_ShowString(uint8_t Line, uint8_t Column, char *String)
{
    uint8_t i;
    for (i = 0; String[i] != '\0'; i++)
    {
        OLED_ShowChar(Line, Column + i, String[i]);
    }
}

static uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;
    while (Y--) Result *= X;
    return Result;
}

void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i++)
    {
        OLED_ShowChar(Line, Column + i, Number / OLED_Pow(10, Length - i - 1) % 10 + '0');
    }
}

void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
{
    uint8_t i;
    uint32_t Number1;
    if (Number >= 0)
    {
        OLED_ShowChar(Line, Column, '+');
        Number1 = Number;
    }
    else
    {
        OLED_ShowChar(Line, Column, '-');
        Number1 = -Number;
    }
    for (i = 0; i < Length; i++)
    {
        OLED_ShowChar(Line, Column + i + 1, Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0');
    }
}

void OLED_ShowFloat(uint8_t Line, uint8_t Column, float value)
{
    char buf[12];
    snprintf(buf, sizeof(buf), "%.1f", (double)value);
    OLED_ShowString(Line, Column, buf);
}

void OLED_Init(void)
{
    /* 硬件 I2C1 已在 MX_I2C1_Init() 中初始化，无需再配置引脚 */
    HAL_Delay(100);  /* OLED 上电稳定延时 */

    OLED_WriteCommand(0xAE);
    OLED_WriteCommand(0xD5);
    OLED_WriteCommand(0x80);
    OLED_WriteCommand(0xA8);
    OLED_WriteCommand(0x3F);
    OLED_WriteCommand(0xD3);
    OLED_WriteCommand(0x00);
    OLED_WriteCommand(0x40);
    OLED_WriteCommand(0xA1);
    OLED_WriteCommand(0xC8);
    OLED_WriteCommand(0xDA);
    OLED_WriteCommand(0x12);
    OLED_WriteCommand(0x81);
    OLED_WriteCommand(0xCF);
    OLED_WriteCommand(0xD9);
    OLED_WriteCommand(0xF1);
    OLED_WriteCommand(0xDB);
    OLED_WriteCommand(0x30);
    OLED_WriteCommand(0xA4);
    OLED_WriteCommand(0xA6);
    OLED_WriteCommand(0x8D);
    OLED_WriteCommand(0x14);
    OLED_WriteCommand(0xAF);

    OLED_Clear();
}
