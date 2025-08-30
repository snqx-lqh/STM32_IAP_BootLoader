#include <stdint.h>
#include <string.h>
#include "usart.h"   // �ṩ USART_GetFlagStatus / USART_SendData / USART_ReceiveData
#include "flash_if.h"    // �ṩ FLASH_IF_Write

/* XMODEM ���� */
#define SOH  0x01
#define STX  0x02
#define EOT  0x04
#define ACK  0x06
#define NAK  0x15
#define CAN  0x18

#include "stm32f1xx_hal.h"  // �������оƬ��ͷ�ļ�

extern UART_HandleTypeDef huart1; // ʹ�� HAL ��Ĵ��ھ��

/* ---------- ����һ���ֽ� ---------- */
static void XM_SendByte(uint8_t data)
{
    // HAL_UART_Transmit Ĭ��������ģʽ����ȴ��������
    HAL_UART_Transmit(&huart1, &data, 1, 10);
}

/* ---------- ����һ���ֽ� ---------- */
/* timeout_ms: ��ʱʱ�䣨���룩��0 ��ʾ������ */
static uint8_t XM_RecvByte(uint8_t *p, uint32_t timeout_ms)
{
    HAL_StatusTypeDef status;

    if (timeout_ms == 0) {
        // ������ģʽ
        if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE)) {
            *p = (uint8_t)(huart1.Instance->DR & 0xFF);
            return 1;
        }
        return 0;
    } else {
        // ����ģʽ������ʱ
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



/* ------------- XMODEM ���������� ------------- */
/* ˵����
 * - ֧�� 128/1024 ���ְ���С
 * - ��ȷ�����ظ�����blk-1����ACK ������д Flash
 * - ����/ȡ��ʱ���� 2 �� CAN
 * - FLASH �����������˵�������ڿ������
 */
int XmodemReceive(uint32_t addr)
{
    /* ������ͷ3 + ����(1024) + CRC2 = 1029 �ֽڣ������� */
    uint8_t  pkt[1032];
    uint8_t  expect_blk = 1;
    uint32_t flash = addr;
 
    /* �������֣������Է��� 'C' ��ʾʹ�� CRC ģʽ */
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
        /* ÿ�뷢һ�� 'C' ֱ���Զ˿�ʼ */
    }

    while (1) {
        uint8_t head = pkt[0];

        if (head == SOH || head == STX) {
            uint16_t size = (head == SOH) ? 128 : 1024;
            /* ��ȡʣ�ࣺseq, ~seq, data(size), crc_hi, crc_lo */
            uint16_t need = 2 + size + 2;  /* ��ȥ�Ѷ��� head */
            for (uint16_t i = 1; i <= need; ++i) {
                if (!XM_RecvByte(&pkt[i], 1000)) {
                    /* ��ʱ��NAK ��ǰ���������ȴ� */
                    XM_SendByte(NAK);
                    /* ����ͬ�����ȴ���һ�ֽ���Ϊ�µ� head */
                    if (!XM_RecvByte(&pkt[0], 1000)) {
                        /* û����ͷ�������� 'C' ��ʾ */
                        XM_SendByte('C');
                        continue;
                    }
                    goto next_iteration;
                }
            }

            uint8_t  seq     = pkt[1];
            uint8_t  seq_inv = pkt[2];
            uint16_t crc_rx  = ((uint16_t)pkt[3 + size] << 8) | pkt[3 + size + 1];

            /* ���У�� */
            if ((uint8_t)(seq + seq_inv) != 0xFF) {
                XM_SendByte(NAK);
                /* ����һͷ */
                if (!XM_RecvByte(&pkt[0], 1000)) { XM_SendByte('C'); }
                goto next_iteration;
            }

            /* ���� CRC */
            uint16_t crc_calc = CRC16_XMODEM(&pkt[3], size);
            if (crc_calc != crc_rx) {
                XM_SendByte(NAK);
                if (!XM_RecvByte(&pkt[0], 1000)) { XM_SendByte('C'); }
                goto next_iteration;
            }

            if (seq == expect_blk) {
                /* �����¿飺д Flash */
				Flash_Write_App_Space(flash,&pkt[3],size);
                flash += size;
                expect_blk++;
                XM_SendByte(ACK);
            }
            else if (seq == (uint8_t)(expect_blk - 1)) {
                /* �ظ��飨��������һ�� ACK ��ʧ��������д��ֱ�� ACK */
                XM_SendByte(ACK);
            }
            else {
                /* ���ز�ͬ����Ҫ���ط���ǰ������ */
                XM_SendByte(NAK);
            }

            /* ��ȡ��һͷ�ֽڣ��������� */
            if (!XM_RecvByte(&pkt[0], 1000)) {
                /* �Ȳ�����ͷ���Ѻ���ʾ���� */
                XM_SendByte('C');
            }
        }
        else if (head == EOT) {
            /* ������������ʵ�֣�ֱ�� ACK ��ɣ������Ͷ��ٷ� EOT��Ҳ���ٴ� ACK�� */
            XM_SendByte(ACK);
            return 0;
        }
        else if (head == CAN) {
            /* ���Ͷ�ȡ�� */
            XM_SendByte(CAN);
            XM_SendByte(CAN);
            return -3;
        }
        else {
            /* ���������Բ���������Զ� */
            //XM_SendByte('C');
        }

    next_iteration:
        /* pkt[0] �ѱ����»�Ҫ���£����� while */
        ;
    }
}
