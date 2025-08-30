#ifndef __FLASH_IF_H
#define __FLASH_IF_H

#include "main.h"

uint32_t Flash_Erase_App_Space();		   
int32_t Flash_Write_App_Space(uint32_t addr, const uint8_t *pData, uint32_t len); 

#endif