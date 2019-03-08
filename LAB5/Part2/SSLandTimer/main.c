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
// Application Name     -   SSL Demo
// Application Overview -   This is a sample application demonstrating the
//                          use of secure sockets on a CC3200 device.The
//                          application connects to an AP and
//                          tries to establish a secure connection to the
//                          Google server.
// Application Details  -
// docs\examples\CC32xx_SSL_Demo_Application.pdf
// or
// http://processors.wiki.ti.com/index.php/CC32xx_SSL_Demo_Application
//
//*****************************************************************************


//*****************************************************************************
//
//! \addtogroup ssl
//! @{
//
//*****************************************************************************
void TimerBaseIntHandler(void);
void TimerRefIntHandler(void);
static void UARTIntHandler();

#include <stdio.h>
// Simplelink includes
#include "simplelink.h"
#include <string.h>
//Driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "rom.h"
#include "rom_map.h"
#include "interrupt.h"
#include "prcm.h"
#include "utils.h"
#include "uart.h"

//Common interface includes
#include "pinmux.h"
#include "gpio_if.h"
#include "common.h"
#include "uart_if.h"



#include <stdlib.h>
#include <math.h>
// Driverlib includes
#include "hw_types.h"
#include "interrupt.h"
#include "hw_ints.h"
#include "hw_apps_rcm.h"
#include "hw_common_reg.h"

#include "rom.h"
#include "rom_map.h"
#include "hw_memmap.h"
#include "timer.h"


// Common interface includes
#include "timer_if.h"

#include "spi.h"
#include "uart_if.h"
#include "gpio_if.h"
#include "udma.h"
#include "Adafruit_GFX.h"
#include "udma_if.h"
//#include "Adafruit_OLED.c"
#include "Adafruit_SSD1351.h"



#define MAX_URI_SIZE 128
#define URI_SIZE MAX_URI_SIZE + 1


#define APPLICATION_NAME        "SSL"
#define APPLICATION_VERSION     "1.1.1.EEC.Spring2018"
#define SERVER_NAME             "a2c96qsr6cjgtl-ats.iot.us-west-2.amazonaws.com"
#define GOOGLE_DST_PORT         8443

//#define SL_SSL_CA_CERT "/cert/StarfieldClass2CA.crt.der" //starfield class2 rootca (from firefox) // <-- this one works
#define SL_SSL_CA_CERT "/cert/feng_root.der"
#define SL_SSL_PRIVATE "/cert/feng_private.der"
#define SL_SSL_CLIENT  "/cert/feng_client.der"


//NEED TO UPDATE THIS FOR IT TO WORK!
#define DATE                1    /* Current Date */
#define MONTH               3     /* Month 1-12 */
#define YEAR                2019  /* Current year */
#define HOUR                10    /* Time - hours */
#define MINUTE              39    /* Time - minutes */
#define SECOND              0     /* Time - seconds */

#define POSTHEADER "POST /things/CC3200_Thing/shadow HTTP/1.1\n\r"
#define HOSTHEADER "Host: a2c96qsr6cjgtl-ats.iot.us-west-2.amazonaws.com\r\n"
#define CHEADER "Connection: Keep-Alive\r\n"
#define CTHEADER "Content-Type: application/json; charset=utf-8\r\n"
#define CLHEADER1 "Content-Length: "
#define CLHEADER2 "\r\n\r\n"

#define DATA1 "{\"state\": {\r\n\"desired\" : {\r\n\"var\" : \"Hello phone, message from CC3200 via AWS IoT!\"\r\n}}}\r\n\r\n"
#define text1  "{\"state\": {\r\n\"desired\" : {\r\n\"var\" : \"A\"\r\n}}}\r\n\r\n"

// Application specific status/error codes
typedef enum{
    // Choosing -0x7D0 to avoid overlap w/ host-driver's error codes
    LAN_CONNECTION_FAILED = -0x7D0,
    INTERNET_CONNECTION_FAILED = LAN_CONNECTION_FAILED - 1,
    DEVICE_NOT_IN_STATION_MODE = INTERNET_CONNECTION_FAILED - 1,

    STATUS_CODE_MAX = -0xBB8
}e_AppStatusCodes;

typedef struct
{
   /* time */
   unsigned long tm_sec;
   unsigned long tm_min;
   unsigned long tm_hour;
   /* date */
   unsigned long tm_day;
   unsigned long tm_mon;
   unsigned long tm_year;
   unsigned long tm_week_day; //not required
   unsigned long tm_year_day; //not required
   unsigned long reserved[3];
}SlDateTime;


//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
volatile unsigned long  g_ulStatus = 0;//SimpleLink Status
unsigned long  g_ulPingPacketsRecv = 0; //Number of Ping Packets received
unsigned long  g_ulGatewayIP = 0; //Network Gateway IP address
unsigned char  g_ucConnectionSSID[SSID_LEN_MAX+1]; //Connection SSID
unsigned char  g_ucConnectionBSSID[BSSID_LEN_MAX]; //Connection BSSID
signed char    *g_Host = SERVER_NAME;
SlDateTime g_time;
#if defined(ccs) || defined(gcc)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif
//*****************************************************************************
//                 GLOBAL VARIABLES -- End: df
//*****************************************************************************


//****************************************************************************
//                      LOCAL FUNCTION PROTOTYPES
//****************************************************************************
static long WlanConnect();
static int set_time();
static void BoardInit(void);
static long InitializeAppVariables();
static int tls_connect();
static int connectToAccessPoint();
static int http_post(int);
char button_signal2;



/************************************************************************************/



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


int cx =  0;

int cy= 0;
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
char buffer[20] = "\0";
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
int buff_index = 0;
char* alpha;

char* buff;
char* buffold = "o";
char* buff2 = "s";

int flag_to_send = 0;
int top_x = 0;

int top_y = 0;

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
volatile static bRxDone;

int direction2=-1;
long lRetVal = -1;
int deleteflag=-1;



//*****************************************************************************
// SimpleLink Asynchronous Event Handlers -- Start
//*****************************************************************************


//*****************************************************************************
//
//! \brief The Function Handles WLAN Events
//!
//! \param[in]  pWlanEvent - Pointer to WLAN Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkWlanEventHandler(SlWlanEvent_t *pWlanEvent) {
    if(!pWlanEvent) {
        return;
    }

    switch(pWlanEvent->Event) {
        case SL_WLAN_CONNECT_EVENT: {
            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);

            //
            // Information about the connected AP (like name, MAC etc) will be
            // available in 'slWlanConnectAsyncResponse_t'.
            // Applications can use it if required
            //
            //  slWlanConnectAsyncResponse_t *pEventData = NULL;
            // pEventData = &pWlanEvent->EventData.STAandP2PModeWlanConnected;
            //

            // Copy new connection SSID and BSSID to global parameters
            memcpy(g_ucConnectionSSID,pWlanEvent->EventData.
                   STAandP2PModeWlanConnected.ssid_name,
                   pWlanEvent->EventData.STAandP2PModeWlanConnected.ssid_len);
            memcpy(g_ucConnectionBSSID,
                   pWlanEvent->EventData.STAandP2PModeWlanConnected.bssid,
                   SL_BSSID_LENGTH);

            UART_PRINT("[WLAN EVENT] STA Connected to the AP: %s , "
                       "BSSID: %x:%x:%x:%x:%x:%x\n\r",
                       g_ucConnectionSSID,g_ucConnectionBSSID[0],
                       g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                       g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                       g_ucConnectionBSSID[5]);
        }
        break;

        case SL_WLAN_DISCONNECT_EVENT: {
            slWlanConnectAsyncResponse_t*  pEventData = NULL;

            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_AQUIRED);

            pEventData = &pWlanEvent->EventData.STAandP2PModeDisconnected;

            // If the user has initiated 'Disconnect' request,
            //'reason_code' is SL_USER_INITIATED_DISCONNECTION
            if(SL_USER_INITIATED_DISCONNECTION == pEventData->reason_code) {
                UART_PRINT("[WLAN EVENT]Device disconnected from the AP: %s,"
                    "BSSID: %x:%x:%x:%x:%x:%x on application's request \n\r",
                           g_ucConnectionSSID,g_ucConnectionBSSID[0],
                           g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                           g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                           g_ucConnectionBSSID[5]);
            }
            else {
                UART_PRINT("[WLAN ERROR]Device disconnected from the AP AP: %s, "
                           "BSSID: %x:%x:%x:%x:%x:%x on an ERROR..!! \n\r",
                           g_ucConnectionSSID,g_ucConnectionBSSID[0],
                           g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                           g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                           g_ucConnectionBSSID[5]);
            }
            memset(g_ucConnectionSSID,0,sizeof(g_ucConnectionSSID));
            memset(g_ucConnectionBSSID,0,sizeof(g_ucConnectionBSSID));
        }
        break;

        default: {
            UART_PRINT("[WLAN EVENT] Unexpected event [0x%x]\n\r",
                       pWlanEvent->Event);
        }
        break;
    }
}

//*****************************************************************************
//
//! \brief This function handles network events such as IP acquisition, IP
//!           leased, IP released etc.
//!
//! \param[in]  pNetAppEvent - Pointer to NetApp Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent) {
    if(!pNetAppEvent) {
        return;
    }

    switch(pNetAppEvent->Event) {
        case SL_NETAPP_IPV4_IPACQUIRED_EVENT: {
            SlIpV4AcquiredAsync_t *pEventData = NULL;

            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_AQUIRED);

            //Ip Acquired Event Data
            pEventData = &pNetAppEvent->EventData.ipAcquiredV4;

            //Gateway IP address
            g_ulGatewayIP = pEventData->gateway;

            UART_PRINT("[NETAPP EVENT] IP Acquired: IP=%d.%d.%d.%d , "
                       "Gateway=%d.%d.%d.%d\n\r",
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,3),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,2),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,1),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,0),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,3),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,2),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,1),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,0));
        }
        break;

        default: {
            UART_PRINT("[NETAPP EVENT] Unexpected event [0x%x] \n\r",
                       pNetAppEvent->Event);
        }
        break;
    }
}


//*****************************************************************************
//
//! \brief This function handles HTTP server events
//!
//! \param[in]  pServerEvent - Contains the relevant event information
//! \param[in]    pServerResponse - Should be filled by the user with the
//!                                      relevant response information
//!
//! \return None
//!
//****************************************************************************
void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *pHttpEvent, SlHttpServerResponse_t *pHttpResponse) {
    // Unused in this application
}

//*****************************************************************************
//
//! \brief This function handles General Events
//!
//! \param[in]     pDevEvent - Pointer to General Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent) {
    if(!pDevEvent) {
        return;
    }

    //
    // Most of the general errors are not FATAL are are to be handled
    // appropriately by the application
    //
    UART_PRINT("[GENERAL EVENT] - ID=[%d] Sender=[%d]\n\n",
               pDevEvent->EventData.deviceEvent.status,
               pDevEvent->EventData.deviceEvent.sender);
}


//*****************************************************************************
//
//! This function handles socket events indication
//!
//! \param[in]      pSock - Pointer to Socket Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkSockEventHandler(SlSockEvent_t *pSock) {
    if(!pSock) {
        return;
    }

    switch( pSock->Event ) {
        case SL_SOCKET_TX_FAILED_EVENT:
            switch( pSock->socketAsyncEvent.SockTxFailData.status) {
                case SL_ECLOSE: 
                    UART_PRINT("[SOCK ERROR] - close socket (%d) operation "
                                "failed to transmit all queued packets\n\n", 
                                    pSock->socketAsyncEvent.SockTxFailData.sd);
                    break;
                default: 
                    UART_PRINT("[SOCK ERROR] - TX FAILED  :  socket %d , reason "
                                "(%d) \n\n",
                                pSock->socketAsyncEvent.SockTxFailData.sd, pSock->socketAsyncEvent.SockTxFailData.status);
                  break;
            }
            break;

        default:
            UART_PRINT("[SOCK EVENT] - Unexpected Event [%x0x]\n\n",pSock->Event);
          break;
    }
}


//*****************************************************************************
// SimpleLink Asynchronous Event Handlers -- End breadcrumb: s18_df
//*****************************************************************************


//*****************************************************************************
//
//! \brief This function initializes the application variables
//!
//! \param    0 on success else error code
//!
//! \return None
//!
//*****************************************************************************
static long InitializeAppVariables() {
    g_ulStatus = 0;
    g_ulGatewayIP = 0;
    g_Host = SERVER_NAME;
    memset(g_ucConnectionSSID,0,sizeof(g_ucConnectionSSID));
    memset(g_ucConnectionBSSID,0,sizeof(g_ucConnectionBSSID));
    return SUCCESS;
}


//*****************************************************************************
//! \brief This function puts the device in its default state. It:
//!           - Set the mode to STATION
//!           - Configures connection policy to Auto and AutoSmartConfig
//!           - Deletes all the stored profiles
//!           - Enables DHCP
//!           - Disables Scan policy
//!           - Sets Tx power to maximum
//!           - Sets power policy to normal
//!           - Unregister mDNS services
//!           - Remove all filters
//!
//! \param   none
//! \return  On success, zero is returned. On error, negative is returned
//*****************************************************************************
static long ConfigureSimpleLinkToDefaultState() {
    SlVersionFull   ver = {0};
    _WlanRxFilterOperationCommandBuff_t  RxFilterIdMask = {0};

    unsigned char ucVal = 1;
    unsigned char ucConfigOpt = 0;
    unsigned char ucConfigLen = 0;
    unsigned char ucPower = 0;

    long lRetVal = -1;
    long lMode = -1;

    lMode = sl_Start(0, 0, 0);
    ASSERT_ON_ERROR(lMode);

    // If the device is not in station-mode, try configuring it in station-mode 
    if (ROLE_STA != lMode) {
        if (ROLE_AP == lMode) {
            // If the device is in AP mode, we need to wait for this event 
            // before doing anything 
            while(!IS_IP_ACQUIRED(g_ulStatus)) {
#ifndef SL_PLATFORM_MULTI_THREADED
              _SlNonOsMainLoopTask(); 
#endif
            }
        }

        // Switch to STA role and restart 
        lRetVal = sl_WlanSetMode(ROLE_STA);
        ASSERT_ON_ERROR(lRetVal);

        lRetVal = sl_Stop(0xFF);
        ASSERT_ON_ERROR(lRetVal);

        lRetVal = sl_Start(0, 0, 0);
        ASSERT_ON_ERROR(lRetVal);

        // Check if the device is in station again 
        if (ROLE_STA != lRetVal) {
            // We don't want to proceed if the device is not coming up in STA-mode 
            return DEVICE_NOT_IN_STATION_MODE;
        }
    }
    
    // Get the device's version-information
    ucConfigOpt = SL_DEVICE_GENERAL_VERSION;
    ucConfigLen = sizeof(ver);
    lRetVal = sl_DevGet(SL_DEVICE_GENERAL_CONFIGURATION, &ucConfigOpt, 
                                &ucConfigLen, (unsigned char *)(&ver));
    ASSERT_ON_ERROR(lRetVal);
    
    UART_PRINT("Host Driver Version: %s\n\r",SL_DRIVER_VERSION);
    UART_PRINT("Build Version %d.%d.%d.%d.31.%d.%d.%d.%d.%d.%d.%d.%d\n\r",
    ver.NwpVersion[0],ver.NwpVersion[1],ver.NwpVersion[2],ver.NwpVersion[3],
    ver.ChipFwAndPhyVersion.FwVersion[0],ver.ChipFwAndPhyVersion.FwVersion[1],
    ver.ChipFwAndPhyVersion.FwVersion[2],ver.ChipFwAndPhyVersion.FwVersion[3],
    ver.ChipFwAndPhyVersion.PhyVersion[0],ver.ChipFwAndPhyVersion.PhyVersion[1],
    ver.ChipFwAndPhyVersion.PhyVersion[2],ver.ChipFwAndPhyVersion.PhyVersion[3]);

    // Set connection policy to Auto + SmartConfig 
    //      (Device's default connection policy)
    lRetVal = sl_WlanPolicySet(SL_POLICY_CONNECTION, 
                                SL_CONNECTION_POLICY(1, 0, 0, 0, 1), NULL, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Remove all profiles
    lRetVal = sl_WlanProfileDel(0xFF);
    ASSERT_ON_ERROR(lRetVal);

    

    //
    // Device in station-mode. Disconnect previous connection if any
    // The function returns 0 if 'Disconnected done', negative number if already
    // disconnected Wait for 'disconnection' event if 0 is returned, Ignore 
    // other return-codes
    //
    lRetVal = sl_WlanDisconnect();
    if(0 == lRetVal) {
        // Wait
        while(IS_CONNECTED(g_ulStatus)) {
#ifndef SL_PLATFORM_MULTI_THREADED
              _SlNonOsMainLoopTask(); 
#endif
        }
    }

    // Enable DHCP client
    lRetVal = sl_NetCfgSet(SL_IPV4_STA_P2P_CL_DHCP_ENABLE,1,1,&ucVal);
    ASSERT_ON_ERROR(lRetVal);

    // Disable scan
    ucConfigOpt = SL_SCAN_POLICY(0);
    lRetVal = sl_WlanPolicySet(SL_POLICY_SCAN , ucConfigOpt, NULL, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Set Tx power level for station mode
    // Number between 0-15, as dB offset from max power - 0 will set max power
    ucPower = 0;
    lRetVal = sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID, 
            WLAN_GENERAL_PARAM_OPT_STA_TX_POWER, 1, (unsigned char *)&ucPower);
    ASSERT_ON_ERROR(lRetVal);

    // Set PM policy to normal
    lRetVal = sl_WlanPolicySet(SL_POLICY_PM , SL_NORMAL_POLICY, NULL, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Unregister mDNS services
    lRetVal = sl_NetAppMDNSUnRegisterService(0, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Remove  all 64 filters (8*8)
    memset(RxFilterIdMask.FilterIdMask, 0xFF, 8);
    lRetVal = sl_WlanRxFilterSet(SL_REMOVE_RX_FILTER, (_u8 *)&RxFilterIdMask,
                       sizeof(_WlanRxFilterOperationCommandBuff_t));
    ASSERT_ON_ERROR(lRetVal);

    lRetVal = sl_Stop(SL_STOP_TIMEOUT);
    ASSERT_ON_ERROR(lRetVal);

    InitializeAppVariables();
    
    return lRetVal; // Success
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
static void BoardInit(void) {
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


//****************************************************************************
//
//! \brief Connecting to a WLAN Accesspoint
//!
//!  This function connects to the required AP (SSID_NAME) with Security
//!  parameters specified in te form of macros at the top of this file
//!
//! \param  None
//!
//! \return  0 on success else error code
//!
//! \warning    If the WLAN connection fails or we don't aquire an IP
//!            address, It will be stuck in this function forever.
//
//****************************************************************************
static long WlanConnect() {
    SlSecParams_t secParams = {0};
    long lRetVal = 0;

    secParams.Key = SECURITY_KEY;
    secParams.KeyLen = strlen(SECURITY_KEY);
    secParams.Type = SECURITY_TYPE;

    UART_PRINT("Attempting connection to access point: ");
    UART_PRINT(SSID_NAME);
    UART_PRINT("... ...");
    lRetVal = sl_WlanConnect(SSID_NAME, strlen(SSID_NAME), 0, &secParams, 0);
    ASSERT_ON_ERROR(lRetVal);

    UART_PRINT(" Connected!!!\n\r");


    // Wait for WLAN Event
    while((!IS_CONNECTED(g_ulStatus)) || (!IS_IP_ACQUIRED(g_ulStatus))) {
        // Toggle LEDs to Indicate Connection Progress
        _SlNonOsMainLoopTask();
        GPIO_IF_LedOff(MCU_IP_ALLOC_IND);
        MAP_UtilsDelay(800000);
        _SlNonOsMainLoopTask();
        GPIO_IF_LedOn(MCU_IP_ALLOC_IND);
        MAP_UtilsDelay(800000);
    }

    return SUCCESS;

}




long printErrConvenience(char * msg, long retVal) {
    UART_PRINT(msg);
    GPIO_IF_LedOn(MCU_RED_LED_GPIO);
    return retVal;
}


//*****************************************************************************
//
//! This function updates the date and time of CC3200.
//!
//! \param None
//!
//! \return
//!     0 for success, negative otherwise
//!
//*****************************************************************************

static int set_time() {
    long retVal;

    g_time.tm_day = DATE;
    g_time.tm_mon = MONTH;
    g_time.tm_year = YEAR;
    g_time.tm_sec = HOUR;
    g_time.tm_hour = MINUTE;
    g_time.tm_min = SECOND;

    retVal = sl_DevSet(SL_DEVICE_GENERAL_CONFIGURATION,
                          SL_DEVICE_GENERAL_CONFIGURATION_DATE_TIME,
                          sizeof(SlDateTime),(unsigned char *)(&g_time));

    ASSERT_ON_ERROR(retVal);
    return SUCCESS;
}

//*****************************************************************************
//
//! This function demonstrates how certificate can be used with SSL.
//! The procedure includes the following steps:
//! 1) connect to an open AP
//! 2) get the server name via a DNS request
//! 3) define all socket options and point to the CA certificate
//! 4) connect to the server via TCP
//!
//! \param None
//!
//! \return  0 on success else error code
//! \return  LED1 is turned solid in case of success
//!    LED2 is turned solid in case of failure
//!
//*****************************************************************************
static int tls_connect() {
    SlSockAddrIn_t    Addr;
    int    iAddrSize;
    unsigned char    ucMethod = SL_SO_SEC_METHOD_TLSV1_2;
    unsigned int uiIP;
//    unsigned int uiCipher = SL_SEC_MASK_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA;
    unsigned int uiCipher = SL_SEC_MASK_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256;
// SL_SEC_MASK_SSL_RSA_WITH_RC4_128_SHA
// SL_SEC_MASK_SSL_RSA_WITH_RC4_128_MD5
// SL_SEC_MASK_TLS_RSA_WITH_AES_256_CBC_SHA
// SL_SEC_MASK_TLS_DHE_RSA_WITH_AES_256_CBC_SHA
// SL_SEC_MASK_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA
// SL_SEC_MASK_TLS_ECDHE_RSA_WITH_RC4_128_SHA
// SL_SEC_MASK_TLS_RSA_WITH_AES_128_CBC_SHA256
// SL_SEC_MASK_TLS_RSA_WITH_AES_256_CBC_SHA256
// SL_SEC_MASK_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256
// SL_SEC_MASK_TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256 // does not work (-340, handshake fails)
    long lRetVal = -1;
    int iSockID;

    lRetVal = sl_NetAppDnsGetHostByName(g_Host, strlen((const char *)g_Host),
                                    (unsigned long*)&uiIP, SL_AF_INET);

    if(lRetVal < 0) {
        return printErrConvenience("Device couldn't retrieve the host name \n\r", lRetVal);
    }

    Addr.sin_family = SL_AF_INET;
    Addr.sin_port = sl_Htons(GOOGLE_DST_PORT);
    Addr.sin_addr.s_addr = sl_Htonl(uiIP);
    iAddrSize = sizeof(SlSockAddrIn_t);
    //
    // opens a secure socket 
    //
    iSockID = sl_Socket(SL_AF_INET,SL_SOCK_STREAM, SL_SEC_SOCKET);
    if( iSockID < 0 ) {
        return printErrConvenience("Device unable to create secure socket \n\r", lRetVal);
    }

    //
    // configure the socket as TLS1.2
    //
    lRetVal = sl_SetSockOpt(iSockID, SL_SOL_SOCKET, SL_SO_SECMETHOD, &ucMethod,\
                               sizeof(ucMethod));
    if(lRetVal < 0) {
        return printErrConvenience("Device couldn't set socket options \n\r", lRetVal);
    }
    //
    //configure the socket as ECDHE RSA WITH AES256 CBC SHA
    //
    lRetVal = sl_SetSockOpt(iSockID, SL_SOL_SOCKET, SL_SO_SECURE_MASK, &uiCipher,\
                           sizeof(uiCipher));
    if(lRetVal < 0) {
        return printErrConvenience("Device couldn't set socket options \n\r", lRetVal);
    }



/////////////////////////////////
// START: COMMENT THIS OUT IF DISABLING SERVER VERIFICATION
    //
    //configure the socket with CA certificate - for server verification
    //
    lRetVal = sl_SetSockOpt(iSockID, SL_SOL_SOCKET, \
                           SL_SO_SECURE_FILES_CA_FILE_NAME, \
                           SL_SSL_CA_CERT, \
                           strlen(SL_SSL_CA_CERT));

    if(lRetVal < 0) {
        return printErrConvenience("Device couldn't set socket options \n\r", lRetVal);
    }
// END: COMMENT THIS OUT IF DISABLING SERVER VERIFICATION
/////////////////////////////////


    //configure the socket with Client Certificate - for server verification
    //
    lRetVal = sl_SetSockOpt(iSockID, SL_SOL_SOCKET, \
                SL_SO_SECURE_FILES_CERTIFICATE_FILE_NAME, \
                                    SL_SSL_CLIENT, \
                           strlen(SL_SSL_CLIENT));

    if(lRetVal < 0) {
        return printErrConvenience("Device couldn't set socket options \n\r", lRetVal);
    }

    //configure the socket with Private Key - for server verification
    //
    lRetVal = sl_SetSockOpt(iSockID, SL_SOL_SOCKET, \
            SL_SO_SECURE_FILES_PRIVATE_KEY_FILE_NAME, \
            SL_SSL_PRIVATE, \
                           strlen(SL_SSL_PRIVATE));

    if(lRetVal < 0) {
        return printErrConvenience("Device couldn't set socket options \n\r", lRetVal);
    }


    /* connect to the peer device - Google server */
    lRetVal = sl_Connect(iSockID, ( SlSockAddr_t *)&Addr, iAddrSize);

    if(lRetVal >= 0) {
        UART_PRINT("Device has connected to the website:");
        UART_PRINT(SERVER_NAME);
        UART_PRINT("\n\r");
    }
    else if(lRetVal == SL_ESECSNOVERIFY) {
        UART_PRINT("Device has connected to the website (UNVERIFIED):");
        UART_PRINT(SERVER_NAME);
        UART_PRINT("\n\r");
    }
    else if(lRetVal < 0) {
        UART_PRINT("Device couldn't connect to server:");
        UART_PRINT(SERVER_NAME);
        UART_PRINT("\n\r");
        return printErrConvenience("Device couldn't connect to server \n\r", lRetVal);
    }

    GPIO_IF_LedOff(MCU_RED_LED_GPIO);
    GPIO_IF_LedOn(MCU_GREEN_LED_GPIO);
    return iSockID;
}



int connectToAccessPoint() {
    long lRetVal = -1;
    GPIO_IF_LedConfigure(LED1|LED3);

    GPIO_IF_LedOff(MCU_RED_LED_GPIO);
    GPIO_IF_LedOff(MCU_GREEN_LED_GPIO);

    lRetVal = InitializeAppVariables();
    ASSERT_ON_ERROR(lRetVal);

    //
    // Following function configure the device to default state by cleaning
    // the persistent settings stored in NVMEM (viz. connection profiles &
    // policies, power policy etc)
    //
    // Applications may choose to skip this step if the developer is sure
    // that the device is in its default state at start of applicaton
    //
    // Note that all profiles and persistent settings that were done on the
    // device will be lost
    //
    lRetVal = ConfigureSimpleLinkToDefaultState();
    if(lRetVal < 0) {
      if (DEVICE_NOT_IN_STATION_MODE == lRetVal)
          UART_PRINT("Failed to configure the device in its default state \n\r");

      return lRetVal;
    }

    UART_PRINT("Device is configured in default state \n\r");

    CLR_STATUS_BIT_ALL(g_ulStatus);

    ///
    // Assumption is that the device is configured in station mode already
    // and it is in its default state
    //
    UART_PRINT("Opening sl_start\n\r");
    lRetVal = sl_Start(0, 0, 0);
    if (lRetVal < 0 || ROLE_STA != lRetVal) {
        UART_PRINT("Failed to start the device \n\r");
        return lRetVal;
    }

    UART_PRINT("Device started as STATION \n\r");

    //
    //Connecting to WLAN AP
    //
    lRetVal = WlanConnect();
    if(lRetVal < 0) {
        UART_PRINT("Failed to establish connection w/ an AP \n\r");
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }

    UART_PRINT("Connection established w/ AP and IP is aquired \n\r");
    return 0;
}

//*****************************************************************************
//
//! Main 
//!
//! \param  none
//!
//! \return None
//!
//*****************************************************************************
    //
    // Initialize board configuration
    //


  int main(void){
    BoardInit();

    PinMuxConfig();

    InitTerm();
    ClearTerm();
    UART_PRINT("My terminal works!\n\r");

    //Connect the CC3200 to the local access point
    lRetVal = connectToAccessPoint();
    //Set time so that encryption can be used
    lRetVal = set_time();
    if(lRetVal < 0) {
        UART_PRINT("Unable to set time in the device");
        LOOP_FOREVER();
    }
    //Connect to the website with TLS encryption
    lRetVal = tls_connect();
    if(lRetVal < 0) {
        ERR_PRINT(lRetVal);
    }

    main2();




    LOOP_FOREVER();
}
//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

static int http_post(int iTLSSockID){
    char acSendBuff[512];
    char acRecvbuff[1460];
    char cCLLength[200];
    char* pcBufHeaders;
    int lRetVal = 0;
    pcBufHeaders = acSendBuff;
    strcpy(pcBufHeaders, POSTHEADER);
    pcBufHeaders += strlen(POSTHEADER);
    strcpy(pcBufHeaders, HOSTHEADER);
    pcBufHeaders += strlen(HOSTHEADER);
    strcpy(pcBufHeaders, CHEADER);
    pcBufHeaders += strlen(CHEADER);
    strcpy(pcBufHeaders, "\r\n\r\n");

    char* front = "{\"state\": {\r\n\"desired\" : {\r\n\"var\" : \"";
    char buffer[10] = "dndjbhj";

    //char* buff = "hitesting";
    char* middle = buff;
    printf("buff %s<---\n", buff);
    char* back = "\"\r\n}}}\r\n\r\n";

    char* text_temp = strcat(front,middle);
    char* text = strcat(text_temp, back);
    //int dataLength = strlen(DATA1);
    //int dataLength = strlen("{\"state\": {\r\n\"desired\" : {\r\n\"var\" : \"AAAA\"\r\n}}}\r\n\r\n");
    int dataLength = strlen(text);




    strcpy(pcBufHeaders, CTHEADER);
    pcBufHeaders += strlen(CTHEADER);
    strcpy(pcBufHeaders, CLHEADER1);

    pcBufHeaders += strlen(CLHEADER1);

    sprintf(cCLLength, "%d", dataLength);

    strcpy(pcBufHeaders, cCLLength);
    pcBufHeaders += strlen(cCLLength);
    strcpy(pcBufHeaders, CLHEADER2);
    pcBufHeaders += strlen(CLHEADER2);

    //strcpy(pcBufHeaders, DATA1);
    //pcBufHeaders += strlen(DATA1);

    //strcpy(pcBufHeaders, text);


    //textcat = strcat("xxx", "w");
    strcpy(pcBufHeaders, text);
    //strcpy(pcBufHeaders, "{\"state\": {\r\n\"desired\" : {\r\n\"var\" : \"A\"\r\n}}}\r\n\r\n");

  //  strcpy(pcBufHeaders, textcat);


    //pcBufHeaders += strlen(text);
    pcBufHeaders += strlen(text);
    //strcpy(pcBufHeaders, "{\"state\": {\r\n\"desired\" : {\r\n\"var\" : \"A\"\r\n}}}\r\n\r\n");

printf("Text: %s", text);
    int testDataLength = strlen(pcBufHeaders);

    UART_PRINT(acSendBuff);


    //
    // Send the packet to the server */
    //
    lRetVal = sl_Send(iTLSSockID, acSendBuff, strlen(acSendBuff), 0);
    if(lRetVal < 0) {
        UART_PRINT("POST failed. Error Number: %i\n\r",lRetVal);
        sl_Close(iTLSSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }
    lRetVal = sl_Recv(iTLSSockID, &acRecvbuff[0], sizeof(acRecvbuff), 0);
    if(lRetVal < 0) {
        UART_PRINT("Received failed. Error Number: %i\n\r",lRetVal);
        //sl_Close(iSSLSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
           return lRetVal;
    }
    else {
        acRecvbuff[lRetVal+1] = '\0';
        UART_PRINT(acRecvbuff);
        UART_PRINT("\n\r\n\r");
    }

    return 0;
}
/*********************************************************************************/
int
main2(void)
  {
    //
    // Initialize board configurations
    //BoardInit();
    //
    // Pinmuxing for LEDs
    //
    //PinMuxConfig();
  //  InitTerm();
   // ClearTerm();
    bRxDone=false;


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



    //UART
    MAP_UARTConfigSetExpClk(UARTA1_BASE,MAP_PRCMPeripheralClockGet(PRCM_UARTA1),
                               UART_BAUD_RATE,
                               (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                               UART_CONFIG_PAR_NONE));
       MAP_UARTIntRegister(UARTA1_BASE,UARTIntHandler);
      // MAP_UARTIntClear(TIMERA1_BASE);
       MAP_UARTIntEnable(UARTA1_BASE,UART_INT_RX|UART_INT_RT);

      MAP_UARTEnable(UARTA1_BASE);
      MAP_UARTIntEnable(UARTA1_BASE,UART_INT_DMARX);
      MAP_UARTDMAEnable(UARTA1_BASE,UART_DMA_RX);






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
    MAP_TimerLoadSet(g_ulRefBase, TIMER_A, MILLISECONDS_TO_TICKS(4000));
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
                printf("alpha recieved from btc  : %c\n", alpha_pressed);
                int indexi2 = indexi;
                int indexj2 = indexj;



                alpha = buttonToChar2(button_signal, counting[indexi2][indexj2]);
               // printf("Button sig: %c, count:%d \n",button_signal, counting[indexi][indexj]);
               //printf("alpha recieved from btc2: %s\n", alpha);

                buff = "X";
                buff += 1;
                buffold+=1;
                buff2+=1;


                if(diff)
                {

                    buff_index++;
                    diff=0;
                    //printf("Alpha: %c Count: %d\n", alpha_pressed,counting[indexi][indexj] );
                    //fillScreen(BLUE);
                    printChar(alpha_pressed, direction);
                    buffer[buff_index] = alpha_pressed;
                    int ind = 0;


                   if(direction == 1)
                    {
                    strcat(buff, alpha);
                    //buff += strlen(alpha);
                    }

                   if(direction ==0)
                   {
                      buff[strlen(buff)-1]=alpha_pressed;

                   }
                    // strcat(buff, alpha);
                    //buff += strlen(alpha);



                       //printf("buff one is: %s\n", buff);
                  //  }
                        printf("Alpha after concat w buff %s\n", alpha);
                        printf("Buff after concatenation w alpha: %s\n", buff);
                        //strcat(buff, alpha);

                    //printf("My buff is %s\n", buff);

                    //WHERE TO PRINT CHAR;
                }
                MAP_UtilsDelay(400000);


                if(indexi==3 && indexj ==0) //MUTE OR STAR
                {
                    deleteflag=1;
                    direction=-1;
                }

                if(indexi==3 && indexj ==2) //#
                {
                    http_post(lRetVal);
                    sl_Stop(SL_STOP_TIMEOUT);
                    /*for (i = 0; i <= buff_index; i++) {
                            //buffer[i] = ;
                            //UARTCharPut(UARTA1_BASE, buffer[i]);
                            //printf("I sent this %c\n", buffer[i]);

                    }*/

                    printf("#\n");
                    setCursor(0,0);
                    //fillScreen(RED);
                    buff_index = 0;
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
//! The interrupt handler for the first timer interrupt.
//!
//! \param  None
//!
//! \return none
//
//*****************************************************************************

static void UARTIntHandler()
{
    printf("entered uart\n");


    if(!bRxDone) {
        MAP_UARTDMADisable(UARTA0_BASE,UART_DMA_RX);
        bRxDone = true;
    }
    else {
        MAP_UARTDMADisable(UARTA0_BASE,UART_DMA_TX);
    }
    MAP_UARTIntClear(UARTA1_BASE,UART_INT_DMATX|UART_INT_DMARX);
    // MAP_UARTDMAEnable(UARTA1_BASE,UART_DMA_TX);
    printf("Trying to get data\n");
    char tmp = UARTCharGet(UARTA1_BASE);
    printf("Data Recieved: %c\n",tmp);
    setCursor(100, 100);
    printChar(tmp, 1);
}
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
//char button_signal2;
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
                direction = 1;
                direction2=0;
                if(deleteflag==1)
                            {
                                deleteflag=0;
                                direction=-1;
                            }
            }
            else if (signalFlag == button_signal2)
            {
                MAP_TimerDisable(g_ulBase, TIMER_A);
                //MAP_UtilsDelay(MILLi);
                MAP_UtilsDelay(10000000);
                MAP_TimerEnable(g_ulBase, TIMER_A);
                //button_signal='\0';
                diff=1;
                direction = 0;
                if(direction2 == 1)
                {
                    direction=1;
                    direction2=0;
                }
                if(deleteflag==1)
                {
                    deleteflag=0;
                    direction=-1;
                }




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




char* buttonToChar2(char button, int count)
{
    short tmp;
    char result;
    char* res;
    switch(button) {
    case '2':
        tmp = count % 3;
        //res = "2";
       if(tmp ==1){
           res = "a";
       }
       if(tmp == 2){
           res = "b";
       }
       if(tmp ==3){
           res = "c";
       }
        break;
    case '3':
       // res = "3";
        tmp = count % 3;
        if(tmp ==1){
                   res = "d";
               }
               if(tmp == 2){
                   res = "e";
               }
               if(tmp ==3){
                   res = "f";
               }
        break;
    case '4':
       // res = "4";
        tmp = count % 3;
        if(tmp ==1){
                   res = "g";
               }
               if(tmp == 2){
                   res = "h";
               }
               if(tmp ==3){
                   res = "i";
               }
        break;
    case '5':
       // res = "5";
        tmp = count % 3;
        if(tmp ==1){
                   res = "j";
               }
               if(tmp == 2){
                   res = "k";
               }
               if(tmp ==3){
                   res = "l";
               }
        break;
    case '6':
       // res = "6";
        tmp = count % 3;
        if(tmp ==1){
                   res = "m";
               }
               if(tmp == 2){
                   res = "n";
               }
               if(tmp ==3){
                   res = "o";
               }
        break;
    case '7':

      //  res = "7";
        tmp = count % 4;
        if(tmp ==1){
                   res = "p";
               }
               if(tmp == 2){
                   res = "q";
               }
               if(tmp ==3){
                   res = "r";
               }
               if(tmp ==4){
                                  res = "s";
                              }
        break;
    case '8':
       // res = "8";
        tmp = count % 3;
        if(tmp ==1){
                   res = "t";
               }
               if(tmp == 2){
                   res = "u";
               }
               if(tmp ==3){
                   res = "v";
               }
        break;
    case '9':
       // res = "9";
        tmp = count % 4;
        if(tmp ==1){
                   res = "w";
               }
               if(tmp == 2){
                   res = "x";
               }
               if(tmp ==3){
                   res = "y";
               }
               if(tmp ==4){
                   res = "z";
                       }
        break;
    case '0':
        res = " ";
        result = ' ';
        break;

    default: result = '\0'; break;
    }
    return res;
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
    direction2=1;
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


void printChar(char ch, short direction)
{
    int textsize = 1;

    if (direction < 1) {
        fillRect(cx, cy, 6, 8, BLUE);
        if (direction < 0) {
            buffer[strlen(buffer)-1] = '\0';
        }
    }
    cx += 6 * direction;
    if (cx < 0) {
        if (cy == 0) {
            cx = 0;
        } else {
            cx = 121;
            cy -= 8;
        }
    }
    drawChar(cx, cy, ch, 0xFFFF, 0xFFFF, textsize);
    if (cx > 124) {
        cx = 0;
        cy += 8;
    }
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

