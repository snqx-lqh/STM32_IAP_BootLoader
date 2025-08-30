#include <stdint.h>
#include <string.h>
#include "usart.h"   
#include "flash_if.h"   

/* XMODEM 常量 */
#define SOH  0x01
#define STX  0x02
#define EOT  0x04
#define ACK  0x06
#define NAK  0x15
#define CAN  0x18

extern UART_HandleTypeDef huart1; // 使用 HAL 库的串口句柄


static void XM_SendByte(uint8_t data)
{
    HAL_UART_Transmit(&huart1, &data, 1, 10);
}

//  timeout_ms: 超时时间（毫秒），0 表示非阻塞  
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

int XmodemReceiveData()
{
	uint8_t  pkt[138];
	uint32_t writeAddr = 0x8002000;
	uint16_t writeSize = 0;
	int      ret;
	uint8_t  ch;
	char     C_Char = 'C';
	
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

