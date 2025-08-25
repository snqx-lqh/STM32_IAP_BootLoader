#include "main.h"

#include "bsp_delay.h"
#include "bsp_usart.h"
#include "bsp_sys.h"

// ����ָ���ض���
typedef  void (*pFunction)(void);
// APP����Ĵ洢��ַ
#define ApplicationAddress 0x8002000

pFunction Jump_To_Application;
uint32_t  JumpAddress;

int JumpToApplication() 
{
	// 1���ж� APP �׵�ַ�д洢�������ǲ��� 0x2000 ��ͷ�ģ���Ϊ�ڴ��ַ������
	// ����ж���Ϊ��ȷ��APP�ĳ�ʼջ��ָ�루MSP���Ƿ���SRAM�����ڡ�
    // ��STM32�У�SRAMͨ����0x20000000��ʼ����С��оƬ���죨��20KB��64KB��128KB�ȣ���
    // �������0x2FFE0000��Ϊ�����ε���λ��ַ�仯��ֻ����SRAM����ĸ�λ������
	if( ( ( *(__IO uint32_t*)ApplicationAddress ) & 0x2FFE0000 ) == 0x20000000 )
	{   
		printf("\r\n Run to app.\r\n");
		// 2����ȡAPP�ĸ�λ������Reset_Handler���ĵ�ַ��Ҳ����APP����ں�����ַ��
		JumpAddress = *(__IO uint32_t*) (ApplicationAddress + 4);
		// 3���Ѹõ�ַת����ʵ�ʵĺ���
		Jump_To_Application = (pFunction) JumpAddress;
		// 4����APP�ĳ�ʼջ����ַ���ص�MSP�Ĵ����С�
		__set_MSP(*(__IO uint32_t*) ApplicationAddress);
		// 5����ת��APP��Reset_Handler��Ҳ����APP���������룬֮��APP���ʼ���Լ���ϵͳ������main()��
		Jump_To_Application();
		return 0;
	}
	else
	{
		printf("\r\n Run to app error.\r\n");
		return -1;
	}
}

/**
 * @brief 
 * @return 
 */
int main()
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	bsp_usart1_init(115200);
	delay_init();
	
	printf("**************************************\r\n");
	printf("          BootLoaderBasic             \r\n");
	printf("                V1                    \r\n");
	printf("**************************************\r\n");
	
	JumpToApplication(); 
	
	while(1)
	{
	}
}

