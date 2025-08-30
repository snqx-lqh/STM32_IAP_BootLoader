#include "flash_if.h"

/**
  * @brief   ������� APP �ĵ�ַ�ռ�
			 �� 0x8002000 ��ַ��ʼ�� 1ҳ1K �� 0x8008000 �� 24 ҳ
  * @param   {Parameters}
  * @retval
 **/
uint32_t Flash_Erase_App_Space()
{
	uint32_t pageError = 0;
	HAL_StatusTypeDef  retErase;
	
	// 1.����Flashд����
	HAL_FLASH_Unlock();
	
	// 2.���ò�����ַ�ı���
	FLASH_EraseInitTypeDef erase;
    erase.TypeErase   = FLASH_TYPEERASE_PAGES; // ����ҳ
    erase.PageAddress = 0x8002000;             // ��������ʼ��ַ  ֻ�ᰴ�������� ��С 0x400 Ҳ���� 1K
    erase.NbPages     = 24;                    // ������ҳ ���ǲ� 0x8002000 - 0x8008000 һ��24K 
	
	// 3.���� APP �ĵ�ַ�������
	retErase = HAL_FLASHEx_Erase(&erase, &pageError);
	
	// 4.��Flashд����
	if (retErase != HAL_OK)
    {
		// ��д����
		HAL_FLASH_Lock();
		// �˳����ش����ʶ
		return pageError;
    }else
	{
		// ��д����
		HAL_FLASH_Lock();
		// �����˳�
		return 0;
	}
}

/**
  * @brief   ��ָ����ַд���ݣ������������ȱ�ݵģ����ǲ�Ӱ�����ѧϰ
			 ����д������ݴ�С ����xmodemЭ�飬����128����1024
  * @param   {Parameters}
  * @retval
 **/
int32_t Flash_Write_App_Space(uint32_t addr, const uint8_t *pData, uint32_t len)
{
	int32_t ret = 0;
	// 1. ���� Flash 
    HAL_FLASH_Unlock();
	// 2. �����д��   
	for(uint32_t i = 0; i < len; i += 2)
    {
        uint16_t half;
        if(i + 1 < len)
            half = pData[i] | (pData[i+1]<<8); // ���ֽ���ǰ�����ֽ��ں�
        else
            half = pData[i] | (0xFF << 8);     // �����ֽ����0xFF
		
        if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr + i, half) != HAL_OK)
        {
            HAL_FLASH_Lock();
            return -1;
        }
        while(__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY));
    }
    //3. ���� Flash  
    HAL_FLASH_Lock();
    return ret;
}