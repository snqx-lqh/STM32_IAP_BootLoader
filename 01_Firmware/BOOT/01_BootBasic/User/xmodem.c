#include <stdint.h>
#include <string.h>
#include "usart.h"   // 提供 USART_GetFlagStatus / USART_SendData / USART_ReceiveData
#include "flash_if.h"    // 提供 FLASH_IF_Write

/* XMODEM 常量 */
#define SOH  0x01
#define STX  0x02
#define EOT  0x04
#define ACK  0x06
#define NAK  0x15
#define CAN  0x18

#include "stm32f1xx_hal.h"  // 根据你的芯片改头文件

extern UART_HandleTypeDef huart1; // 使用 HAL 库的串口句柄

/* ---------- 发送一个字节 ---------- */
static void XM_SendByte(uint8_t data)
{
    // HAL_UART_Transmit 默认是阻塞模式，会等待发送完成
    HAL_UART_Transmit(&huart1, &data, 1, 10);
}

/* ---------- 接收一个字节 ---------- */
/* timeout_ms: 超时时间（毫秒），0 表示非阻塞 */
static uint8_t XM_RecvByte(uint8_t *p, uint32_t timeout_ms)
{
    HAL_StatusTypeDef status;

    if (timeout_ms == 0) {
        // 非阻塞模式
        if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE)) {
            *p = (uint8_t)(huart1.Instance->DR & 0xFF);
            return 1;
        }
        return 0;
    } else {
        // 阻塞模式，带超时
        status = HAL_UART_Receive(&huart1, p, 1, timeout_ms);
        if (status == HAL_OK) return 1;
        else return 0;
    }
}

/* ------------- CRC16 (XMODEM) ------------- */
static uint16_t CRC16_XMODEM(const uint8_t *d, uint16_t len)
{
    uint16_t crc = 0;
    while (len--) {
        crc ^= (uint16_t)(*d++) << 8;
        for (uint8_t i = 0; i < 8; ++i) {
            if (crc & 0x8000) crc = (crc << 1) ^ 0x1021;
            else              crc = (crc << 1);
        }
    }
    return crc;
}


int XmodemReceiveData()
{
	uint8_t  pkt[138];
	uint32_t writeAddr = 0x8002000;
	uint16_t writeSize = 0;
	int     ret;
	uint8_t ch;
	char    C_Char = 'C';
	
	while(1)
	{
		for(int i=0;i< 64;i++)
		{
			if(C_Char)
			{
				XM_SendByte('C');
			}
			ret = XM_RecvByte(&ch, 1000);
			if (ret == 1) 
			{
				switch (ch)
				{
					case SOH: 
						writeSize = 128;
						pkt[0] = SOH;
						goto start_receive;
						break;
					case STX:
						writeSize = 1024;
						pkt[0] = STX;
						goto start_receive;
						break;
					case EOT:
						XM_SendByte(ACK);
						goto xmodem_exit;
						break;
					case CAN:
						break;
					default:
						break;
				}
			}
		}
start_receive:
		if(C_Char) C_Char=0;
		for(int i = 1;i < writeSize + 3 + 2;i++)
		{
			ret = XM_RecvByte(pkt+i, 1000);
		}
		Flash_Write_App_Space(writeAddr,&pkt[3],writeSize);
		writeAddr += writeSize;
		XM_SendByte(ACK);
		continue;
	}
xmodem_exit:
	return 0;
}



/* ------------- XMODEM 接收主函数 ------------- */
/* 说明：
 * - 支持 128/1024 两种包大小
 * - 正确处理重复包（blk-1）：ACK 但不重写 Flash
 * - 出错/取消时发送 2 个 CAN
 * - FLASH 擦除：按你的说明，已在开机完成
 */
int XmodemReceive(uint32_t addr)
{
    /* 最大包：头3 + 数据(1024) + CRC2 = 1029 字节，留余量 */
    uint8_t  pkt[1032];
    uint8_t  expect_blk = 1;
    uint32_t flash = addr;
 
    /* 启动握手：周期性发送 'C' 表示使用 CRC 模式 */
    while (1) {
		XM_SendByte('C');
        uint8_t ch;
		uint8_t ret = 0;
		ret = XM_RecvByte(&ch, 1000);
        if (ret == 1) 
		{
            if (ch == SOH || ch == STX || ch == EOT || ch == CAN) {
                pkt[0] = ch;
                break;
            }
        }
        /* 每秒发一次 'C' 直到对端开始 */
    }

    while (1) {
        uint8_t head = pkt[0];

        if (head == SOH || head == STX) {
            uint16_t size = (head == SOH) ? 128 : 1024;
            /* 读取剩余：seq, ~seq, data(size), crc_hi, crc_lo */
            uint16_t need = 2 + size + 2;  /* 除去已读的 head */
            for (uint16_t i = 1; i <= need; ++i) {
                if (!XM_RecvByte(&pkt[i], 1000)) {
                    /* 超时：NAK 当前包，继续等待 */
                    XM_SendByte(NAK);
                    /* 重新同步：等待下一字节作为新的 head */
                    if (!XM_RecvByte(&pkt[0], 1000)) {
                        /* 没有新头，继续发 'C' 提示 */
                        XM_SendByte('C');
                        continue;
                    }
                    goto next_iteration;
                }
            }

            uint8_t  seq     = pkt[1];
            uint8_t  seq_inv = pkt[2];
            uint16_t crc_rx  = ((uint16_t)pkt[3 + size] << 8) | pkt[3 + size + 1];

            /* 块号校验 */
            if ((uint8_t)(seq + seq_inv) != 0xFF) {
                XM_SendByte(NAK);
                /* 读下一头 */
                if (!XM_RecvByte(&pkt[0], 1000)) { XM_SendByte('C'); }
                goto next_iteration;
            }

            /* 计算 CRC */
            uint16_t crc_calc = CRC16_XMODEM(&pkt[3], size);
            if (crc_calc != crc_rx) {
                XM_SendByte(NAK);
                if (!XM_RecvByte(&pkt[0], 1000)) { XM_SendByte('C'); }
                goto next_iteration;
            }

            if (seq == expect_blk) {
                /* 正常新块：写 Flash */
				Flash_Write_App_Space(flash,&pkt[3],size);
                flash += size;
                expect_blk++;
                XM_SendByte(ACK);
            }
            else if (seq == (uint8_t)(expect_blk - 1)) {
                /* 重复块（可能是上一包 ACK 丢失）：不重写，直接 ACK */
                XM_SendByte(ACK);
            }
            else {
                /* 严重不同步：要求重发当前期望块 */
                XM_SendByte(NAK);
            }

            /* 读取下一头字节，进入下轮 */
            if (!XM_RecvByte(&pkt[0], 1000)) {
                /* 等不到新头，友好提示继续 */
                XM_SendByte('C');
            }
        }
        else if (head == EOT) {
            /* 结束：按常见实现，直接 ACK 完成（若发送端再发 EOT，也会再次 ACK） */
            XM_SendByte(ACK);
            return 0;
        }
        else if (head == CAN) {
            /* 发送端取消 */
            XM_SendByte(CAN);
            XM_SendByte(CAN);
            return -3;
        }
        else {
            /* 噪声：忽略并继续拉起对端 */
            //XM_SendByte('C');
        }

    next_iteration:
        /* pkt[0] 已被更新或将要更新，继续 while */
        ;
    }
}
