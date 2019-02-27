/* PRIYANKA SHAH FENG CAI LAB4 */
// Standard include
#include <stdio.h>
#include <string.h>

#include "hw_uart.h"

#include <stdlib.h>
#include <math.h>
// Driverlib includes
#include "hw_types.h"
#include "interrupt.h"
#include "hw_ints.h"
#include "hw_apps_rcm.h"
#include "hw_common_reg.h"
#include "prcm.h"
#include "rom.h"
#include "rom_map.h"
#include "hw_memmap.h"
#include "timer.h"
#include "utils.h"
#include "uart.h"
#include "interrupt.h"

// Common interface includes
#include "timer_if.h"
#include "gpio.h"
#include "spi.h"
#include "uart_if.h"
#include "gpio_if.h"
#include "udma.h"
#include "pinmux.h"
#include "Adafruit_GFX.h"
#include "udma_if.h"
//#include "Adafruit_OLED.c"
#include "Adafruit_SSD1351.h"

//*****************************************************************************
//                      MACRO DEFINITIONS
//*****************************************************************************
#define MILLISECONDS_TO_TICKS(ms)   ((80000000/1000) * (ms))
#define PIN_READ 0x10
#define REMOTE_BASE GPIOA1_BASE
#define OLEDCS_BASE GPIOA0_BASE
#define OLEDCS_CONTROL 0x80
#define SPI_IF_BIT_RATE  400000
#define TR_BUFF_SIZE     100
#define BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800

#define cursor_x        0
#define cursor_y;       0
//*****************************************************************************
//                      Global Variables for Vector Table
//*****************************************************************************
#if defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif

//*****************************************************************************
//
// Globals used by the timer interrupt handler.
//
//*****************************************************************************
static volatile unsigned long g_ulSysTickValue;
static volatile unsigned long g_ulBase = TIMERA0_BASE;
static volatile unsigned long g_ulRefBase = TIMERA1_BASE;
static volatile unsigned long g_ulRefTimerInts = 0;
static volatile unsigned long g_ulIntClearVector;
static unsigned char g_ucTxBuff[2];
static unsigned char g_ucRxBuff[2];
volatile unsigned int a=0;
char buffer[8] = "\0";
unsigned long rxData1;
unsigned long rxData2;
signed long rxData=-1;
unsigned long txDummy;
unsigned long g_ulTimerInts;
unsigned long rxBuff[10];
unsigned long rxDataBuff[410];
unsigned long main_coeff[8];
volatile short old, new;
int nowCount = -1;

int SAMPLE_SPACE=410;
int flagSampleInt=12;
int x=0;
int f_tone[8]={697, 770, 852, 941, 1209, 1336, 1477, 1633}; // frequencies of rows & columns
//int f_tone_final[8]={205587, 205544, 205491, 205427, 205195, 205066, 204906, 204712};
long int f_tone_final[8]={31548, 31281, 30951, 30556, 29144, 28361, 27409};
int count_array[12];

long int fPower[8];
int new_dig = -1;
long int power_all[8];

char button_signal; //this is the data returned by the post_test() function
char alpha_pressed;
int newCounting[12];
int counting[4][3];
int indexi=0;
int indexj=0;
int indexi2=0;
int indexj2=0;
volatile int direction;
volatile int buttom_x, buttom_y;
volatile int duration, bufferLength;
volatile int diff;
char signalFlag;

//*****************************************************************************
//
//! The interrupt handler for the first timer interrupt.
//!
//! \param  None
//!
//! \return none
//
//*****************************************************************************

void resetMatrix(void)
{
    int i;
    int j;
    for (i = 0; i < 4; i++) {
        for(j=0;j<4;j++)
        {
            counting[i][j] = 0;
        }
    }
}
short maxIndex(short array[], short length)
{
    short i, index = 0, tmp = 0;
    for (i = 0; i < length; i++) {
        if (array[i] > tmp) {
            tmp = array[i];
            index = i;
        }
    }
    return index;
}
/*
void maxIndex(void)
{
    int i,j,tmp = 0;
    indexi = 0;
    indexj=0;
    for (i = 0; i < 4; i++) {
        for(j = 0; j < 4; j++)
        {
            if (counting[i][j] > tmp) {
                tmp = counting[i][j];
                indexi = i;
                indexj = j;
            }
        }
    }
}*/

void post_test(void)
//---------------------------------------------------------------//
{
//initialize variables to be used in the function
int i,row,col,max_power;
char button_signal2;
button_signal2=signalFlag;

 char row_col[4][4] =       // array with the order of the digits in the DTMF system
    {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
    };

 //printf("I'm in post_test");
// find the maximum power in the row frequencies and the row number
    max_power=0;            //initialize max_power=0
    for(i=0;i<4;i++)        //loop 4 times from 0>3 (the indecies of the rows)
        {
        if (power_all[i] > max_power)   //if power of the current row frequency > max_power
            {
            max_power=power_all[i];     //set max_power as the current row frequency
            row=i;                      //update row number
            }
        }
// find the maximum power in the column frequencies and the column number
    max_power=0;            //initialize max_power=0
    for(i=4;i<8;i++)        //loop 4 times from 4>7 (the indecies of the columns)
        {
        if (power_all[i] > max_power)   //if power of the current column frequency > max_power
            {
            max_power=power_all[i];     //set max_power as the current column frequency
            col=i;                      //update column number
            }
        }
    //if(power_all[col]<= 1000 && power_all[row] <= 1000) //if the maximum powers equal zero > this means no signal or inter-digit pause

    if(power_all[col]>90000 && power_all[row]>90000) // check if maximum powers of row & column exceed certain threshold AND new_dig flag is set to 1
        {
            button_signal=row_col[row][col-4];
            signalFlag=button_signal;
           //printf("\nData out: %d\n\n", row_col[row][col-4]);
            if(signalFlag !=button_signal2)
            {
                diff=1;
                MAP_UtilsDelay(10000000);
            }
            else if (signalFlag == button_signal2)
            {
                MAP_TimerDisable(g_ulBase, TIMER_A);
                //MAP_UtilsDelay(MILLi);
                MAP_UtilsDelay(10000000);
                MAP_TimerEnable(g_ulBase, TIMER_A);
                //button_signal='\0';
                diff=1;



            }
            counting[row][col-4]++;
            indexi=row;
            indexj=col-4;
         //   printf("count: %d\n\n", counting[row][col-4]);

            Timer_IF_InterruptClear(g_ulRefBase);
            MAP_TimerEnable(g_ulRefBase, TIMER_A);


        }
}


char buttonToChar(char button, int count)
{
    short tmp;
    char result;
    switch(button) {
    case '2':
        tmp = count % 3;
        if (tmp) {
            result = 'a' + tmp - 1;
        } else {
            result = 'a' + 2;
        }
        break;
    case '3':
        tmp = count % 3;
        if (tmp) {
            result = 'd' + tmp - 1;
        } else {
            result = 'd' + 2;
        }
        break;
    case '4':
        tmp = count % 3;
        if (tmp) {
            result = 'g' + tmp - 1;
        } else {
            result = 'g' + 2;
        }
        break;
    case '5':
        tmp = count % 3;
        if (tmp) {
            result = 'j' + tmp - 1;
        } else {
            result = 'j' + 2;
        }
        break;
    case '6':
        tmp = count % 3;
        if (tmp) {
            result = 'm' + tmp - 1;
        } else {
            result = 'm' + 2;
        }
        break;
    case '7':
        tmp = count % 4;
        if (tmp) {
            result = 'p' + tmp - 1;
        } else {
            result = 'p' + 3;
        }
        break;
    case '8':
        tmp = count % 3;
        if (tmp) {
            result = 't' + tmp - 1;
        } else {
            result = 't' + 2;
        }
        break;
    case '9':
        tmp = count % 4;
        if (tmp) {
            result = 'w' + tmp - 1;
        } else {
            result = 'w' + 3;
        }
        break;
    case '0':
        result = ' ';
        break;

    default: result = '\0'; break;
    }
    return result;
}

long int goertzel(int sample[], long int coeff, int N)
//---------------------------------------------------------------//
{
//initialize variables to be used in the function
int Q, Q_prev, Q_prev2,i;
long prod1,prod2,prod3,power;

    Q_prev = 0;         //set delay element1 Q_prev as zero
    Q_prev2 = 0;        //set delay element2 Q_prev2 as zero
    power=0;            //set power as zero

    for (i=0; i<N; i++) // loop N times and calculate Q, Q_prev, Q_prev2 at each iteration
        {
            Q = (sample[i]) + ((coeff* Q_prev)>>14) - (Q_prev2); // >>14 used as the coeff was used in Q15 format
            Q_prev2 = Q_prev;                                    // shuffle delay elements
            Q_prev = Q;
        }

        //calculate the three products used to calculate power
        prod1=( (long) Q_prev*Q_prev);
        prod2=( (long) Q_prev2*Q_prev2);
        prod3=( (long) Q_prev *coeff)>>14;
        prod3=( prod3 * Q_prev2);

        power = ((prod1+prod2-prod3))>>8; //calculate power using the three products and scale the result down

        return power;
}


unsigned long ADC(void)
{
        MAP_SPICSEnable(GSPI_BASE);
        //CS enable Pin3
        GPIOPinWrite(GPIOA1_BASE, 0x10,0);
        MAP_SPITransfer(GSPI_BASE,g_ucTxBuff,g_ucRxBuff,2,
        SPI_CS_ENABLE|SPI_CS_DISABLE);
        //MAP_SPIDataPut(GSPI_BASE,txDummy);
        //MAP_SPIDataGet(GSPI_BASE,&rxData1);

        //MAP_SPIDataPut(GSPI_BASE,txDummy);
        //MAP_SPIDataGet(GSPI_BASE,&rxData2);

        //CS Disable Pin3
        GPIOPinWrite(GPIOA1_BASE, 0x10, 0x10);

        rxData1=g_ucRxBuff[0];
        rxData2=g_ucRxBuff[1];

        rxData=((0x1f & rxData1) << 5) | ((0xf8 & rxData2) >>3);

        //printf("%d %d\n",g_ucRxBuff[0],g_ucRxBuff[1]);
        //printf("%d\n",rxData);

        return rxData;

}

void
TimerBaseIntHandler(void)
{
    Timer_IF_InterruptClear(g_ulBase);
    flagSampleInt=1;

}

//*****************************************************************************
//
//! The interrupt handler for the second timer interrupt.
//!
//! \param  None
//!
//! \return none
//
//*****************************************************************************
void
TimerRefIntHandler(void)
{
    Timer_IF_InterruptClear(g_ulRefBase);
    resetMatrix();
    MAP_TimerDisable(g_ulRefBase, TIMER_A);
    printf("timer2 \n");
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
static void
BoardInit(void)
{
/* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
  //
  // Set vector table base
  //
#if defined(ccs)
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



void resetArray(short array[], short length)
{
    int i;
    for (i = 0; i < length; i++) {
        array[i] = 0;
    }
}

short maxIndex2(short array[], short length)
{
    short i, index = 0, tmp = 0;
    for (i = 0; i < length; i++) {
        if (array[i] > tmp) {
            tmp = array[i];
            index = i;
        }
    }
    return index;
}


//*****************************************************************************
//
//!    main function demonstrates the use of the timers to generate
//! periodic interrupts.
//!
//! \param  None
//!
//! \return none
//
//*****************************************************************************
int
main(void)
 {
    //
    // Initialize board configurations
    BoardInit();
    //
    // Pinmuxing for LEDs
    //
    PinMuxConfig();

    //
    // configure the LED RED and GREEN
    //
       // fillRect(0,0,6,8, BLACK);


        //
        // Configure SPI interface
        //
        MAP_PRCMPeripheralClkEnable(PRCM_GSPI,PRCM_RUN_MODE_CLK);
        //MAP_PRCMPeripheralReset(PRCM_GSPI);
    MAP_PRCMPeripheralReset(PRCM_GSPI);
           MAP_SPIReset(GSPI_BASE);
        MAP_SPIConfigSetExpClk(GSPI_BASE,MAP_PRCMPeripheralClockGet(PRCM_GSPI),
                         SPI_IF_BIT_RATE,SPI_MODE_MASTER,SPI_SUB_MODE_0,
                         (SPI_SW_CTRL_CS |
                         SPI_4PIN_MODE |
                         SPI_TURBO_OFF |
                         SPI_CS_ACTIVELOW |
                         SPI_WL_8));

        // Enable SPI for communication
        //
    MAP_SPIEnable(GSPI_BASE);


    Adafruit_Init();
    fillScreen(BLUE);
    // Configuring the timers
    Timer_IF_Init(PRCM_TIMERA0, g_ulBase, TIMER_CFG_PERIODIC, TIMER_A, 0);
    Timer_IF_Init(PRCM_TIMERA1, g_ulRefBase, TIMER_CFG_PERIODIC, TIMER_A, 0);
    // Setup the interrupts for the timer timeouts.
    Timer_IF_IntSetup(g_ulBase, TIMER_A, TimerBaseIntHandler);
    Timer_IF_IntSetup(g_ulRefBase, TIMER_A, TimerRefIntHandler);

    //Coeff

    //
    // Turn on the timers feeding values in mSec
    //
    //MAP_TimerLoadSet(g_ulBase, TIMER_A, MILLISECONDS_TO_TICKS(500));
    //Timer_IF_Start(g_ulBase, TIMER_A, 1);

    //Sampling Interupt
    MAP_TimerLoadSet(g_ulBase,TIMER_A, MILLISECONDS_TO_TICKS(0.0625));
    MAP_TimerEnable(g_ulBase,TIMER_A);
    MAP_TimerLoadSet(g_ulRefBase, TIMER_A, MILLISECONDS_TO_TICKS(6000));
    //GPIOPinWrite(OLEDCS_BASE, OLEDCS_CONTROL, 0);
    //fillScreen(RED);



       int i;
       for (i=0;i<8;i++)
       {
           main_coeff[i]=f_tone_final[i];

           //main_coeff[i]=(2*(2*3.14*cos(f_tone[i]/9615)))*(1<<14);
           //printf("MainCoeff: %d\n",main_coeff[i]);
       }
       //calculate coeff at each frquency - Q15 format


    //
       int complete=2;
       int count=0;
       setCursor(0, 0);
       buttom_x = 0;
       buttom_y = 64;
       direction = 0;
       bufferLength = 0;


    while(1)
       {


        if(flagSampleInt==1)
        {
            flagSampleInt=0;
            rxDataBuff[x]=ADC()-400;
            x++;
            if(x>SAMPLE_SPACE)
            {
                x=0;
                int count=0;
                MAP_TimerDisable(g_ulBase, TIMER_A);
                int j;
                for (j=0; j<8; j++)
                {
                fPower[j]=goertzel(rxDataBuff, main_coeff[j],SAMPLE_SPACE);
                power_all[j] =fPower[j];
                }


             //   new=maxIndex(newCounting,12);
                post_test();//Gives charactor presses to button_signal

                alpha_pressed = buttonToChar(button_signal, counting[indexi][indexj]);
                if(diff)
                {
                    diff=0;
                    printf("Alpha: %c Count: %d\n", alpha_pressed,counting[indexi][indexj] );

                    //WHERE TO PRINT CHAR
                }
                MAP_UtilsDelay(400000);
                if(indexi==3 && indexj ==0) //MUTE OR STAR
                {
                    printf("*\n");
                }

                if(indexi==3 && indexj ==2) //#
                {
                    printf("#\n");
                }

                button_signal=(' ');
                complete=1;


                indexi=0;
                indexj=0;
            }
        }

        if (complete==1)
                      {
                          MAP_TimerEnable(g_ulBase, TIMER_A);
                          complete=0;
                      }
    }
}



//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
