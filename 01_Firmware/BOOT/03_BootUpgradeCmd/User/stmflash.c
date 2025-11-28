#include "stmflash.h" 

/**
  * @brief  Read half words (16-bit data) of the specified address
  * @note   This function can be used for all STM32F10x devices.
  * @param  faddr: The address to be read (the multiple of the address, which is 2)
  * @retval Value of specified address
  */
uint16_t STMFLASH_ReadHalfWord(uint32_t faddr)
{
	return *(volatile uint16_t*)faddr; 
}


/**
  * @brief  There is no check writing.
  * @note   This function can be used for all STM32F10x devices.
  * @param  WriteAddr: The starting address to be written.
  * @param  pBuffer: The pointer to the data.
  * @param  NumToWrite:  The number of half words written
  * @retval None
  */
static void STMFLASH_Write_NoCheck(uint32_t WriteAddr,uint16_t *pBuffer,uint16_t NumToWrite)   
{ 			 		 
	uint16_t i;
	for(i=0;i<NumToWrite;i++)
	{
		// FLASH_ProgramHalfWord(WriteAddr,pBuffer[i]);  // 标准库
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, WriteAddr, pBuffer[i]); // HAL 库
	    WriteAddr+=2;//add addr 2.
	}  
} 

uint16_t STMFLASH_BUF[PAGE_SIZE / 2];//Up to 2K bytes

void HAL_FLASH_ErasePage(uint32_t addr)
{
	uint32_t pageError = 0;
	HAL_StatusTypeDef  retErase;

	FLASH_EraseInitTypeDef erase;
    erase.TypeErase   = FLASH_TYPEERASE_PAGES; // 擦除页
    erase.PageAddress = addr;             // 擦除的起始地址  只会按照整数算 最小 0x400 也就是 1K
    erase.NbPages     = 1;                    // 擦多少页 我们擦 0x8002000 - 0x8008000 一共24K 

	retErase = HAL_FLASHEx_Erase(&erase, &pageError);
}

/**
  * @brief  Write data from the specified address to the specified length.
  * @note   This function can be used for all STM32F10x devices.
  * @param  WriteAddr: The starting address to be written.(The address must be a multiple of two)
  * @param  pBuffer: The pointer to the data.
  * @param  NumToWrite:  The number of half words written
  * @retval None
  */
void STMFLASH_Write(uint32_t WriteAddr,uint16_t *pBuffer,uint16_t NumToWrite)	
{
	uint32_t secpos;	   //扇区地址
	uint16_t secoff;	   //扇区内偏移地址(16位字计算)
	uint16_t secremain; //扇区内剩余地址(16位字计算)	   
 	uint16_t i;    
	uint32_t offaddr;   //去掉0X08000000后的地址
	if((WriteAddr < FLASH_BASE) || (WriteAddr >= FLASH_BASE + 1024 * FLASH_SIZE))return;//非法地址
	// FLASH_Unlock();						//解锁 标准库
	HAL_FLASH_Unlock();                     //解锁 HAL库 
	offaddr = WriteAddr - FLASH_BASE;		//实际偏移地址.
	secpos = offaddr / PAGE_SIZE;			//扇区地址  0~127 for STM32F103RBT6
	secoff = (offaddr % PAGE_SIZE) / 2;		//在扇区内的偏移(2个字节为基本单位.)
	secremain = PAGE_SIZE / 2 - secoff;		//扇区剩余空间大小   
	if(NumToWrite <= secremain)
		secremain = NumToWrite;//不大于该扇区范围
	while(1) 
	{	
		STMFLASH_Read(secpos * PAGE_SIZE + FLASH_BASE, STMFLASH_BUF, PAGE_SIZE / 2);//读出整个扇区的内容
		for(i = 0; i < secremain; i++)//校验数据
		{
			if(STMFLASH_BUF[secoff + i] != 0XFFFF)break;//需要擦除  	  
		}
		if(i < secremain)//需要擦除
		{
			//FLASH_ErasePage(secpos * PAGE_SIZE + FLASH_BASE);//擦除这个扇区
			HAL_FLASH_ErasePage(secpos * PAGE_SIZE + FLASH_BASE); //擦除这个扇区 HAL 库
			for(i=0; i < secremain; i++)
			{
				STMFLASH_BUF[i + secoff] = pBuffer[i];	  
			}
			STMFLASH_Write_NoCheck(secpos * PAGE_SIZE + FLASH_BASE, STMFLASH_BUF, PAGE_SIZE / 2);//写入整个扇区  
		}else 
			STMFLASH_Write_NoCheck(WriteAddr, pBuffer, secremain);//写已经擦除了的,直接写入扇区剩余区间. 				   
		if(NumToWrite == secremain)break;//写入结束了
		else//写入未结束
		{
			secpos++;				//扇区地址增1
			secoff = 0;				//偏移位置为0 	 
		   	pBuffer += secremain;  	//指针偏移
			WriteAddr += secremain;	//写地址偏移	   
		   	NumToWrite -= secremain;	//字节(16位)数递减
			if(NumToWrite > (PAGE_SIZE / 2)) secremain = PAGE_SIZE / 2;//下一个扇区还是写不完
			else secremain = NumToWrite;//下一个扇区可以写完了
		}	 
	};	
	//FLASH_Lock();//上锁
	HAL_FLASH_Lock();                     //上锁 HAL库 
}

/**
  * @brief  Start reading the specified data from the specified address.
  * @note   This function can be used for all STM32F10x devices.
  * @param  ReadAddr: Start addr
  * @param  pBuffer: The pointer to the data.
  * @param  NumToWrite:  The number of half words written(16bit)
  * @retval None
  */
void STMFLASH_Read(uint32_t ReadAddr,uint16_t *pBuffer,uint16_t NumToRead)   	
{
	uint16_t i;
	for(i=0;i<NumToRead;i++)
	{
		pBuffer[i]=STMFLASH_ReadHalfWord(ReadAddr);//读取2个字节.
		ReadAddr+=2;//偏移2个字节.	
	}
}

/**
  * @brief  Calculate the number of pages
  * @param  Size: The image size
  * @retval The number of pages
  */
uint32_t FLASH_PagesMask(__IO uint32_t Size)
{
	uint32_t pagenumber = 0x0;
	uint32_t size = Size;

	if ((size % PAGE_SIZE) != 0)
	{
		pagenumber = (size / PAGE_SIZE) + 1;
	}
	else
	{
		pagenumber = size / PAGE_SIZE;
	}
	return pagenumber;
}

/**
  * @brief   擦除从指定的 APP 地址开始的 size 大小空间 代码中会计算该长度需要擦多少块
  * @param   size 大小 outPutCont 我没使用，也没注意他原来是干嘛的
  * @retval
 **/
uint8_t EraseSomePages(__IO uint32_t size, uint8_t outPutCont)
{ 
	uint32_t pageError = 0;
	HAL_StatusTypeDef  retErase;
	uint32_t NbrOfPage = 0;  
	
	NbrOfPage = FLASH_PagesMask(size);

	HAL_FLASH_Unlock();                     //解锁  
	
	// 2.设置擦除地址的变量
	FLASH_EraseInitTypeDef erase;
    erase.TypeErase   = FLASH_TYPEERASE_PAGES; // 擦除页
    erase.PageAddress = ApplicationAddress;    // 擦除的起始地址   
    erase.NbPages     = NbrOfPage;             // 擦多少页  
	
	// 3.擦除 APP 的地址区域代码
	retErase = HAL_FLASHEx_Erase(&erase, &pageError);
	
	//FLASH_Lock();
	HAL_FLASH_Lock();                     //上锁 HAL库 
	 
	return 1;
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











