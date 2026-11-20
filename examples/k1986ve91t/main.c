#include <MDR32FxQI_port.h>
#include <MDR32FxQI_rst_clk.h>
#include <FLASH_test.h>
#include "MDR32FxQI_ssp.h"

void Delay(int waitTicks);

#define LED_PERIOD 500000
#define VD7 PORT_Pin_10
#define VD8 PORT_Pin_11
#define VD9 PORT_Pin_12
#define VD10 PORT_Pin_13
#define VD11 PORT_Pin_14

#define LED_ALL VD7 | VD8 | VD9 | VD10 | VD11
static SSP_InitTypeDef  My1_SSP_InitStruct;
uint16_t result;

int main()
{
	 RST_CLK_DeInit();
   RST_CLK_HSEconfig(RST_CLK_HSE_ON);
   if(RST_CLK_HSEstatus() != SUCCESS){
      while (1);
   }
   RST_CLK_CPU_PLLconfig(RST_CLK_CPU_PLLsrcHSEdiv1, RST_CLK_CPU_PLLmul10);
   RST_CLK_CPU_PLLcmd(ENABLE);
   if(RST_CLK_CPU_PLLstatus() == ERROR) {
      while (1);
   }

  RST_CLK_CPU_PLLuse(ENABLE);
  RST_CLK_CPUclkPrescaler(RST_CLK_CPUclkDIV1);
  RST_CLK_PCLKcmd(RST_CLK_PCLK_EEPROM, ENABLE);
  RST_CLK_PCLKcmd(RST_CLK_PCLK_EEPROM, DISABLE);
  RST_CLK_PCLKcmd(RST_CLK_PCLK_BKP, ENABLE);
  RST_CLK_CPUclkSelection(RST_CLK_CPUclkCPU_C3);

  PORT_InitTypeDef GPIOInitStruct;

  RST_CLK_PCLKcmd (RST_CLK_PCLK_PORTD | RST_CLK_PCLK_SSP1 | RST_CLK_PCLK_PORTB, ENABLE);
	MDR_RST_CLK->SSP_CLOCK |= (1 << 24);
  PORT_StructInit(&GPIOInitStruct);

  GPIOInitStruct.PORT_Pin        = LED_ALL;
  GPIOInitStruct.PORT_OE         = PORT_OE_OUT;
  GPIOInitStruct.PORT_SPEED      = PORT_SPEED_SLOW;
  GPIOInitStruct.PORT_MODE       = PORT_MODE_DIGITAL;
  PORT_Init(MDR_PORTD, &GPIOInitStruct);

	vTest_Ext_FLASH(); // Запуск теста

  while (1)
  {
		PORT_SetBits(MDR_PORTD, LED_ALL);	// LED ON
		Delay(LED_PERIOD);

		PORT_ResetBits(MDR_PORTD, LED_ALL); // LED OFF
		Delay(LED_PERIOD);
  }
}


void Delay(int waitTicks)
{
  int i;
  for (i = 0; i < waitTicks; i++)
  {
    __NOP();
  }
}
