#ifndef __STMFLASH_H__
#define __STMFLASH_H__

#include "main.h"

/* Define the APP start address -------------------------------*/
#define ApplicationAddress    0x8003000

#define FLASH_32

/* Define the Flsah area size ---------------------------------*/  
#if defined (FLASH_128)  
 #define PAGE_SIZE                         (0x400)    /* 1 Kbyte */
 #define FLASH_SIZE                        (0x20000)  /* 128 KBytes */
#elif defined FLASH_256
 #define PAGE_SIZE                         (0x800)    /* 2 Kbytes */
 #define FLASH_SIZE                        (0x40000)  /* 256 KBytes */
#elif defined FLASH_512
 #define PAGE_SIZE                         (0x800)    /* 2 Kbytes */
 #define FLASH_SIZE                        (0x80000)  /* 512 KBytes */
#elif defined FLASH_1024
 #define PAGE_SIZE                         (0x800)    /* 2 Kbytes */
 #define FLASH_SIZE                        (0x100000) /* 1 MByte */
#elif defined FLASH_32
 #define PAGE_SIZE                         (0x400)    /* 2 Kbytes */
 #define FLASH_SIZE                        (0x8000) /* 1 MByte */
#else 
 #error "Please select first the STM32 device to be used (in stm32f10x.h)"    
#endif

/* Compute the FLASH upload image size --------------------------*/  
#define FLASH_IMAGE_SIZE                   (uint32_t) (FLASH_SIZE - (ApplicationAddress - 0x08000000))

extern uint16_t STMFLASH_ReadHalfWord(uint32_t faddr);		  //读出半字  
extern void STMFLASH_Write(uint32_t WriteAddr,uint16_t *pBuffer,uint16_t NumToWrite);		//从指定地址开始写入指定长度的数据
extern void STMFLASH_Read(uint32_t ReadAddr,uint16_t *pBuffer,uint16_t NumToRead);   		//从指定地址开始读出指定长度的数据								   
uint8_t EraseSomePages(__IO uint32_t size, uint8_t outPutCont);
int32_t Flash_Write_App_Space(uint32_t addr, const uint8_t *pData, uint32_t len); 

#endif

















