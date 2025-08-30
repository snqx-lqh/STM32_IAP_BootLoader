#include "main.h"

#include "bsp_delay.h"
#include "bsp_usart.h"
#include "bsp_sys.h"

#define ApplicationAddress  0x8002000//应用程序地址

/**
 * @brief 
 * @return 
 */
int main()
{
	//设置中断向量表的偏移大小
	NVIC_SetVectorTable(NVIC_VectTab_FLASH, ApplicationAddress - 0x08000000);
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	bsp_usart1_init(115200);
	delay_init();
	while(1)
	{
		printf("HELLO1\r\n");
		delay_ms(1000);
		printf("HELLO2\r\n");
		delay_ms(1000);
	}
}

