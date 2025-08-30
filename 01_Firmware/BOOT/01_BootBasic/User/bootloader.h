#ifndef __BOOTLOADER_H
#define __BOOTLOADER_H

#include "stm32f1xx_hal.h"
#include <stdbool.h>
#include <stdint.h>
#include "usart.h"

// ===== Flash 布局 =====
#define APP_BASE_ADDR   0x08002000U   // App 起始地址 (Boot 占用 8K)
#define FLASH_END_ADDR  0x08008000U   // STM32F103C6 总 32K
#define PAGE_SIZE       1024U         // 1KB/page

// ===== XMODEM 常量 =====
#define X_SOH  0x01  // 128B
#define X_EOT  0x04
#define X_ACK  0x06
#define X_NAK  0x15
#define X_CAN  0x18
#define X_C    0x43  // 'C'

#define X_PACKET_SIZE 128
#define X_TIMEOUT_MS  1000
#define X_MAX_RETRY   5

// ===== 对外接口 =====

// 检查 App 是否有效
bool BL_AppIsValid(void);

// 跳转到 App
void BL_JumpToApp(void);

// XMODEM 接收并写入 App 区 (返回 0 表示成功)
int BL_XmodemReceive(UART_HandleTypeDef *huart);

// 初始化擦除 App 区
HAL_StatusTypeDef BL_FlashEraseApp(void);

// 向 Flash 写数据 (半字编程)
HAL_StatusTypeDef BL_FlashWrite(uint32_t addr, const uint8_t *data, uint32_t len);

#endif // __BOOTLOADER_H
