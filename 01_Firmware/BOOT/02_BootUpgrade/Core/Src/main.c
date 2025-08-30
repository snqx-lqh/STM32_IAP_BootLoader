/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usart.h"
#include "gpio.h"
#include "bootloader.h"
#include "xmodem.h"
#include "flash_if.h"
#include "stdio.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/*********************************************************************
项目使用的单片机是 STM32F103C6T6 他的Flash是32K 也就是 0x800_0000 - 0x800_8000
我使用 0x0x800_0000 - 0x800_2000 存放 Bootloader
  使用 0x0x800_2000 - 0x800_8000 存放 App 代码

**********************************************************************/

//  timeout_ms: 超时时间（毫秒），0 表示非阻塞  
static uint8_t UART_RecvByte(uint8_t *p, uint32_t timeout_ms)
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

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
	printf("**************************************\r\n");
	printf("          BootLoaderBasic             \r\n");
	printf("                V1                    \r\n");
	printf("**************************************\r\n");
	
	uint8_t ch;
	uint8_t boot_count = 5;
	while(1)
	{
		// 带1S延时的串口接收
		UART_RecvByte(&ch,1000);
		// 如果接收到了回车换行符 就进入 擦除代码区数据 接收 Bin 数据 的代码
		if(ch == 0x0D || ch == 0x0A)
		{
			// 擦除 APP 区代码
			Flash_Erase_App_Space();
			
			// 使用Xmodem接收数据
			if (XmodemReceiveData() == 0) 
			{
				printf("Download OK, jump to App...\r\n");	
				break;
			}
		}
		// 没有接收到字符就减减，减到0也就退出。
		else
		{
			boot_count --;
			printf("boot_count:%d\r\n",boot_count);
			if(boot_count == 0)
			{
				break;
			}
		}
	}
	
	HAL_Delay(100);
	// 跳转到 APP
	JumpToApplication(); 
	
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  //BL_XmodemReceive(&huart1);
	  //HAL_UART_Transmit(&huart1,"hello\r\n",sizeof("hello\r\n"),10);
	  //HAL_Delay(1000);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
