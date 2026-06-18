/**
 * @file    w5500.h
 * @brief   W5500 以太网控制器驱动 (SPI)
 */

#ifndef __W5500_H
#define __W5500_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

/* W5500 SPI 帧头: 控制字节 = BSB[4:0]<<3 | RWB | OM[1:0]
 * BSB 编码 (左移3位后):
 *   Common:  0x00
 *   sock=0: REG=0x08, TX=0x10, RX=0x18
 *   sock=1: REG=0x20, TX=0x28, RX=0x30
 *   sock=n: REG=(3n+1)<<3 */
#define W5500_BSB_COMMON       (0x00 << 3)  /* 通用寄存器 */
#define W5500_BSB_SOCK_REG(s)  ((3 * (s) + 1) << 3)  /* Socket s 寄存器 */
#define W5500_BSB_SOCK_TX(s)   ((3 * (s) + 2) << 3)  /* Socket s TX 缓冲区 */
#define W5500_BSB_SOCK_RX(s)   ((3 * (s) + 3) << 3)  /* Socket s RX 缓冲区 */

#define W5500_READ          0x00
#define W5500_WRITE         0x04

/* ---- 通用寄存器地址 ---- */
#define W5500_MR            0x0000  /* Mode Register */
#define W5500_GAR           0x0001  /* Gateway Address (4B) */
#define W5500_SUBR          0x0005  /* Subnet Mask (4B) */
#define W5500_SHAR          0x0009  /* Source MAC (6B) */
#define W5500_SIPR          0x000F  /* Source IP (4B) */
#define W5500_PHYCFGR       0x002E  /* PHY Configuration */

/* ---- Socket 寄存器地址 (偏移) ---- */
#define W5500_Sn_MR         0x0000
#define W5500_Sn_CR         0x0001
#define W5500_Sn_SR         0x0003
#define W5500_Sn_PORT       0x0004
#define W5500_Sn_DIPR       0x000C
#define W5500_Sn_DPORT      0x0010
#define W5500_Sn_TXMEM_SIZE 0x001E  /* TX 缓冲区大小 (KB) */
#define W5500_Sn_RXMEM_SIZE 0x001F  /* RX 缓冲区大小 (KB) */
#define W5500_Sn_TX_FSR     0x0020
#define W5500_Sn_TX_RD      0x0022
#define W5500_Sn_TX_WR      0x0024
#define W5500_Sn_RX_RSR     0x0026
#define W5500_Sn_RX_RD      0x0028

/* Socket 命令 */
#define W5500_Sn_CR_OPEN    0x01
#define W5500_Sn_CR_LISTEN  0x02
#define W5500_Sn_CR_CONNECT 0x04
#define W5500_Sn_CR_DISCON  0x08
#define W5500_Sn_CR_SEND    0x20
#define W5500_Sn_CR_RECV    0x40

/* Socket 状态 */
#define W5500_Sn_SR_CLOSED      0x00
#define W5500_Sn_SR_INIT        0x13
#define W5500_Sn_SR_LISTEN      0x14
#define W5500_Sn_SR_ESTABLISHED 0x17
#define W5500_Sn_SR_CLOSE_WAIT  0x1C
#define W5500_Sn_SR_SOCK_TCP    0x17

/* Socket 模式 */
#define W5500_Sn_MR_TCP     0x01
#define W5500_Sn_MR_UDP     0x02

/* 网络配置结构体 */
typedef struct {
    uint8_t mac[6];
    uint8_t ip[4];
    uint8_t subnet[4];
    uint8_t gateway[4];
} W5500_Config;

/* API */
void     W5500_Reset(void);
uint8_t  W5500_Init(const W5500_Config *cfg);
uint8_t  W5500_GetPHYStatus(void);  /* 1=link up, 0=link down */
uint8_t  W5500_GetVersion(void);    /* 应返回 0x04 */

/* 寄存器读写 */
uint8_t  W5500_ReadReg(uint16_t addr, uint8_t bsb);
void     W5500_WriteReg(uint16_t addr, uint8_t bsb, uint8_t data);
uint16_t W5500_ReadReg16(uint16_t addr, uint8_t bsb);
void     W5500_WriteReg16(uint16_t addr, uint8_t bsb, uint16_t data);
void     W5500_ReadBuf(uint16_t addr, uint8_t bsb, uint8_t *buf, uint16_t len);
void     W5500_WriteBuf(uint16_t addr, uint8_t bsb, const uint8_t *buf, uint16_t len);

/* Socket TCP 操作 */
uint8_t  W5500_TCP_Open(uint8_t sock, uint16_t port);
uint8_t  W5500_TCP_Listen(uint8_t sock);
uint8_t  W5500_TCP_Connect(uint8_t sock, const uint8_t *ip, uint16_t port);
uint8_t  W5500_GetSocketStatus(uint8_t sock);
uint16_t W5500_TCP_Send(uint8_t sock, const uint8_t *buf, uint16_t len);
uint16_t W5500_TCP_Recv(uint8_t sock, uint8_t *buf, uint16_t maxlen);
void     W5500_TCP_Close(uint8_t sock);

#endif /* __W5500_H */
