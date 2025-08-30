#ifndef __BOOTLOADER_H
#define __BOOTLOADER_H

#include "stm32f1xx_hal.h"
#include <stdbool.h>
#include <stdint.h>
#include "usart.h"

// ===== Flash ���� =====
#define APP_BASE_ADDR   0x08002000U   // App ��ʼ��ַ (Boot ռ�� 8K)
#define FLASH_END_ADDR  0x08008000U   // STM32F103C6 �� 32K
#define PAGE_SIZE       1024U         // 1KB/page

// ===== XMODEM ���� =====
#define X_SOH  0x01  // 128B
#define X_EOT  0x04
#define X_ACK  0x06
#define X_NAK  0x15
#define X_CAN  0x18
#define X_C    0x43  // 'C'

#define X_PACKET_SIZE 128
#define X_TIMEOUT_MS  1000
#define X_MAX_RETRY   5

// ===== ����ӿ� =====

// ��� App �Ƿ���Ч
bool BL_AppIsValid(void);

// ��ת�� App
void BL_JumpToApp(void);

// XMODEM ���ղ�д�� App �� (���� 0 ��ʾ�ɹ�)
int BL_XmodemReceive(UART_HandleTypeDef *huart);

// ��ʼ������ App ��
HAL_StatusTypeDef BL_FlashEraseApp(void);

// �� Flash д���� (���ֱ��)
HAL_StatusTypeDef BL_FlashWrite(uint32_t addr, const uint8_t *data, uint32_t len);

#endif // __BOOTLOADER_H
