#include "flash_if.h"

/**
  * @brief   擦除存放 APP 的地址空间
			 从 0x8002000 地址开始算 1页1K 到 0x8008000 有 24 页
  * @param   {Parameters}
  * @retval
 **/
uint32_t Flash_Erase_App_Space()
{
	uint32_t pageError = 0;
	HAL_StatusTypeDef  retErase;
	
	// 1.解锁Flash写保护
	HAL_FLASH_Unlock();
	
	// 2.设置擦除地址的变量
	FLASH_EraseInitTypeDef erase;
    erase.TypeErase   = FLASH_TYPEERASE_PAGES; // 擦除页
    erase.PageAddress = 0x8002000;             // 擦除的起始地址  只会按照整数算 最小 0x400 也就是 1K
    erase.NbPages     = 24;                    // 擦多少页 我们擦 0x8002000 - 0x8008000 一共24K 
	
	// 3.擦除 APP 的地址区域代码
	retErase = HAL_FLASHEx_Erase(&erase, &pageError);
	
	// 4.打开Flash写保护
	if (retErase != HAL_OK)
    {
		// 打开写保护
		HAL_FLASH_Lock();
		// 退出返回错误标识
		return pageError;
    }else
	{
		// 打开写保护
		HAL_FLASH_Lock();
		// 正常退出
		return 0;
	}
}

/**
  * @brief   往指定地址写数据，这个函数是有缺陷的，但是不影响这次学习
			 我们写入的数据大小 根据xmodem协议，就是128或者1024
  * @param   {Parameters}
  * @retval
 **/
int32_t Flash_Write_App_Space(uint32_t addr, const uint8_t *pData, uint32_t len)
{
	int32_t ret = 0;
	// 1. 解锁 Flash 
    HAL_FLASH_Unlock();
	// 2. 逐半字写入   
	for(uint32_t i = 0; i < len; i += 2)
    {
        uint16_t half;
        if(i + 1 < len)
            half = pData[i] | (pData[i+1]<<8); // 低字节在前，高字节在后
        else
            half = pData[i] | (0xFF << 8);     // 奇数字节最后补0xFF
		
        if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr + i, half) != HAL_OK)
        {
            HAL_FLASH_Lock();
            return -1;
        }
        while(__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY));
    }
    //3. 锁定 Flash  
    HAL_FLASH_Lock();
    return ret;
}