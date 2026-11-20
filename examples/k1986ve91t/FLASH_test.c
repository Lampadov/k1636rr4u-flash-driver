/*
 *  FLASH_test.c
 *
 *  Created on: 20.11.2025
 *  Author: Support Milandr
 */

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "stdint.h"
#include <MDR32FxQI_port.h>
// Пользовательские файлы
#include "FLASH_test.h"
#include "MDR32FxQI_ssp.h"
#include "MDR32F9Q2I.h"
#include "MDR32FxQI_ebc.h"
#include "MDR32FxQI_timer.h"

#define SERIAL_MODE

static PORT_InitTypeDef My_PortInitStructure;
static SSP_InitTypeDef  My_SSP_InitStruct;

// Функция перестановки бит от старшего к младшему
uint16_t MSBtoLSB(uint16_t data, uint32_t size)
{
	uint16_t tmp = 0;
	uint32_t i = 0;

	for(i = 0; i < size; i++){
		tmp	= tmp | (((data&(1 << i)) >> i) << (size-i-1));
	}
	return tmp;
}

// Функция перестановки бит от младшего к старшему
uint16_t LSBtoMSB(uint16_t data, uint32_t size)
{
	uint16_t tmp = 0;
	uint32_t i = 0;

	for(i = 0; i < size; i++){
		tmp |= ((((data & (1 << (size - 1 - i)) )) >> (size - 1 - i))) << (i);
	}
	return tmp;
}

#ifdef SERIAL_MODE

// Тестирование Flash-памяти в последовательном режиме
// Функция стирает первый блок памяти (всего 8) 2 Мбит, записывает данные во второй подблок (1 Мбит) и верифицирует его
void vTest_Ext_FLASH( void )
{
	// Настройки справедливы для частоты микроконтроллера 80 МГц
	// Если драйвер интегрируется в Ваш проект, то необходимо не забыть включить тактирование используемых портов и блока SSP

	// Настройка выводов под SPI
  My_PortInitStructure.PORT_Pin = (PORT_Pin_12 | PORT_Pin_13 | PORT_Pin_14 | PORT_Pin_15);
  My_PortInitStructure.PORT_OE = PORT_OE_OUT;
  My_PortInitStructure.PORT_FUNC = PORT_FUNC_ALTER;
  My_PortInitStructure.PORT_SPEED = PORT_SPEED_MAXFAST;
  My_PortInitStructure.PORT_MODE = PORT_MODE_DIGITAL;
  PORT_Init(MDR_PORTB, &My_PortInitStructure);

	// Инициализация SPI
	SSP_DeInit(MDR_SSP1);
	SSP_BRGInit(MDR_SSP1, SSP_HCLKdiv2);	// Инициализация тактовой частоты
	SSP_StructInit(&My_SSP_InitStruct);

	My_SSP_InitStruct.SSP_WordLength = SSP_WordLength16b;
	My_SSP_InitStruct.SSP_FRF = SSP_FRF_SPI_Motorola;
	My_SSP_InitStruct.SSP_SPO = SSP_SPO_High;
	My_SSP_InitStruct.SSP_SPH = SSP_SPH_2Edge;
	My_SSP_InitStruct.SSP_CPSDVSR = 8;
	My_SSP_InitStruct.SSP_SCR = 0;
	//  F_SSPCLK / ( CPSDVR * (1 + SCR) ) = 40 / (8 * (1 + 0)) = 5 МГц для режима программирования */
	//  F_SSPCLK / ( CPSDVR * (1 + SCR) ) = 40 / (2 * (1 + 9)) = 2 МГц для режима стирания микросхемы */
	SSP_Init(MDR_SSP1, &My_SSP_InitStruct);
	SSP_Cmd(MDR_SSP1, ENABLE);

	// Настройка FSS для ручного стробирования
  My_PortInitStructure.PORT_Pin = PORT_Pin_12 | PORT_Pin_13;
  My_PortInitStructure.PORT_OE = PORT_OE_OUT;
  My_PortInitStructure.PORT_FUNC = PORT_FUNC_PORT;
  My_PortInitStructure.PORT_SPEED = PORT_SPEED_MAXFAST;
  My_PortInitStructure.PORT_MODE = PORT_MODE_DIGITAL;
  My_PortInitStructure.PORT_PULL_DOWN = PORT_PULL_DOWN_ON;
	PORT_Init(MDR_PORTB, &My_PortInitStructure);

	//  PB14 - SPI_Rx на данном этапе инициализируется как порт
  My_PortInitStructure.PORT_Pin = PORT_Pin_14;
  My_PortInitStructure.PORT_OE = PORT_OE_IN;
  My_PortInitStructure.PORT_FUNC = PORT_FUNC_PORT;
  My_PortInitStructure.PORT_SPEED = PORT_SPEED_MAXFAST;
  My_PortInitStructure.PORT_MODE = PORT_MODE_DIGITAL;
	PORT_Init(MDR_PORTB, &My_PortInitStructure);

	// CS2 для Flash-памяти
	My_PortInitStructure.PORT_Pin = PORT_Pin_14;
	My_PortInitStructure.PORT_OE = PORT_OE_OUT;
	My_PortInitStructure.PORT_FUNC = PORT_FUNC_PORT;
	My_PortInitStructure.PORT_SPEED = PORT_SPEED_MAXFAST;
	My_PortInitStructure.PORT_MODE = PORT_MODE_DIGITAL;
	PORT_Init(MDR_PORTE, &My_PortInitStructure);

	// OE и WE для Flash-памяти
	My_PortInitStructure.PORT_Pin = PORT_Pin_1 | PORT_Pin_2;
	My_PortInitStructure.PORT_OE = PORT_OE_OUT;
	My_PortInitStructure.PORT_FUNC = PORT_FUNC_PORT;
	My_PortInitStructure.PORT_SPEED = PORT_SPEED_MAXFAST;
	My_PortInitStructure.PORT_MODE = PORT_MODE_DIGITAL;
	PORT_Init(MDR_PORTC, &My_PortInitStructure);

	uint16_t addr = 0;
	uint16_t data = 0x1234;
	uint32_t wait = 0;
	volatile uint32_t i = 32;

	uint32_t num_test = 0;
	uint16_t pattern = 0;
	for(num_test = 0; num_test < 1; num_test++){

	pattern = 0x5555; // Данные для записи

	// Стираем первый блок (2 Мбит)
	volatile uint32_t error = 0;

	error = Erase_Sector_Flash(FLASH_BLOCK_1);

	// Перенастраиваем частоту работы SPI
	// Выключаем SSP1
	SSP_Cmd(MDR_SSP1, DISABLE);
	// Переинициализация регистров SPI1
	My_SSP_InitStruct.SSP_CPSDVSR = 8; //  F_SSPCLK / ( CPSDVR * (1 + SCR) ) = 40 / (8 * (1 + 0)) = 5 МГц для режима программирования */
	My_SSP_InitStruct.SSP_SCR = 0;
	SSP_Init(MDR_SSP1, &My_SSP_InitStruct);
	// Включаем SPI1
	SSP_Cmd(MDR_SSP1, ENABLE);

	int i = 0;
	// Заполняем данными pattern и ~pattern второй подблок первого блока (суммарно 1 Мбит)
	for (i = 0; i < 65536; i++){
		if((i&1) == 0)
			error = Write_Halfword_To_Flash((uint16_t)i, (uint16_t)pattern, FLASH_BLOCK_1, FLASH_SUBBLOCK_2);
		else
			error = Write_Halfword_To_Flash((uint16_t)i, (uint16_t)(~pattern), FLASH_BLOCK_1, FLASH_SUBBLOCK_2);
	}

	uint16_t tmp = 0;
	uint16_t Receive_Data = 0;
	// Считываем 32 16-ти разрядных слов
	uint32_t Num_errors = 0;
	for (i = 0; i < 65536; i++){
		Receive_Data = Read_Halfword_From_Flash((uint16_t)i, FLASH_BLOCK_1, FLASH_SUBBLOCK_2);
		if((i&1) == 0)
			tmp = pattern;
		else
			tmp = (~pattern);
		if(Receive_Data != tmp){
			Num_errors++;
		}
	}
}
}
#endif // #ifdef SERIAL_MODE


#ifdef SERIAL_MODE
// Функция стирания конкретного подблока микросхемы Flash-памяти
uint32_t Erase_Sector_Flash(uint32_t Num_Sector)
{

  My_PortInitStructure.PORT_Pin = (PORT_Pin_13 | PORT_Pin_14 | PORT_Pin_15);
  My_PortInitStructure.PORT_OE = PORT_OE_OUT;
  My_PortInitStructure.PORT_FUNC = PORT_FUNC_ALTER;
  My_PortInitStructure.PORT_SPEED = PORT_SPEED_MAXFAST;
  My_PortInitStructure.PORT_MODE = PORT_MODE_DIGITAL;
	PORT_Init(MDR_PORTB, &My_PortInitStructure);

	// Настройка тактовой частоты 2 МГц для стирания микросхемы
	SSP_Cmd(MDR_SSP1, DISABLE);

	//  F_SSPCLK / ( CPSDVR * (1 + SCR) ) = 40 / (2 * (1 + 9)) = 2 МГц для режима стирания микросхемы
	My_SSP_InitStruct.SSP_CPSDVSR = 2;
	My_SSP_InitStruct.SSP_SCR = 9;

	SSP_Init(MDR_SSP1, &My_SSP_InitStruct);
	SSP_Cmd(MDR_SSP1, ENABLE);

	static uint16_t CMD = 0;
	static uint16_t cmd_lsb_full_cycle = 0;

  // Формируем данные для командного цикла
	CMD = ((( Num3 | Num2 | Num1 | Num0 | CEB | NVRB | OEB | TMEN | BYTEB_16 | VREAD)|(Num_Sector))<<2) | (1 << 1);
	CMD = MSBtoLSB(CMD, 13) & 0x1FFF;
	// Формируем данные для полного цикла
	cmd_lsb_full_cycle = ((( Num3 | Num2 | Num1 | Num0 | CEB | NVRB | OEB | TMEN | BYTEB_16 | VREAD)|(Num_Sector)) << 2);
	cmd_lsb_full_cycle = MSBtoLSB(cmd_lsb_full_cycle, 13) & 0x1FFF;

	// Передаем командную последовательность для стирания сектора
	// Первый цикл
	// Ждем пока не освободится SPI
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);

	// Устанавливаем PB13 как порт ввода-вывода
	MDR_PORTB->FUNC = MDR_PORTB->FUNC & (~3 << 26);

	// Дополнительный тактовый импульс
	MDR_PORTB->RXTX &= ~PORT_Pin_13;
	MDR_PORTB->RXTX |=  PORT_Pin_13;

	// Передаем команду 13 бит
	MDR_PORTB->RXTX &= ~PORT_Pin_13;
	MDR_PORTB->RXTX &= ~PORT_Pin_12;
	MDR_PORTB->RXTX |= PORT_Pin_12;
	MDR_PORTB->RXTX |=  PORT_Pin_13;
	MDR_PORTB->RXTX &= ~PORT_Pin_12;
	MDR_PORTB->FUNC = ((MDR_PORTB->FUNC & (~3 << 26)) | (PORT_FUNC_ALTER << 26));

	// Передаем команду
	MDR_SSP1->CR0 = (MDR_SSP1->CR0 & 0xFFF0) | SSP_WordLength13b;
	SSP_SendData(MDR_SSP1, CMD);
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);

	// Передаем адрес
	MDR_SSP1->CR0 = (MDR_SSP1->CR0 & 0xFFF0) | SSP_WordLength12b;
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_TNF) == RESET);
	SSP_SendData(MDR_SSP1, ADDR_555);
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);

	// Передаем данные
	MDR_SSP1->CR0 = (MDR_SSP1->CR0 & 0xFFF0) | SSP_WordLength8b;
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_TNF) == RESET);
	SSP_SendData(MDR_SSP1, DATA_AA);
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);

	// Дополнительно четыре тактовых импульса CLK
	MDR_SSP1->CR0 = (MDR_SSP1->CR0&0xFFF0) | SSP_WordLength4b;
	SSP_SendData(MDR_SSP1, 0x00);
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);

	// Второй командный цикл
	// Устанавливаем PB13 как порт ввода-вывода
	MDR_PORTB->FUNC = MDR_PORTB->FUNC & (~3 << 26);
	// Передаем команду 13 бит
	MDR_PORTB->RXTX &= ~PORT_Pin_13;
	MDR_PORTB->RXTX &= ~PORT_Pin_12;
	MDR_PORTB->RXTX |= PORT_Pin_12;
	MDR_PORTB->RXTX |=  PORT_Pin_13;
	MDR_PORTB->RXTX &= ~PORT_Pin_12;
	MDR_PORTB->FUNC = ((MDR_PORTB->FUNC&(~3 << 26)) | (PORT_FUNC_ALTER << 26));

	// Передаем команду
	MDR_SSP1->CR0 = (MDR_SSP1->CR0 & 0xFFF0) | SSP_WordLength13b;
	SSP_SendData(MDR_SSP1, CMD);
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);
	// Передаем адрес
	MDR_SSP1->CR0 = (MDR_SSP1->CR0 & 0xFFF0) | SSP_WordLength12b;
	SSP_SendData(MDR_SSP1, ADDR_AAA);
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);
	// Передаем данные
	MDR_SSP1->CR0 = (MDR_SSP1->CR0 & 0xFFF0) | SSP_WordLength8b;
	SSP_SendData(MDR_SSP1, DATA_55);
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);

	// Дополнительно четыре тактовых импульса CLK
	MDR_SSP1->CR0 = (MDR_SSP1->CR0 & 0xFFF0) | SSP_WordLength4b;
	SSP_SendData(MDR_SSP1, 0x00);
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);

	// Третий командный цикл
	// Устанавливаем PB13 как порт ввода-вывода
	MDR_PORTB->FUNC = MDR_PORTB->FUNC & (~3 << 26);
	// Передаем команду 13 бит
	MDR_PORTB->RXTX &= ~PORT_Pin_13;
	MDR_PORTB->RXTX &= ~PORT_Pin_12;
	MDR_PORTB->RXTX |= PORT_Pin_12;
	MDR_PORTB->RXTX |=  PORT_Pin_13;
	MDR_PORTB->RXTX &= ~PORT_Pin_12;
	MDR_PORTB->FUNC = ((MDR_PORTB->FUNC & (~3 << 26)) | (PORT_FUNC_ALTER << 26));

	// Передаем команду
	MDR_SSP1->CR0 = (MDR_SSP1->CR0 & 0xFFF0) | SSP_WordLength13b;
	SSP_SendData(MDR_SSP1, CMD);
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);

	// Передаем адрес
	MDR_SSP1->CR0 = (MDR_SSP1->CR0 & 0xFFF0) | SSP_WordLength12b;
	SSP_SendData(MDR_SSP1, ADDR_555);
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);

	// Передаем данные
	MDR_SSP1->CR0 = (MDR_SSP1->CR0 & 0xFFF0) | SSP_WordLength8b;
	SSP_SendData(MDR_SSP1, DATA_80);
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);

	// Дополнительно четыре тактовых импульса CLK
	MDR_SSP1->CR0 = (MDR_SSP1->CR0 & 0xFFF0) | SSP_WordLength4b;
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_TNF) == RESET);
	SSP_SendData(MDR_SSP1, 0x00);
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);

	// Четвертый командный цикл
	// Устанавливаем PB13 как порт ввода-вывода
	MDR_PORTB->FUNC = MDR_PORTB->FUNC & (~3 << 26);

	// Передаем команду 13 бит
	MDR_PORTB->RXTX &= ~PORT_Pin_13;
	MDR_PORTB->RXTX &= ~PORT_Pin_12;
	MDR_PORTB->RXTX |= PORT_Pin_12;
	MDR_PORTB->RXTX |=  PORT_Pin_13;
	MDR_PORTB->RXTX &= ~PORT_Pin_12;
	MDR_PORTB->FUNC = ((MDR_PORTB->FUNC & (~3 << 26)) | (PORT_FUNC_ALTER << 26));

	// Передаем команду
	MDR_SSP1->CR0 = (MDR_SSP1->CR0 & 0xFFF0) | SSP_WordLength13b;
	SSP_SendData(MDR_SSP1, CMD);
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);

	// Передаем адрес
	MDR_SSP1->CR0 = (MDR_SSP1->CR0 & 0xFFF0) | SSP_WordLength12b;
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_TNF) == RESET);
	SSP_SendData(MDR_SSP1, ADDR_555);
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);

	// Передаем данные
	MDR_SSP1->CR0 = (MDR_SSP1->CR0&0xFFF0) | SSP_WordLength8b;
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_TNF) == RESET);
	SSP_SendData(MDR_SSP1, DATA_AA);
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);

	// Дополнительно четыре тактовых импульса CLK
	MDR_SSP1->CR0 = (MDR_SSP1->CR0&0xFFF0) | SSP_WordLength4b;
	SSP_SendData(MDR_SSP1, 0x00);
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);

	// Пятый командный цикл
	// Устанавливаем PB13 как порт ввода-вывода
	MDR_PORTB->FUNC = MDR_PORTB->FUNC&(~3 << 26);
	// Передаем команду 13 бит
	MDR_PORTB->RXTX &= ~PORT_Pin_13;
	MDR_PORTB->RXTX &= ~PORT_Pin_12;
	MDR_PORTB->RXTX |= PORT_Pin_12;
	MDR_PORTB->RXTX |=  PORT_Pin_13;
	MDR_PORTB->RXTX &= ~PORT_Pin_12;
	MDR_PORTB->FUNC = ((MDR_PORTB->FUNC&(~3 << 26)) | (PORT_FUNC_ALTER << 26));

	// Передаем команду
	MDR_SSP1->CR0 = (MDR_SSP1->CR0 & 0xFFF0) | SSP_WordLength13b;
	SSP_SendData(MDR_SSP1, CMD);
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);
	// Передаем адрес
	MDR_SSP1->CR0 = (MDR_SSP1->CR0&0xFFF0) | SSP_WordLength12b;
	SSP_SendData(MDR_SSP1, ADDR_AAA);
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);
	// Передаем данные
	MDR_SSP1->CR0 = (MDR_SSP1->CR0&0xFFF0) | SSP_WordLength8b;
	SSP_SendData(MDR_SSP1, DATA_55);
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);

	// Дополнительно четыре тактовых импульса CLK
	MDR_SSP1->CR0 = (MDR_SSP1->CR0&0xFFF0) | SSP_WordLength4b;
	SSP_SendData(MDR_SSP1, 0x00);
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);

	// Шестой командный цикл
	// Стираем блок микросхемы
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);
	// Устанавливаем PB13 как порт ввода-вывода
	MDR_PORTB->FUNC = MDR_PORTB->FUNC & (~3 << 26);
	// Передаем команду 13 бит
	MDR_PORTB->RXTX &= ~PORT_Pin_13;
	MDR_PORTB->RXTX &= ~PORT_Pin_12;
	MDR_PORTB->RXTX |= PORT_Pin_12;
	MDR_PORTB->RXTX |=  PORT_Pin_13;
	MDR_PORTB->RXTX &= ~PORT_Pin_12;
	MDR_PORTB->FUNC = ((MDR_PORTB->FUNC & (~3 << 26)) | (PORT_FUNC_ALTER << 26));

	// Передаем команду
	MDR_SSP1->CR0 = (MDR_SSP1->CR0 & 0xFFF0) | SSP_WordLength13b;
	SSP_SendData(MDR_SSP1, cmd_lsb_full_cycle);
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);
	// Передаем адрес
	MDR_SSP1->CR0 = (MDR_SSP1->CR0&0xFFF0) | SSP_WordLength16b;
	SSP_SendData(MDR_SSP1, ADDR_555_16);
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);
	// Передаем данные
	SSP_SendData(MDR_SSP1, DATA_10);
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);

	// Стирание микросхемы осуществляется при тактовой частоте 2 МГц

  My_PortInitStructure.PORT_Pin = (PORT_Pin_14 | PORT_Pin_15);
  My_PortInitStructure.PORT_OE = PORT_OE_IN;
  My_PortInitStructure.PORT_FUNC = PORT_FUNC_PORT;
  My_PortInitStructure.PORT_SPEED = PORT_SPEED_MAXFAST;
  My_PortInitStructure.PORT_MODE = PORT_MODE_DIGITAL;
	PORT_Init(MDR_PORTB, &My_PortInitStructure);

	uint32_t i = 28000;
	uint32_t busy = 0;
	uint32_t busy_up = 0;
	uint32_t busy_down = 0;
	uint32_t busy_up_again = 0;

	while(i > 0){
		while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_TNF) == RESET);
		SSP_SendData(MDR_SSP1, 0x00);
		busy = PORT_ReadInputDataBit(MDR_PORTB, PORT_Pin_14);
		if((busy == 1) && (busy_up == 0)){ // Busy поднялся
			busy_up = 1;
		}
		else{
			if((busy == 0) && (busy_up == 1)) { // Busy опустился
				busy_down = 1;
			}
			else{
				if((busy == 1) && (busy_up == 1) && (busy_down == 1)){
					busy_up_again = 1;
					break;
				}
			}
		}
		i--;
	}
	// Ждем выдачу всех данных из FIFO
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);

	PORT_ResetBits(MDR_PORTB, PORT_Pin_12);
	if(busy_up_again == 1){
		return ERR_OK;
	}
	else{
		return ERR_SECTOR_NOT_ERRASE;
	}

} // void Erase_Sector_Flash(uint32_t Num_Sector)
#endif // #ifdef SERIAL_MODE


#ifdef SERIAL_MODE
// Функция записи полуслова во внешнюю Flash-память по последовательному интерфейсу
uint32_t Write_Halfword_To_Flash(uint16_t addr, uint16_t data, uint32_t Num_block, uint32_t Num_subblock)
{
	uint16_t CMD = 0;
	uint16_t cmd_lsb_full_cycle = 0;

  My_PortInitStructure.PORT_Pin = (PORT_Pin_13 | PORT_Pin_14 | PORT_Pin_15);
  My_PortInitStructure.PORT_OE = PORT_OE_OUT;
  My_PortInitStructure.PORT_FUNC = PORT_FUNC_ALTER;
  My_PortInitStructure.PORT_SPEED = PORT_SPEED_MAXFAST;
  My_PortInitStructure.PORT_MODE = PORT_MODE_DIGITAL;
	PORT_Init(MDR_PORTB, &My_PortInitStructure);

	// Формируем данные для командного цикла
	CMD = ((( Num3 | Num2 | Num1 | Num0 | CEB | NVRB | OEB | TMEN | BYTEB_16 | VREAD)|(Num_block)|(Num_subblock)) << 2) | (1 << 1);
	CMD = MSBtoLSB(CMD, 13) & 0x1FFF;
	// Формируем данные для полного цикла
	cmd_lsb_full_cycle = (( Num3 | Num2 | Num1 | Num0 | CEB | NVRB | OEB | TMEN | BYTEB_16 | VREAD)|(Num_block)|(Num_subblock)) << 2;
	cmd_lsb_full_cycle = MSBtoLSB(cmd_lsb_full_cycle, 13) & 0x1FFF;

	// Выключение микросхемы
	PORT_SetBits(MDR_PORTE, PORT_Pin_14);
	PORT_SetBits(MDR_PORTC, PORT_Pin_1 | PORT_Pin_2);

	// Устанавливаем PB13 как порт ввода-вывода
	MDR_PORTB->FUNC = MDR_PORTB->FUNC&(~3 << 26);

	// Дополнительный тактовый импульс
	MDR_PORTB->RXTX &= ~PORT_Pin_13;
	MDR_PORTB->RXTX |=  PORT_Pin_13;

	// Первый цикл
	// Ждем пока не освободится SPI
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);

	// Передаем команду 13 бит
	MDR_PORTB->RXTX &= ~PORT_Pin_13;
	MDR_PORTB->RXTX &= ~PORT_Pin_12;
	MDR_PORTB->RXTX |= PORT_Pin_12;
	MDR_PORTB->RXTX |=  PORT_Pin_13;
	MDR_PORTB->RXTX &= ~PORT_Pin_12;
	MDR_PORTB->FUNC = ((MDR_PORTB->FUNC & (~3<<26)) | (PORT_FUNC_ALTER << 26));

	// Передаем команду
	MDR_SSP1->CR0 = (MDR_SSP1->CR0 & 0xFFF0) | SSP_WordLength13b;
	SSP_SendData(MDR_SSP1, CMD);
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);
	// Передаем адрес
	MDR_SSP1->CR0 = (MDR_SSP1->CR0&0xFFF0) | SSP_WordLength12b;
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_TNF) == RESET);
	SSP_SendData(MDR_SSP1, ADDR_555);
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);
	// Передаем данные
	MDR_SSP1->CR0 = (MDR_SSP1->CR0&0xFFF0) | SSP_WordLength8b;
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_TNF) == RESET);
	SSP_SendData(MDR_SSP1, DATA_AA);
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);

	// Дополнительно четыре тактовых импульса CLK
	MDR_SSP1->CR0 = (MDR_SSP1->CR0 & 0xFFF0) | SSP_WordLength4b;
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_TNF) == RESET);
	SSP_SendData(MDR_SSP1, 0x00);
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);

	// Второй командный цикл
	// Устанавливаем PB13 как порт ввода-вывода
	MDR_PORTB->FUNC = MDR_PORTB->FUNC & (~3 << 26);
	// Передаем команду 13 бит
	MDR_PORTB->RXTX &= ~PORT_Pin_13;
	MDR_PORTB->RXTX &= ~PORT_Pin_12;
	MDR_PORTB->RXTX |= PORT_Pin_12;
	MDR_PORTB->RXTX |=  PORT_Pin_13;
	MDR_PORTB->RXTX &= ~PORT_Pin_12;
	MDR_PORTB->FUNC = ((MDR_PORTB->FUNC & (~3<<26)) | (PORT_FUNC_ALTER << 26));

	// Передаем команду
	MDR_SSP1->CR0 = (MDR_SSP1->CR0 & 0xFFF0) | SSP_WordLength13b;
	SSP_SendData(MDR_SSP1, CMD);
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);
	// Передаем адрес
	MDR_SSP1->CR0 = (MDR_SSP1->CR0 & 0xFFF0) | SSP_WordLength12b;
	SSP_SendData(MDR_SSP1, ADDR_AAA);
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);
	// Передаем данные
	MDR_SSP1->CR0 = (MDR_SSP1->CR0 & 0xFFF0) | SSP_WordLength8b;
	SSP_SendData(MDR_SSP1, DATA_55);
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);

	// Дополнительно четыре тактовых импульса CLK
	MDR_SSP1->CR0 = (MDR_SSP1->CR0&0xFFF0) | SSP_WordLength4b;
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_TNF) == RESET);
	SSP_SendData(MDR_SSP1, 0x00);
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);

	// Третий командный цикл
	// Устанавливаем PB13 как порт ввода-вывода
	MDR_PORTB->FUNC = MDR_PORTB->FUNC & (~3 << 26);
	// Передаем команду 13 бит
	MDR_PORTB->RXTX &= ~PORT_Pin_13;
	MDR_PORTB->RXTX &= ~PORT_Pin_12;
	MDR_PORTB->RXTX |= PORT_Pin_12;
	MDR_PORTB->RXTX |=  PORT_Pin_13;
	MDR_PORTB->RXTX &= ~PORT_Pin_12;
	MDR_PORTB->FUNC = ((MDR_PORTB->FUNC & (~3 << 26)) | (PORT_FUNC_ALTER << 26));

	// Передаем команду
	MDR_SSP1->CR0 = (MDR_SSP1->CR0 & 0xFFF0) | SSP_WordLength13b;
	SSP_SendData(MDR_SSP1, CMD);
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);
	// Передаем адрес
	MDR_SSP1->CR0 = (MDR_SSP1->CR0 & 0xFFF0) | SSP_WordLength12b;
	SSP_SendData(MDR_SSP1, ADDR_555);
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);
	// Передаем данные
	MDR_SSP1->CR0 = (MDR_SSP1->CR0 & 0xFFF0) | SSP_WordLength8b;
	SSP_SendData(MDR_SSP1, DATA_A0);
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);

	// Дополнительно четыре тактовых импульса CLK
	MDR_SSP1->CR0 = (MDR_SSP1->CR0 & 0xFFF0) | SSP_WordLength4b;
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_TNF) == RESET);
	SSP_SendData(MDR_SSP1, 0x00);

	// Четвертый цикл - записываем данные
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);
	// Устанавливаем PB13 как порт ввода-вывода
	MDR_PORTB->FUNC = MDR_PORTB->FUNC&(~3 << 26);
	// Передаем команду 13 бит
	MDR_PORTB->RXTX &= ~PORT_Pin_13;
	MDR_PORTB->RXTX &= ~PORT_Pin_12;
	MDR_PORTB->RXTX |= PORT_Pin_12;
	MDR_PORTB->RXTX |=  PORT_Pin_13;
	MDR_PORTB->RXTX &= ~PORT_Pin_12;
	MDR_PORTB->FUNC = ((MDR_PORTB->FUNC & (~3 << 26)) | (PORT_FUNC_ALTER << 26));

	// Передаем адрес
	MDR_SSP1->CR0 = (MDR_SSP1->CR0 & 0xFFF0) | SSP_WordLength13b;
	SSP_SendData(MDR_SSP1, cmd_lsb_full_cycle);
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);
	// Передаем адрес
	MDR_SSP1->CR0 = (MDR_SSP1->CR0 & 0xFFF0) | SSP_WordLength16b;
	SSP_SendData(MDR_SSP1, MSBtoLSB(addr,16));
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);
	// Передаем данные
	SSP_SendData(MDR_SSP1, MSBtoLSB(data,16));
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);

	// Время записи одного байта в память  = 200 мкс
	// При частоте SPI =  5 МГц, время выдачи одного символа составит 0,2 мкс,
	// Тогда для ожидания завершения записи двух байт во FLASH память
	// Потребуется 200 /(16*0,2) = 125 циклов, где для запаса возьмём 164 цикла.

	// Устанавливаем PB15 как порт ввода-вывода
  My_PortInitStructure.PORT_Pin = (PORT_Pin_14 | PORT_Pin_15);
  My_PortInitStructure.PORT_OE = PORT_OE_IN;
  My_PortInitStructure.PORT_FUNC = PORT_FUNC_PORT;
  My_PortInitStructure.PORT_SPEED = PORT_SPEED_MAXFAST;
  My_PortInitStructure.PORT_MODE = PORT_MODE_DIGITAL;
	PORT_Init(MDR_PORTB, &My_PortInitStructure);

	uint32_t i = 64;
	uint32_t busy = 0;
	uint32_t busy_up = 0;
	uint32_t busy_down = 0;
	uint32_t busy_up_again = 0;

	while(i > 0){
		while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_TNF) == RESET);
		SSP_SendData(MDR_SSP1, 0x00);
		busy = PORT_ReadInputDataBit(MDR_PORTB, PORT_Pin_14);
		if((busy == 1) && (busy_up == 0)){ // busy поднялся
			busy_up = 1;
		}
		else{
			if((busy == 0) && (busy_up == 1)) { //  busy опустился
				busy_down = 1;
			}
			else{
				if((busy == 1) && (busy_up == 1) && (busy_down == 1)){
					busy_up_again = 1;
					break;
				}
			}
		}
		i--;
	}
	// Ожидаем выдачи всех данных из FIFO
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);

	PORT_ResetBits(MDR_PORTB, PORT_Pin_12);
	if(busy_up_again == 1){
		return ERR_OK;
	}
	else{
		return ERR_DATA_NOT_WRITTEN;
	}

} // void Write_Halfword_To_Flash(uint16_t addr, uint16_t data, uint32_t Num_block, uint32_t Num_subblock)
#endif // #ifdef SERIAL_MODE

#ifdef SERIAL_MODE
// Функция чтения полуслова из Flash-памяти
uint16_t Read_Halfword_From_Flash(uint16_t addr, uint32_t Num_block, uint32_t Num_subblock)
{
	// Считанные данные из FLASH
	uint16_t Received_Data = 0;
	// Необходимое число оставшихся бит для чтения после считывания BUSY
	uint16_t Remaining_Bits = 0;

  My_PortInitStructure.PORT_Pin = (PORT_Pin_13 | PORT_Pin_14 | PORT_Pin_15);
  My_PortInitStructure.PORT_OE = PORT_OE_OUT;
  My_PortInitStructure.PORT_FUNC = PORT_FUNC_ALTER;
  My_PortInitStructure.PORT_SPEED = PORT_SPEED_MAXFAST;
  My_PortInitStructure.PORT_MODE = PORT_MODE_DIGITAL;
	PORT_Init(MDR_PORTB, &My_PortInitStructure);

	// Формируем команду для чтения двух байт
	uint16_t CMD_read = 0;
	CMD_read = (((Num3 | Num2 | Num1 | Num0 | CEB | NVRB | WEB | TMEN | BYTEB_16 | VREAD) | (Num_block) | (Num_subblock) )<<2 ) | (1 << 0) ;
	CMD_read = MSBtoLSB(CMD_read, 13) & 0x1FFF;

	// Цикл чтения
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);
	// Устанавливаем PB13 как порт ввода-вывода
	MDR_PORTB->FUNC = MDR_PORTB->FUNC & (~3 << 26);
	// Дополнительный тактовый импульс
	MDR_PORTB->RXTX &= ~PORT_Pin_13;
	MDR_PORTB->RXTX |=  PORT_Pin_13;

	// Передаем команду 13 бит
	MDR_PORTB->RXTX &= ~PORT_Pin_13;
	MDR_PORTB->RXTX &= ~PORT_Pin_12;
	MDR_PORTB->RXTX |= PORT_Pin_12;
	MDR_PORTB->RXTX |=  PORT_Pin_13;
	MDR_PORTB->RXTX &= ~PORT_Pin_12;
	MDR_PORTB->FUNC = ((MDR_PORTB->FUNC & (~3 << 26)) | (PORT_FUNC_ALTER << 26));

	// Передаем команду
	MDR_SSP1->CR0 = (MDR_SSP1->CR0 & 0xFFF0) | SSP_WordLength13b;
	SSP_SendData(MDR_SSP1, CMD_read);
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);
	// Передаем адрес
	MDR_SSP1->CR0 = (MDR_SSP1->CR0 & 0xFFF0) | SSP_WordLength16b;
	SSP_SendData(MDR_SSP1, MSBtoLSB(addr,16));
	while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);

	// Читаем записанные данные
	// PB14 - SPI_Rx
	My_PortInitStructure.PORT_Pin = PORT_Pin_14;
	My_PortInitStructure.PORT_OE = PORT_OE_IN;
	My_PortInitStructure.PORT_FUNC = PORT_FUNC_ALTER;
	My_PortInitStructure.PORT_SPEED = PORT_SPEED_MAXFAST;
	My_PortInitStructure.PORT_MODE = PORT_MODE_DIGITAL;
	PORT_Init(MDR_PORTB, &My_PortInitStructure);

	// Устанавливаем PB15 как порт ввода-вывода
	MDR_PORTB->OE  &= ~PORT_Pin_15;
	MDR_PORTB->FUNC = MDR_PORTB->FUNC & (~(3<<30));
	uint16_t tmp = 0;
	uint32_t end_receive_data = 0;
	// Вычитываем FIFO приемника
	while(SSP_GetFlagStatus(MDR_SSP1,SSP_FLAG_RNE) != 0)
		SSP_ReceiveData(MDR_SSP1);

	int i = 0;
	while(1){
		// Передаем пустые данные для получения тактирования
		SSP_SendData(MDR_SSP1, 0x00);
		while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);
		// Считываем принятые данные по SPI
		tmp = SSP_ReceiveData(MDR_SSP1);
		// Ждем пока передадим данные полностью
		while(SSP_GetFlagStatus(MDR_SSP1, SSP_FLAG_BSY) == SET);
		if(end_receive_data){
			Received_Data |= tmp >> (16 - Remaining_Bits);
			Received_Data = LSBtoMSB(Received_Data, 16);
			break;
		}
		// После получении первой "1" считываем данные
		for(i = 0; i < 16; i++){
			if(tmp&(1<<(15 - i))){
				Remaining_Bits = i + 1;
				Received_Data = tmp << Remaining_Bits;
				end_receive_data = 1;
				break;
			}
		}
	}

	PORT_ResetBits(MDR_PORTB, PORT_Pin_12);

	return Received_Data;

} // uint16_t Read_Halfword_From_Flash(uint16_t addr, uint32_t Num_block, uint32_t Num_subblock)
#endif // #ifdef SERIAL_MODE
