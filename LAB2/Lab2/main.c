//*****************************************************************************
//
// Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/
//
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions
//  are met:
//
//    Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the
//    distribution.
//
//    Neither the name of Texas Instruments Incorporated nor the names of
//    its contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
//  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************

//*****************************************************************************
//
// Application Name     - Timer Count Capture
// Application Overview - This application showcases Timer's count capture 
//                        feature to measure frequency of an external signal.
// Application Details  -
// http://processors.wiki.ti.com/index.php/CC32xx_Timer_Count_Capture_Application
// or
// docs\examples\CC32xx_Timer_Count_Capture_Application.pdf
//
//*****************************************************************************

// Driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "interrupt.h"
#include "prcm.h"
#include "gpio.h"
#include "utils.h"
#include "uart.h"
#include "timer.h"
#include "rom.h"
#include "rom_map.h"
#include "pin.h"
#include "math.h"
#include "string.h"
#include "spi.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1351.h"
#include "glcdfont.h"
// Common interface includes
#include "uart_if.h"
#include "pinmux.h"
#include "string.h"


#define APPLICATION_VERSION     "1.1.1"
#define APP_NAME        "Timer Count Capture"


#define MASTER 			1
//#define SLAVE			0

#define SPI_IF_BIT_RATE  100000
//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
//static unsigned long g_ulSamples[2];

//static char number[100];

//static int value = 1;
static int i = 0;
//static int x = 0;
static int y = 0;
static char asc = 0;
//static int index = 0;
//static int delta = 0;
#if defined(ccs) || defined(gcc)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************


//*****************************************************************************
//
//! Timer interrupt handler
//
//*****************************************************************************
/*static void TimerIntHandler()
{

	//
    // Clear the interrupt
    //
	g_ulSamples[1] = MAP_TimerValueGet(TIMERA1_BASE,TIMER_A);
	TimerLoadSet(TIMERA1_BASE, TIMER_A,0xffff);
	//Report("%d", value);

    //
    // Get the samples and compute the frequency
    //
	//number[i] =
    g_ulSamples[0] = g_ulSamples[1];

   // Report("sample 0: %d\n\r", g_ulSamples[0]);
    //Report("sample 1: %d\n\r", g_ulSamples[1]);

    delta = g_ulSamples[1];
    if(delta < 54000)
    	value = 0;
    else
    	value = 1;
    if(i==0)
    	value = 0;
	number[i] = (char)value;
	i++;
    //if(delta > 6000)
    //	delta = 65535/2 - delta;
    //Report("delta: %d\n\r", delta);
	MAP_TimerIntClear(TIMERA1_BASE,TIMER_CAPA_EVENT);


}
*/

//*****************************************************************************
//
//! Application startup display on UART
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void
DisplayBanner(char * AppName)
{

    Report("\n\n\n\r");
    Report("\t\t *************************************************\n\r");
    Report("\t\t\t  CC3200 %s Application       \n\r", AppName);
    Report("\t\t *************************************************\n\r");
    Report("\n\n\n\r");
}

//*****************************************************************************
//
//! Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
void
BoardInit(void)
{
/* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
  //
  // Set vector table base
  //
#if defined(ccs) || defined(gcc)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif
    //
    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}


/*void delay(unsigned long ulCount){
	int i;

  do{
    ulCount--;
		for (i=0; i< 63000; i++) ;
	}while(ulCount);
}
*/
//*****************************************************************************
//
//! Main  Function
//
//*****************************************************************************
int main()
{


    BoardInit();
    PinMuxConfig();
    //InitTerm();
    //DisplayBanner(APP_NAME);
    MAP_PRCMPeripheralClkEnable(PRCM_GSPI,PRCM_RUN_MODE_CLK);
	MAP_PRCMPeripheralReset(PRCM_GSPI);
    //MAP_SPIReset(GSPI_BASE);
    MAP_SPIConfigSetExpClk(GSPI_BASE,MAP_PRCMPeripheralClockGet(PRCM_GSPI),
					SPI_IF_BIT_RATE,SPI_MODE_MASTER,SPI_SUB_MODE_3,
					(SPI_SW_CTRL_CS |
					SPI_4PIN_MODE |
					SPI_TURBO_OFF |
					SPI_CS_ACTIVELOW |
					SPI_WL_8));
	MAP_SPIEnable(GSPI_BASE);
	//MAP_SPICSEnable(GSPI_BASE);
	Adafruit_Init();
	fillScreen(RED);
	//fillCircle(0,0,10,BLUE);

	//MAP_PinConfigSet(PIN_02,PIN_TYPE_STD_PU,PIN_STRENGTH_6MA);
   /* MAP_TimerIntRegister(TIMERA1_BASE,TIMER_A,TimerIntHandler);
    MAP_TimerConfigure(TIMERA1_BASE, (TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_CAP_TIME));
    MAP_TimerControlEvent(TIMERA1_BASE,TIMER_A,TIMER_EVENT_NEG_EDGE);
    MAP_TimerLoadSet(TIMERA1_BASE,TIMER_A,0xffff);
    MAP_TimerIntEnable(TIMERA1_BASE,TIMER_CAPA_EVENT);
    MAP_TimerEnable(TIMERA1_BASE,TIMER_A);
*/
    //Enable and set up the UARTA1
   // MAP_UARTEnable(UARTA1_BASE);
   // MAP_UARTFIFOEnable(UARTA1_BASE);
   // MAP_UARTIntRegister(UARTA1_BASE,UARTIntHandler);
    //MAP_UARTIntEnable(UARTA1_BASE,UART_INT_DMARX);
  /*  MAP_UARTConfigSetExpClk(UARTA1_BASE,MAP_PRCMPeripheralClockGet(PRCM_UARTA1),
                             UART_BAUD_RATE,
                             (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                             UART_CONFIG_PAR_NONE));

*/

     while(1){

        /* VERTICAL
         *  fillRect(0, 0, 128, 16, RED);
         fillRect(0, 16, 128, 16, BLUE);
         fillRect(0, 32, 128, 16, GREEN);
         fillRect(0, 48, 128, 16, YELLOW);
         fillRect(0, 64, 128, 16, CYAN);
         fillRect(0, 80, 128, 16, BLACK);
         fillRect(0, 96, 128, 16, WHITE);
         fillRect(0, 112, 128, 16, 0xF34F);

         */

          /*//HORIZONTAL
                   fillRect(0, 0, 16, 128, RED);
                 fillRect(16, 0, 16, 128, BLUE);
                 fillRect(32, 0, 16, 128, GREEN);
                 fillRect(48, 0, 16, 128, YELLOW);
                 fillRect(64, 0, 16, 128, CYAN);
                 fillRect(80, 0, 16, 128, BLACK);
                 fillRect(96, 0, 16, 128, WHITE);
                 fillRect(112, 0, 16, 128, 0xF34F);

*/

      /*   drawChar(0,50,'H', BLUE,1,2);

         drawChar(10,50,'E', BLUE,1,2);
         drawChar(20,50,'L', BLUE,1,2);
         drawChar(30,50,'L', BLUE,1,2);
         drawChar(40,50,'O', BLUE,1,2);
         drawChar(50,50,' ', BLUE,1,2);
         drawChar(60,50,'W', BLUE,1,2);
         drawChar(70,50,'O', BLUE,1,2);

         drawChar(80,50,'R', BLUE,1,2);
         drawChar(90,50,'L', BLUE,1,2);
         drawChar(100,50,'D', BLUE,1,2);
         */

       /*  while(1){


             if (x> 127 | y > 127){
                 break;
             }
             drawChar(x, y, asc, YELLOW, BLACK, 1);
             x+=6;
             if(x > 122){
                 x = 0;
                 y+=8;

             }
             if(x> 128 & y> 128){
                 fillScreen(BLACK);
                 delay(1000);
                    x = 0;
                    y = 0;
                    y+=16;
             }



             if(asc == 255)
                 break;
             else
                 asc++;


         }
         delay(300);
*/

        // while (i< 10){
          //drawChar(int x, int y, unsigned char c, unsigned int color, unsigned int bg, unsigned char size)
             //font[i];
             //i++;


      /*   testlines();
         delay(300);
         testfastlines();
         delay(300);
        testdrawrects();
         delay(300);
         testfillrects(RED, BLUE); //eh
         delay(300);
         testfillcircles(26, YELLOW);
         delay(300);
       testroundrects();
         delay(300);
         testtriangles();
         delay(300);
*/
         I2C_IF_Read(0x02, , 1);



	}




}
