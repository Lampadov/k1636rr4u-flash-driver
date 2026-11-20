/*
 *  FLASH_test.h
 *
 *  Created on: 20.11.2025
 *  Author: Support Milandr
 */

#ifndef NAND_FLASH_TEST_H_
#define NAND_FLASH_TEST_H_

// Num[3:1] выбирают один из восьми блоков памяти по 2 Мбит
#define Num3				(0 << 10)
#define Num2				(0 << 9)
#define Num1				(0 << 8)
// Num[0] выбирает подблок в выбранном блоке (один подблок объемом 1 Мбит)
#define Num0				(0 << 7)
#define CEB					(0 << 6)
#define NVRB				(1 << 5)
#define OEB					(1 << 4)
#define WEB					(1 << 3)
#define TMEN				(0 << 2)
#define BYTEB_16		(1 << 1)
#define BYTEB_8			(0 << 1)
#define VREAD 			(0 << 0)

// Выбор одного из восьми блоков (2 Мбит)
#define FLASH_BLOCK_1			(0x0 << 8)
#define FLASH_BLOCK_2			(0x1 << 8)
#define FLASH_BLOCK_3 		(0x2 << 8)
#define FLASH_BLOCK_4			(0x3 << 8)
#define FLASH_BLOCK_5			(0x4 << 8)
#define FLASH_BLOCK_6			(0x5 << 8)
#define FLASH_BLOCK_7 		(0x6 << 8)
#define FLASH_BLOCK_8			(0x7 << 8)

// Выбор одного из двух подблоков в выбранном блоке (1 Мбит)
#define FLASH_SUBBLOCK_1  (0x0 << 7)
#define FLASH_SUBBLOCK_2  (0x1 << 7)


//Адреса для командных последовательностей
#define ADDR_555	0x0AAA
#define ADDR_AAA    0x0555
#define ADDR_555_16 0xAAA0

// Данные для командных последовательностей
#define DATA_AA 	0x0055
#define DATA_55		0x00AA
#define DATA_A0		0x0005
#define DATA_10  	0x0800
#define DATA_80 	0x0001
#define DATA_50		0x000A


#define ERR_OK 						 0
#define ERR_DATA_NOT_WRITTEN		-1
#define ERR_SECTOR_NOT_ERRASE		-2

// Прототипы функций
void vTest_Ext_FLASH( void );
uint32_t Write_Halfword_To_Flash(uint16_t addr, uint16_t data, uint32_t Num_block, uint32_t Num_subblock);
uint32_t Erase_Sector_Flash(uint32_t Num_Sector);
uint32_t Erase_Sector_Flash_NVRB(uint32_t Num_Sector);
uint16_t Read_Halfword_From_Flash(uint16_t addr, uint32_t Num_block, uint32_t Num_subblock);

#endif /* NAND_FLASH_TEST_H_ */
