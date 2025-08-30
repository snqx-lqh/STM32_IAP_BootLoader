#include "bootloader.h"

// ========== CRC16-CCITT ==========
static uint16_t crc16_ccitt(const uint8_t *data, uint32_t len) {
    uint16_t crc = 0;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; j++) {
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
        }
    }
    return crc;
}

// ========== 应用有效性 ==========
bool BL_AppIsValid(void) {
    uint32_t sp = *(uint32_t*)APP_BASE_ADDR;
    uint32_t rv = *(uint32_t*)(APP_BASE_ADDR + 4);

    if ((sp & 0x2FFE0000U) != 0x20000000U) return false; // 栈顶必须在 SRAM
    if (rv < APP_BASE_ADDR || rv >= FLASH_END_ADDR) return false;
    return true;
}

// ========== 跳转到应用 ==========
typedef void (*app_entry_t)(void);

void BL_JumpToApp(void) {
    __disable_irq();

    uint32_t app_sp = *(uint32_t*)APP_BASE_ADDR;
    uint32_t app_pc = *(uint32_t*)(APP_BASE_ADDR + 4);

    // 停止外设
    HAL_UART_DeInit(&huart1);
    SysTick->CTRL = 0; SysTick->LOAD = 0; SysTick->VAL = 0;

    SCB->VTOR = APP_BASE_ADDR; // 中断向量表重定向
    __set_MSP(app_sp);
    __set_PSP(app_sp);

    ((app_entry_t)app_pc)();
}

// ========== Flash 操作 ==========
HAL_StatusTypeDef BL_FlashEraseApp(void) {
    FLASH_EraseInitTypeDef ei = {0};
    uint32_t page_error = 0;

    uint32_t start = APP_BASE_ADDR;
    uint32_t n_pages = (FLASH_END_ADDR - APP_BASE_ADDR) / PAGE_SIZE;

    HAL_FLASH_Unlock();

    ei.TypeErase   = FLASH_TYPEERASE_PAGES;
    ei.PageAddress = start;
    ei.NbPages     = n_pages;

    return HAL_FLASHEx_Erase(&ei, &page_error);
}

HAL_StatusTypeDef BL_FlashWrite(uint32_t addr, const uint8_t *data, uint32_t len) {
    HAL_StatusTypeDef st = HAL_OK;
    for (uint32_t i = 0; i < len; i += 2) {
        uint16_t hw = data[i] | ((i+1 < len ? data[i+1] : 0xFF) << 8);
        st = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr + i, hw);
        if (st != HAL_OK) break;
        if (*(volatile uint16_t*)(addr + i) != hw) { st = HAL_ERROR; break; }
    }
    return st;
}

// ========== UART 辅助 ==========
static int uart_recv_byte(UART_HandleTypeDef *huart, uint8_t *b, uint32_t timeout_ms) {
    return (HAL_UART_Receive(huart, b, 1, timeout_ms) == HAL_OK) ? 0 : -1;
}
static void uart_send_byte(UART_HandleTypeDef *huart, uint8_t b) {
    HAL_UART_Transmit(huart, &b, 1, 50);
}

// ========== XMODEM 接收并写入 ==========
int BL_XmodemReceive(UART_HandleTypeDef *huart) {
    uint8_t blk = 1;
    uint32_t write_addr = APP_BASE_ADDR;

    if (BL_FlashEraseApp() != HAL_OK) { HAL_FLASH_Lock(); return -10; }

    // 握手
    for (int retry = 0; retry < X_MAX_RETRY; retry++) {
        uart_send_byte(huart, X_C);
        uint8_t c;
        if (uart_recv_byte(huart, &c, X_TIMEOUT_MS) == 0) {
            if (c == X_SOH) goto RX_PACKET;
            if (c == X_EOT) { uart_send_byte(huart, X_ACK); HAL_FLASH_Lock(); return 0; }
        }
    }
    HAL_FLASH_Lock();
    return -11;

RX_LOOP:
    {
        uint8_t c;
        if (uart_recv_byte(huart, &c, X_TIMEOUT_MS) != 0) { uart_send_byte(huart, X_NAK); goto RX_LOOP; }
        if (c == X_SOH) goto RX_PACKET;
        if (c == X_EOT) { uart_send_byte(huart, X_ACK); HAL_FLASH_Lock(); return 0; }
        if (c == X_CAN) { HAL_FLASH_Lock(); return -20; }
        uart_send_byte(huart, X_NAK);
        goto RX_LOOP;
    }

RX_PACKET: ;
    uint8_t hdr[2];
    if (HAL_UART_Receive(huart, hdr, 2, X_TIMEOUT_MS) != HAL_OK) { uart_send_byte(huart, X_NAK); goto RX_LOOP; }
    uint8_t blk_no = hdr[0];
    uint8_t blk_inv = hdr[1];
    if ((uint8_t)(blk_no + blk_inv) != 0xFF) { uart_send_byte(huart, X_NAK); goto RX_LOOP; }

    uint8_t data[X_PACKET_SIZE];
    if (HAL_UART_Receive(huart, data, X_PACKET_SIZE, X_TIMEOUT_MS) != HAL_OK) { uart_send_byte(huart, X_NAK); goto RX_LOOP; }
    uint8_t crcb[2];
    if (HAL_UART_Receive(huart, crcb, 2, X_TIMEOUT_MS) != HAL_OK) { uart_send_byte(huart, X_NAK); goto RX_LOOP; }
    uint16_t rxcrc = ((uint16_t)crcb[0] << 8) | crcb[1];
    uint16_t calc = crc16_ccitt(data, X_PACKET_SIZE);
    if (rxcrc != calc) { uart_send_byte(huart, X_NAK); goto RX_LOOP; }

    if (blk_no == blk) {
        if (write_addr + X_PACKET_SIZE > FLASH_END_ADDR) { uart_send_byte(huart, X_CAN); HAL_FLASH_Lock(); return -30; }
        if (BL_FlashWrite(write_addr, data, X_PACKET_SIZE) != HAL_OK) { uart_send_byte(huart, X_CAN); HAL_FLASH_Lock(); return -31; }
        write_addr += X_PACKET_SIZE;
        blk++;
    }
    // 重发的旧块直接 ACK
    uart_send_byte(huart, X_ACK);
    goto RX_LOOP;
}
