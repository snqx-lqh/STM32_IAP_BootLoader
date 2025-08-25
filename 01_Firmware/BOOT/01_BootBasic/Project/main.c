#include "main.h"

#include "bsp_delay.h"
#include "bsp_usart.h"
#include "bsp_sys.h"

// 函数指针重定义
typedef  void (*pFunction)(void);
// APP代码的存储地址
#define ApplicationAddress 0x8002000

pFunction Jump_To_Application;
uint32_t  JumpAddress;

int JumpToApplication() 
{
	// 1、判断 APP 首地址中存储的数据是不是 0x2000 开头的，因为内存地址在这里
	// 这个判断是为了确认APP的初始栈顶指针（MSP）是否在SRAM区域内。
    // 在STM32中，SRAM通常从0x20000000开始，大小因芯片而异（如20KB、64KB、128KB等）。
    // 这个掩码0x2FFE0000是为了屏蔽掉低位地址变化，只保留SRAM区域的高位特征。
	if( ( ( *(__IO uint32_t*)ApplicationAddress ) & 0x2FFE0000 ) == 0x20000000 )
	{   
		printf("\r\n Run to app.\r\n");
		// 2、获取APP的复位向量（Reset_Handler）的地址，也就是APP的入口函数地址，
		JumpAddress = *(__IO uint32_t*) (ApplicationAddress + 4);
		// 3、把该地址转化成实际的函数
		Jump_To_Application = (pFunction) JumpAddress;
		// 4、把APP的初始栈顶地址加载到MSP寄存器中。
		__set_MSP(*(__IO uint32_t*) ApplicationAddress);
		// 5、跳转到APP的Reset_Handler，也就是APP的启动代码，之后APP会初始化自己的系统、调用main()。
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

