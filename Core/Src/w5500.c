/**
 * @file    w5500.c
 * @brief   W5500 以太网控制器驱动实现
 *
 * 硬件连接:
 *   PA4 → W5500_CS   (SPI 片选, 低有效)
 *   PB0 → W5500_RST  (复位, 低有效)
 *   PA5 → SPI1_SCK
 *   PA6 → SPI1_MISO
 *   PA7 → SPI1_MOSI
 *
 * SPI 协议 (W5500 变种):
 *   字节1: 地址高 8 位
 *   字节2: 地址低 8 位
 *   字节3: 控制字节 = BSB[4:0] | RWB | OMB[1:0]
 *   字节4~N: 数据 (读/写)
 */

#include "w5500.h"
#include "main.h"
#include <string.h>

extern SPI_HandleTypeDef hspi1;

/* ---- CS / RST 控制 ---- */
#define W5500_CS_LOW()   HAL_GPIO_WritePin(W5500_CS_GPIO_Port, W5500_CS_Pin, GPIO_PIN_RESET)
#define W5500_CS_HIGH()  HAL_GPIO_WritePin(W5500_CS_GPIO_Port, W5500_CS_Pin, GPIO_PIN_SET)
#define W5500_RST_LOW()  HAL_GPIO_WritePin(W5500_RST_GPIO_Port, W5500_RST_Pin, GPIO_PIN_RESET)
#define W5500_RST_HIGH() HAL_GPIO_WritePin(W5500_RST_GPIO_Port, W5500_RST_Pin, GPIO_PIN_SET)

#define W5500_SPI_TIMEOUT  100

/* ---- 底层 SPI 收发 ---- */

static void W5500_SPI_WriteByte(uint8_t data)
{
    uint8_t rx;
    HAL_SPI_TransmitReceive(&hspi1, &data, &rx, 1, W5500_SPI_TIMEOUT);
}

static uint8_t W5500_SPI_ReadByte(void)
{
    uint8_t tx = 0xFF, rx;
    HAL_SPI_TransmitReceive(&hspi1, &tx, &rx, 1, W5500_SPI_TIMEOUT);
    return rx;
}

/* ---- 寄存器读写 ---- */

uint8_t W5500_ReadReg(uint16_t addr, uint8_t bsb)
{
    uint8_t val;
    W5500_CS_LOW();
    W5500_SPI_WriteByte((addr >> 8) & 0xFF);
    W5500_SPI_WriteByte(addr & 0xFF);
    W5500_SPI_WriteByte(bsb | W5500_READ);
    val = W5500_SPI_ReadByte();
    W5500_CS_HIGH();
    return val;
}

void W5500_WriteReg(uint16_t addr, uint8_t bsb, uint8_t data)
{
    W5500_CS_LOW();
    W5500_SPI_WriteByte((addr >> 8) & 0xFF);
    W5500_SPI_WriteByte(addr & 0xFF);
    W5500_SPI_WriteByte(bsb | W5500_WRITE);
    W5500_SPI_WriteByte(data);
    W5500_CS_HIGH();
}

uint16_t W5500_ReadReg16(uint16_t addr, uint8_t bsb)
{
    uint16_t val;
    val  = (uint16_t)W5500_ReadReg(addr, bsb) << 8;
    val |= W5500_ReadReg(addr + 1, bsb);
    return val;
}

void W5500_WriteReg16(uint16_t addr, uint8_t bsb, uint16_t data)
{
    W5500_WriteReg(addr,     bsb, (data >> 8) & 0xFF);
    W5500_WriteReg(addr + 1, bsb, data & 0xFF);
}

void W5500_ReadBuf(uint16_t addr, uint8_t bsb, uint8_t *buf, uint16_t len)
{
    W5500_CS_LOW();
    W5500_SPI_WriteByte((addr >> 8) & 0xFF);
    W5500_SPI_WriteByte(addr & 0xFF);
    W5500_SPI_WriteByte(bsb | W5500_READ);
    HAL_SPI_Receive(&hspi1, buf, len, W5500_SPI_TIMEOUT);
    W5500_CS_HIGH();
}

void W5500_WriteBuf(uint16_t addr, uint8_t bsb, const uint8_t *buf, uint16_t len)
{
    W5500_CS_LOW();
    W5500_SPI_WriteByte((addr >> 8) & 0xFF);
    W5500_SPI_WriteByte(addr & 0xFF);
    W5500_SPI_WriteByte(bsb | W5500_WRITE);
    HAL_SPI_Transmit(&hspi1, (uint8_t *)buf, len, W5500_SPI_TIMEOUT);
    W5500_CS_HIGH();
}

/* ---- 复位 ---- */

void W5500_Reset(void)
{
    W5500_RST_LOW();
    HAL_Delay(2);
    W5500_RST_HIGH();
    HAL_Delay(10);
}

/* ---- 初始化 ---- */

uint8_t W5500_Init(const W5500_Config *cfg)
{
    W5500_Reset();

    /* 软复位 */
    W5500_WriteReg(W5500_MR, W5500_BSB_COMMON, 0x80);
    HAL_Delay(50);

    /* 等待复位完成 */
    for (int i = 0; i < 100; i++) {
        if ((W5500_ReadReg(W5500_MR, W5500_BSB_COMMON) & 0x80) == 0)
            break;
        HAL_Delay(1);
    }

    /* 检查芯片版本 (应为 0x04) */
    uint8_t ver = W5500_GetVersion();
    if (ver != 0x04)
        return 0;

    /* 配置网络参数 */
    W5500_WriteBuf(W5500_SHAR, W5500_BSB_COMMON, cfg->mac, 6);
    W5500_WriteBuf(W5500_SIPR, W5500_BSB_COMMON, cfg->ip, 4);
    W5500_WriteBuf(W5500_SUBR, W5500_BSB_COMMON, cfg->subnet, 4);
    W5500_WriteBuf(W5500_GAR,  W5500_BSB_COMMON, cfg->gateway, 4);

    /* 初始化 Socket 缓冲区: 共 16KB TX + 16KB RX
     * Socket 0: 8KB TX + 8KB RX
     * Socket 1~7: 各 1KB TX + 1KB RX (8×1KB = 8KB) */
    for (uint8_t s = 0; s < 8; s++) {
        uint8_t bsb = W5500_BSB_SOCK0_REG | (s << 2);
        W5500_WriteReg(W5500_Sn_TXMEM_SIZE, bsb, (s == 0) ? 8 : 1);
        W5500_WriteReg(W5500_Sn_RXMEM_SIZE, bsb, (s == 0) ? 8 : 1);
    }

    return 1;
}

/* ---- PHY 状态 ---- */

uint8_t W5500_GetPHYStatus(void)
{
    return (W5500_ReadReg(W5500_PHYCFGR, W5500_BSB_COMMON) & 0x01);
}

uint8_t W5500_GetVersion(void)
{
    return W5500_ReadReg(0x0039, W5500_BSB_COMMON);  /* VERSIONR */
}

/* ---- Socket TCP 操作 ---- */

uint8_t W5500_TCP_Open(uint8_t sock, uint16_t port)
{
    uint8_t bsb = W5500_BSB_SOCK0_REG | (sock << 2);

    /* 关闭 */
    W5500_WriteReg(W5500_Sn_CR, bsb, W5500_Sn_CR_DISCON);
    W5500_WriteReg(W5500_Sn_CR, bsb, 0x00);

    /* TCP 模式 */
    W5500_WriteReg(W5500_Sn_MR, bsb, W5500_Sn_MR_TCP);

    /* 本地端口 */
    W5500_WriteReg16(W5500_Sn_PORT, bsb, port);

    /* 打开 */
    W5500_WriteReg(W5500_Sn_CR, bsb, W5500_Sn_CR_OPEN);
    HAL_Delay(1);

    /* 检查状态 */
    uint8_t sr = W5500_ReadReg(W5500_Sn_SR, bsb);
    if (sr != W5500_Sn_SR_INIT) {
        W5500_WriteReg(W5500_Sn_CR, bsb, 0x00);
        return 0;
    }

    return 1;
}

uint8_t W5500_TCP_Connect(uint8_t sock, const uint8_t *ip, uint16_t port)
{
    uint8_t bsb = W5500_BSB_SOCK0_REG | (sock << 2);

    /* 目标 IP 和端口 */
    W5500_WriteBuf(W5500_Sn_DIPR, bsb, ip, 4);
    W5500_WriteReg16(W5500_Sn_DPORT, bsb, port);

    /* 发起连接 */
    W5500_WriteReg(W5500_Sn_CR, bsb, W5500_Sn_CR_CONNECT);

    return 1;
}

uint16_t W5500_TCP_Send(uint8_t sock, const uint8_t *buf, uint16_t len)
{
    uint8_t bsb = W5500_BSB_SOCK0_REG | (sock << 2);
    uint8_t txb = W5500_BSB_SOCK0_TX | (sock << 2);

    /* 检查 TX 空闲大小 */
    uint16_t free_size = W5500_ReadReg16(W5500_Sn_TX_FSR, bsb);
    if (len > free_size)
        len = free_size;

    /* 写入 TX 缓冲区 */
    uint16_t wr = W5500_ReadReg16(W5500_Sn_TX_WR, bsb);
    W5500_WriteBuf(wr, txb, buf, len);

    /* 更新写指针并发送 */
    W5500_WriteReg16(W5500_Sn_TX_WR, bsb, wr + len);
    W5500_WriteReg(W5500_Sn_CR, bsb, W5500_Sn_CR_SEND);

    return len;
}

uint16_t W5500_TCP_Recv(uint8_t sock, uint8_t *buf, uint16_t maxlen)
{
    uint8_t bsb = W5500_BSB_SOCK0_REG | (sock << 2);
    uint8_t rxb = W5500_BSB_SOCK0_RX | (sock << 2);

    uint16_t recv_size = W5500_ReadReg16(W5500_Sn_RX_RSR, bsb);
    if (recv_size == 0)
        return 0;

    if (recv_size > maxlen)
        recv_size = maxlen;

    uint16_t rd = W5500_ReadReg16(W5500_Sn_RX_RD, bsb);
    W5500_ReadBuf(rd, rxb, buf, recv_size);

    /* 更新读指针并通知接收完成 */
    W5500_WriteReg16(W5500_Sn_RX_RD, bsb, rd + recv_size);
    W5500_WriteReg(W5500_Sn_CR, bsb, W5500_Sn_CR_RECV);

    return recv_size;
}

void W5500_TCP_Close(uint8_t sock)
{
    uint8_t bsb = W5500_BSB_SOCK0_REG | (sock << 2);
    W5500_WriteReg(W5500_Sn_CR, bsb, W5500_Sn_CR_DISCON);
    W5500_WriteReg(W5500_Sn_CR, bsb, 0x00);
}
