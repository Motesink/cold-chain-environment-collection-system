/**************************************************************************************************
  Filename:       SampleApp.c
  Revised:        $Date: 2009-03-18 15:56:27 -0700 (Wed, 18 Mar 2009) $
  Revision:       $Revision: 19453 $

  Description:    Sample Application (no Profile).


  Copyright 2007 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED 揂S IS?WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
**************************************************************************************************/

/*********************************************************************
  This application isn't intended to do anything useful, it is
  intended to be a simple example of an application's structure.

  This application sends it's messages either as broadcast or
  broadcast filtered group messages.  The other (more normal)
  message addressing is unicast.  Most of the other sample
  applications are written to support the unicast message model.

  Key control:
    SW1:  Sends a flash command to all devices in Group 1.
    SW2:  Adds/Removes (toggles) this device in and out
          of Group 1.  This will enable and disable the
          reception of the flash command.
*********************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "OSAL.h"
#include "ZGlobals.h"

#include "OSAL_Clock.h"       // 添加时钟模块头文件
extern uint32 osal_getClock(void); // 声明系统时钟获取函数

#include "AF.h"
#include "aps_groups.h"
#include "ZDApp.h"

#include "SampleApp.h"
#include "SampleAppHw.h"

#include "OnBoard.h"

/* HAL */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"
#include "MT_UART.h"
#include "MT_APP.h"
#include "MT.h"
#include "DHT11.h"
#include "ds18b20.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <stdarg.h>
/*********************************************************************
 * MACROS
 */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr)[0])

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

uint8 AppTitle[] = "ALD2530 DS18B20"; //应用程序名称 

// This list should be filled with Application specific Cluster IDs.
const cId_t SampleApp_ClusterList[SAMPLEAPP_MAX_CLUSTERS] =
{
  SAMPLEAPP_PERIODIC_CLUSTERID,
  SAMPLEAPP_FLASH_CLUSTERID,
  SAMPLEAPP_P2P_CLUSTERID
};

const SimpleDescriptionFormat_t SampleApp_SimpleDesc =
{
  SAMPLEAPP_ENDPOINT,              //  int Endpoint;
  SAMPLEAPP_PROFID,                //  uint16 AppProfId[2];
  SAMPLEAPP_DEVICEID,              //  uint16 AppDeviceId[2];
  SAMPLEAPP_DEVICE_VERSION,        //  int   AppDevVer:4;
  SAMPLEAPP_FLAGS,                 //  int   AppFlags:4;
  SAMPLEAPP_MAX_CLUSTERS,          //  uint8  AppNumInClusters;
  (cId_t *)SampleApp_ClusterList,  //  uint8 *pAppInClusterList;
  SAMPLEAPP_MAX_CLUSTERS,          //  uint8  AppNumInClusters;
  (cId_t *)SampleApp_ClusterList   //  uint8 *pAppInClusterList;
};

// This is the Endpoint/Interface description.  It is defined here, but
// filled-in in SampleApp_Init().  Another way to go would be to fill
// in the structure here and make it a "const" (in code space).  The
// way it's defined in this sample app it is define in RAM.
endPointDesc_t SampleApp_epDesc;

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
uint8 SampleApp_TaskID;   // Task ID for internal task/event processing
                          // This variable will be received when
                          // SampleApp_Init() is called.
devStates_t SampleApp_NwkState;

uint8 SampleApp_TransID;  // This is the unique message ID (counter)

afAddrType_t SampleApp_Periodic_DstAddr; //广播
afAddrType_t SampleApp_Flash_DstAddr;    //组播
afAddrType_t SampleApp_P2P_DstAddr;      //点播

aps_Group_t SampleApp_Group;

uint8 SampleAppPeriodicCounter = 0;
uint8 SampleAppFlashCounter = 0;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
void SampleApp_HandleKeys( uint8 shift, uint8 keys );
void SampleApp_MessageMSGCB( afIncomingMSGPacket_t *pckt );
void SampleApp_SendPeriodicMessage( void );
void SampleApp_SendFlashMessage( uint16 flashTime );
void SampleApp_Send_P2P_Message(void);
/*********************************************************************
 * NETWORK LAYER CALLBACKS
 */

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      SampleApp_Init
 *
 * @brief   Initialization function for the Generic App Task.
 *          This is called during initialization and should contain
 *          any application specific initialization (ie. hardware
 *          initialization/setup, table initialization, power up
 *          notificaiton ... ).
 *
 * @param   task_id - the ID assigned by OSAL.  This ID should be
 *                    used to send messages and set timers.
 *
 * @return  none
 */

static void SampleApp_UartCallback(uint8 port, uint8 event) {
  if (event == HAL_UART_RX_FULL) {
    uint8 buf[128];
    uint16 len = HalUARTRead(0, buf, 128);
    // 处理HC-25返回的数据（如MQTT连接状态）
  }
}

void SampleApp_Init( uint8 task_id )
{ 
  SampleApp_TaskID = task_id;
  SampleApp_NwkState = DEV_INIT;
  SampleApp_TransID = 0;
  
  MT_UartInit();                  //串口初始化
  MT_UartRegisterTaskID(task_id); //注册串口任务
  P0SEL &= 0x7f;                  //DHT11的io口初始化 p0.7
  P0SEL &= 0xFD;                  //DS18B20的io口初始化 p0.1
  
  
  // 串口初始化
  halUARTCfg_t uartConfig;
  uartConfig.baudRate = HAL_UART_BR_115200; // 波特率115200
  uartConfig.flowControl = FALSE;
  uartConfig.callBackFunc = SampleApp_UartCallback; // 回调函数
  HalUARTOpen(0, &uartConfig); // 打开串口0
  
  
  // Device hardware initialization can be added here or in main() (Zmain.c).
  // If the hardware is application specific - add it here.
  // If the hardware is other parts of the device add it in main().

 #if defined ( BUILD_ALL_DEVICES )
  // The "Demo" target is setup to have BUILD_ALL_DEVICES and HOLD_AUTO_START
  // We are looking at a jumper (defined in SampleAppHw.c) to be jumpered
  // together - if they are - we will start up a coordinator. Otherwise,
  // the device will start as a router.
  if ( readCoordinatorJumper() )
    zgDeviceLogicalType = ZG_DEVICETYPE_COORDINATOR;
  else
    zgDeviceLogicalType = ZG_DEVICETYPE_ROUTER;
#endif // BUILD_ALL_DEVICES

#if defined ( HOLD_AUTO_START )
  // HOLD_AUTO_START is a compile option that will surpress ZDApp
  //  from starting the device and wait for the application to
  //  start the device.
  ZDOInitDevice(0);
#endif

  // Setup for the periodic message's destination address
  // Broadcast to everyone
  SampleApp_Periodic_DstAddr.addrMode = (afAddrMode_t)AddrBroadcast;
  SampleApp_Periodic_DstAddr.endPoint = SAMPLEAPP_ENDPOINT;
  SampleApp_Periodic_DstAddr.addr.shortAddr = 0xFFFF;

  // Setup for the flash command's destination address - Group 1
  SampleApp_Flash_DstAddr.addrMode = (afAddrMode_t)afAddrGroup;
  SampleApp_Flash_DstAddr.endPoint = SAMPLEAPP_ENDPOINT;
  SampleApp_Flash_DstAddr.addr.shortAddr = SAMPLEAPP_FLASH_GROUP;
  
  SampleApp_P2P_DstAddr.addrMode = (afAddrMode_t)Addr16Bit; //点播 
  SampleApp_P2P_DstAddr.endPoint = SAMPLEAPP_ENDPOINT; 
  SampleApp_P2P_DstAddr.addr.shortAddr = 0x0000;            //发给协调器

  // Fill out the endpoint description.
  SampleApp_epDesc.endPoint = SAMPLEAPP_ENDPOINT;
  SampleApp_epDesc.task_id = &SampleApp_TaskID;
  SampleApp_epDesc.simpleDesc
            = (SimpleDescriptionFormat_t *)&SampleApp_SimpleDesc;
  SampleApp_epDesc.latencyReq = noLatencyReqs;

  // Register the endpoint description with the AF
  afRegister( &SampleApp_epDesc );

  // Register for all key events - This app will handle all key events
  RegisterForKeys( SampleApp_TaskID );

  // By default, all devices start out in Group 1
  SampleApp_Group.ID = 0x0001;
  osal_memcpy( SampleApp_Group.name, "Group 1", 7 );
  aps_AddGroup( SAMPLEAPP_ENDPOINT, &SampleApp_Group );

#if defined ( LCD_SUPPORTED )
  HalLcdWriteString( "SampleApp", HAL_LCD_LINE_1 );
#endif
}

/*********************************************************************
 * @fn      SampleApp_ProcessEvent
 *
 * @brief   Generic Application Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id  - The OSAL assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  none
 */
uint16 SampleApp_ProcessEvent( uint8 task_id, uint16 events )
{
  afIncomingMSGPacket_t *MSGpkt;
  (void)task_id;  // Intentionally unreferenced parameter

  if ( events & SYS_EVENT_MSG )
  {
    MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( SampleApp_TaskID );
    while ( MSGpkt )
    {
      switch ( MSGpkt->hdr.event )
      {
        // Received when a key is pressed
        case KEY_CHANGE:
          SampleApp_HandleKeys( ((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys );
          break;

        // Received when a messages is received (OTA) for this endpoint
        case AF_INCOMING_MSG_CMD:
          SampleApp_MessageMSGCB( MSGpkt );
          break;

        // Received whenever the device changes state in the network
        case ZDO_STATE_CHANGE:
          SampleApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
          if ( //(SampleApp_NwkState == DEV_ZB_COORD) ||
                 (SampleApp_NwkState == DEV_ROUTER)
              || (SampleApp_NwkState == DEV_END_DEVICE) )
          {
            // Start sending the periodic message in a regular interval.
            osal_start_timerEx( SampleApp_TaskID,
                              SAMPLEAPP_SEND_PERIODIC_MSG_EVT,
                              SAMPLEAPP_SEND_PERIODIC_MSG_TIMEOUT );
          }
          else
          {
            // Device is no longer in the network
          }
          break;

        default:
          break;
      }

      // Release the memory
      osal_msg_deallocate( (uint8 *)MSGpkt );

      // Next - if one is available
      MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( SampleApp_TaskID );
    }

    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }

  // Send a message out - This event is generated by a timer
  //  (setup in SampleApp_Init()).
  if ( events & SAMPLEAPP_SEND_PERIODIC_MSG_EVT )
  {
    // Send the periodic message
    //SampleApp_SendPeriodicMessage();
    SampleApp_Send_P2P_Message();

    // Setup to send message again in normal period (+ a little jitter)
    osal_start_timerEx( SampleApp_TaskID, SAMPLEAPP_SEND_PERIODIC_MSG_EVT,
        (SAMPLEAPP_SEND_PERIODIC_MSG_TIMEOUT + (osal_rand() & 0x00FF)) );

    // return unprocessed events
    return (events ^ SAMPLEAPP_SEND_PERIODIC_MSG_EVT);
  }

  // Discard unknown events
  return 0;
}

/*********************************************************************
 * Event Generation Functions
 */
/*********************************************************************
 * @fn      SampleApp_HandleKeys
 *
 * @brief   Handles all key events for this device.
 *
 * @param   shift - true if in shift/alt.
 * @param   keys - bit field for key events. Valid entries:
 *                 HAL_KEY_SW_2
 *                 HAL_KEY_SW_1
 *
 * @return  none
 */
void SampleApp_HandleKeys( uint8 shift, uint8 keys )
{
  (void)shift;  // Intentionally unreferenced parameter
  
  if ( keys & HAL_KEY_SW_1 )
  {
    /* This key sends the Flash Command is sent to Group 1.
     * This device will not receive the Flash Command from this
     * device (even if it belongs to group 1).
     */
    SampleApp_SendFlashMessage( SAMPLEAPP_FLASH_DURATION );
  }

  if ( keys & HAL_KEY_SW_2 )
  {
    /* The Flashr Command is sent to Group 1.
     * This key toggles this device in and out of group 1.
     * If this device doesn't belong to group 1, this application
     * will not receive the Flash command sent to group 1.
     */
    aps_Group_t *grp;
    grp = aps_FindGroup( SAMPLEAPP_ENDPOINT, SAMPLEAPP_FLASH_GROUP );
    if ( grp )
    {
      // Remove from the group
      aps_RemoveGroup( SAMPLEAPP_ENDPOINT, SAMPLEAPP_FLASH_GROUP );
    }
    else
    {
      // Add to the flash group
      aps_AddGroup( SAMPLEAPP_ENDPOINT, &SampleApp_Group );
    }
  }
}

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*********************************************************************
 * @fn      SampleApp_MessageMSGCB
 *
 * @brief   Data message processor callback.  This function processes
 *          any incoming data - probably from other devices.  So, based
 *          on cluster ID, perform the intended action.
 *
 * @param   none
 *
 * @return  none
 */
/*void SampleApp_MessageMSGCB( afIncomingMSGPacket_t *pkt )
{
  uint16 flashTime;

  switch ( pkt->clusterId )
  {
    case SAMPLEAPP_P2P_CLUSTERID:        //协调器收到数据通过串口或屏输出
      HalUARTWrite(0, "TEMP:", 5);       //提示接收到数据
      HalUARTWrite(0, pkt->cmd.Data, pkt->cmd.DataLength); //输出接收到的数据
      HalUARTWrite(0, "\n", 1);         // 回车换行
      break;    
    case SAMPLEAPP_PERIODIC_CLUSTERID:
      break;

    case SAMPLEAPP_FLASH_CLUSTERID:
      flashTime = BUILD_UINT16(pkt->cmd.Data[1], pkt->cmd.Data[2] );
      HalLedBlink( HAL_LED_4, 4, 50, (flashTime / 4) );
      break;
  }
}*/



/*void SampleApp_MessageMSGCB(afIncomingMSGPacket_t *pkt) {
    uint16 flashTime;
    static char lcdBuffer[40];
    
    switch (pkt->clusterId) {
        case SAMPLEAPP_P2P_CLUSTERID: {
            // JSON验证
            if (pkt->cmd.DataLength < 50 || pkt->cmd.Data[0] != '{') {
                HalUARTWrite(0, "Invalid JSON format\n", 19);
                break;
            }
            
            // 解析节点ID
            char nodeID[8] = {0};
            sscanf((char*)pkt->cmd.Data + 6, "%6[^\"]", nodeID);
            
            // 解析时间戳
            char timestamp[20] = {0};
            sscanf((char*)pkt->cmd.Data + 35, "%19[^\"]", timestamp);
            
            // 解析传感器数据
            float ds18b20_temp, dht11_temp, dht11_hum;
            char tempStr[8];
            
            // 温度解析
            memset(tempStr, 0, 8);
            sscanf((char*)pkt->cmd.Data + 55, "%7[^\"]", tempStr);
            ds18b20_temp = atof(tempStr);
            
            // 湿度解析
            memset(tempStr, 0, 8);
            sscanf((char*)pkt->cmd.Data + 75, "%7[^\"]", tempStr);
            dht11_temp = atof(tempStr);
            
            // 时间解析
            memset(tempStr, 0, 8);
            sscanf((char*)pkt->cmd.Data + 95, "%7[^\"]", tempStr);
            dht11_hum = atof(tempStr);
            
            // 串口输出原始数据
            HalUARTWrite(0, "Received: ", 9);
            HalUARTWrite(0, pkt->cmd.Data, pkt->cmd.DataLength);
            HalUARTWrite(0, "\r\n", 2);
            
            // LCD显示优化
            // 先格式化到缓冲区，再显示缓冲区内容
            sprintf(lcdBuffer, "T:%.1f/%.1fC H:%.1f%%", 
                    ds18b20_temp, dht11_temp, dht11_hum);
            HalLcdWriteString(lcdBuffer, HAL_LCD_LINE_1); 
            
            sprintf(lcdBuffer, "TS:%s", timestamp);
            HalLcdWriteString(lcdBuffer, HAL_LCD_LINE_2);  
            
            sprintf(lcdBuffer, "Node:%s", nodeID);
            HalLcdWriteString(lcdBuffer, HAL_LCD_LINE_3);  
            
            // 解析结果输出
            //HalUARTWrite(0, "解析结果:\r\n", 10);
            //int len = sprintf(lcdBuffer, "节点ID: %s\r\n", nodeID);
            //HalUARTWrite(0, (uint8*)lcdBuffer, len);
            
            // 其他解析结果...
            break;
        }
        
        case SAMPLEAPP_PERIODIC_CLUSTERID:
            HalLcdWriteString("Periodic Msg", HAL_LCD_LINE_1);
            break;
            
        case SAMPLEAPP_FLASH_CLUSTERID:
            flashTime = BUILD_UINT16(pkt->cmd.Data[1], pkt->cmd.Data[2]);
            HalLedBlink(HAL_LED_4, 4, 50, flashTime/4);
            break;
    }
}*/

void SampleApp_MessageMSGCB(afIncomingMSGPacket_t *pkt) {
    uint16 flashTime;
    static char lcdBuffer[40];
    
    switch (pkt->clusterId) {
        case SAMPLEAPP_P2P_CLUSTERID: {
            // 直接转发接收到的原始数据到HC-25
          
            HalUARTWrite(0, pkt->cmd.Data, pkt->cmd.DataLength);
            
            // 保留原有LCD显示功能（可选）
            if (pkt->cmd.DataLength >= 20) {
                sprintf(lcdBuffer, "RX:%.16s", pkt->cmd.Data);
                HalLcdWriteString(lcdBuffer, HAL_LCD_LINE_1);
            }
            break;
        }
        
        case SAMPLEAPP_PERIODIC_CLUSTERID:
            HalLcdWriteString("Periodic Msg", HAL_LCD_LINE_1);
            break;
            
        case SAMPLEAPP_FLASH_CLUSTERID:
            flashTime = BUILD_UINT16(pkt->cmd.Data[1], pkt->cmd.Data[2]);
            HalLedBlink(HAL_LED_4, 4, 50, flashTime/4);
            break;
    }
}

/*void SampleApp_MessageMSGCB(afIncomingMSGPacket_t *pkt) {
    uint16 flashTime;
    static char lcdBuffer[40];
    static uint8 node_id[2] = {0x11, 0x23}; // 默认节点ID
    
    switch (pkt->clusterId) {
        case SAMPLEAPP_P2P_CLUSTERID: {
            // 原有功能：JSON解析与显示
            if (pkt->cmd.DataLength >= 50 && pkt->cmd.Data[0] == '{') {
                // 解析节点ID
                char nodeID[8] = {0};
                sscanf((char*)pkt->cmd.Data + 6, "%6[^\"]", nodeID);
                
                // 解析时间戳
                char timestamp[20] = {0};
                sscanf((char*)pkt->cmd.Data + 35, "%19[^\"]", timestamp);
                
                // 解析传感器数据
                float ds18b20_temp, dht11_temp, dht11_hum;
                char tempStr[8];
                
                // DS18B20温度解析
                memset(tempStr, 0, 8);
                sscanf((char*)pkt->cmd.Data + 55, "%7[^\"]", tempStr);
                ds18b20_temp = atof(tempStr);
                
                // DHT11温度解析
                memset(tempStr, 0, 8);
                sscanf((char*)pkt->cmd.Data + 75, "%7[^\"]", tempStr);
                dht11_temp = atof(tempStr);
                
                // DHT11湿度解析
                memset(tempStr, 0, 8);
                sscanf((char*)pkt->cmd.Data + 95, "%7[^\"]", tempStr);
                dht11_hum = atof(tempStr);
                
                // 串口输出原始数据
                HalUARTWrite(0, "Received: ", 9);
                HalUARTWrite(0, pkt->cmd.Data, pkt->cmd.DataLength);
                HalUARTWrite(0, "\r\n", 2);
                
                // LCD显示优化
                sprintf(lcdBuffer, "T:%.1f/%.1fC H:%.1f%%", 
                        ds18b20_temp, dht11_temp, dht11_hum);
                HalLcdWriteString(lcdBuffer, HAL_LCD_LINE_1);
                
                sprintf(lcdBuffer, "TS:%s", timestamp);
                HalLcdWriteString(lcdBuffer, HAL_LCD_LINE_2);
                
                sprintf(lcdBuffer, "Node:%s", nodeID);
                HalLcdWriteString(lcdBuffer, HAL_LCD_LINE_3);
            }
            
            // 新增功能：直接转发原始数据到HC-25
            HalUARTWrite(0, "TX_HC25: ", 8);        // 调试前缀
            HalUARTWrite(0, pkt->cmd.Data, pkt->cmd.DataLength);  // 原始数据转发
            HalUARTWrite(0, "\r\n", 2);             // 帧结束符
           
            break;
        }
        
        case SAMPLEAPP_PERIODIC_CLUSTERID:
            HalLcdWriteString("Periodic Msg", HAL_LCD_LINE_1);
            break;
            
        case SAMPLEAPP_FLASH_CLUSTERID:
            flashTime = BUILD_UINT16(pkt->cmd.Data[1], pkt->cmd.Data[2]);
            HalLedBlink(HAL_LED_4, 4, 50, flashTime/4);
            break;
    }
}*/


/*********************************************************************
 * @fn      SampleApp_SendPeriodicMessage
 *
 * @brief   Send the periodic message.
 *
 * @param   none
 *
 * @return  none
 */
void SampleApp_SendPeriodicMessage( void )
{
  if ( AF_DataRequest( &SampleApp_Periodic_DstAddr, &SampleApp_epDesc,                                       
                       SAMPLEAPP_PERIODIC_CLUSTERID,
                       1,
                       (uint8*)&SampleAppPeriodicCounter,
                       &SampleApp_TransID,
                       AF_DISCV_ROUTE,
                       AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )
  {
  }
  else
  {
    // Error occurred in request to send.
  }
}

/*********************************************************************
 * @fn      SampleApp_SendFlashMessage
 *
 * @brief   Send the flash message to group 1.
 *
 * @param   flashTime - in milliseconds
 *
 * @return  none
 */
void SampleApp_SendFlashMessage( uint16 flashTime )
{
  uint8 buffer[3];
  buffer[0] = (uint8)(SampleAppFlashCounter++);
  buffer[1] = LO_UINT16( flashTime );
  buffer[2] = HI_UINT16( flashTime );

  if ( AF_DataRequest( &SampleApp_Flash_DstAddr, &SampleApp_epDesc,
                       SAMPLEAPP_FLASH_CLUSTERID,
                       3,
                       buffer,
                       &SampleApp_TransID,
                       AF_DISCV_ROUTE,
                       AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )
  {
  }
  else
  {
    // Error occurred in request to send.
  }
}

/*********************************************************************
 * @fn      SampleApp_Send_P2P_Message
 *
 * @brief   point to point.
 *
 * @param   none
 *
 * @return  none
 */
/*void SampleApp_Send_P2P_Message( void )
{
  byte str[5];
  char strTemp[10];
  byte temp;

  temp = ReadDs18B20();                //读取温度数据
  str[0] = temp/10+48;
  str[1] = temp%10+48;
  str[2] = ' ';
  str[3] = 'C';
  str[4] = '\0';    
       
  HalUARTWrite(0, "TEMP:", 5);         //终端通过串口输出提示信息
  HalUARTWrite(0, str, 4);
  HalUARTWrite(0, "\n",1);

  osal_memcpy(strTemp, "TEMP:", 5);
  osal_memcpy(&strTemp[5], str, 5);
  HalLcdWriteString(strTemp, HAL_LCD_LINE_3); //LCD显示
   
  //把温度数据无线发给协调器
  if ( AF_DataRequest( &SampleApp_P2P_DstAddr, &SampleApp_epDesc,
                       SAMPLEAPP_P2P_CLUSTERID,
                       2,
                       str,
                       &SampleApp_TransID,
                       AF_DISCV_ROUTE,
                       AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )
  {
  }
  else
  {
    // Error occurred in request to send.
  }
}*/

/*void SampleApp_Send_P2P_Message( void )
{
  byte str[8];        // 扩展缓冲区大小
  char strTemp[16];   // 扩展LCD显示缓冲区
  float temp;         // 改为浮点类型存储温度

  temp = floatReadDs18B20();         // 使用带小数的读取函数
  // 格式化温度字符串（带1位小数）
  sprintf((char*)str, "%d.%d", (int)temp, (int)(temp*10) % 10);
  
  HalUARTWrite(0, "TEMP:", 5);
  HalUARTWrite(0, (byte*)str, strlen((char*)str));
  HalUARTWrite(0, " °C\r\n", 6);
  
  sprintf(strTemp, "TEMP:%s°C", str);
  HalLcdWriteString(strTemp, HAL_LCD_LINE_3);
  
  // 无线发送完整温度数据（带小数）
  if ( AF_DataRequest( &SampleApp_P2P_DstAddr, &SampleApp_epDesc,
                       SAMPLEAPP_P2P_CLUSTERID,
                       strlen((char*)str),      // 发送完整长度
                       str,
                       &SampleApp_TransID,
                       AF_DISCV_ROUTE,
                       AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )
  {
  }
  else
  {
   
  }
}*/

void SampleApp_Send_P2P_Message(void) {
    // 定义传感器数据结构（全部使用整数类型）
    typedef struct {
        float ds18b20_temp;    // 保留DS18B20的浮点格式
        char dht11_temp[3];     // DHT11温度
        char dht11_hum[3];      // DHT11湿度
        char timestamp[20];
    } SensorData;
    
    SensorData data;
    char json_str[256];
    static uint8 node_id[2];
    
    // 节点ID生成
    #ifdef ZDAPP_CONFIG_PAN_ID
        node_id[0] = (ZDAPP_CONFIG_PAN_ID >> 8) & 0xFF;
        node_id[1] = ZDAPP_CONFIG_PAN_ID & 0xFF;
    #else
        node_id[0] = 0x11; 
        node_id[1] = 0x23;
    #endif
    
    const uint8 device_id[6] = {0x23,0x21,0x54,0x10,0x13,0x06};
    
    // 读取传感器数据
    data.ds18b20_temp = floatReadDs18B20();  // 读取DS18B20数据
    DHT11();    // 调用DHT11读取函数
    
    // 直接获取整数形式的温湿度值
    data.dht11_temp[0] = wendu_shi+0x30;
    data.dht11_temp[1] = wendu_ge+0x30;
    data.dht11_temp[2] = '\0';
    data.dht11_hum[0] = shidu_shi+0x30;
    data.dht11_hum[1] = shidu_ge+0x30;
    data.dht11_hum[2] = '\0';
    
    
    // 手动拼接时间戳
    uint32 timestamp = osal_getClock();
    uint8 sec = timestamp % 60;
    uint8 min = (timestamp / 60) % 60;
    uint8 hr = (timestamp / 3600) % 24;
    sprintf(data.timestamp, "%02d:%02d:%02d", hr, min, sec);
    
    // 构建JSON数据包（使用整数格式）
    char ds18b20_str[8];
    
    // 格式化传感器数据
    sprintf(ds18b20_str, "%.1f", data.ds18b20_temp);  // DS18B20保留一位小数
    
    // 构建完整JSON字符串
    int json_len = sprintf(json_str,
                      "{\"PIN_ID\":\"%02X%02X\",\"ID\":\"%02X%02X%02X%02X%02X%02X\",\"Devices\":"
                        "{\"DS18B20-Temp\":%s,\"DHT11-Temp\":\"%s\",\"DHT11-Humidity\":\"%s\",""\"Time\":\"%s\"}}",
                        node_id[0], node_id[1],
                        device_id[0], device_id[1], device_id[2], 
                        device_id[3], device_id[4], device_id[5],
                        ds18b20_str, data.dht11_temp, data.dht11_hum,
                        data.timestamp
                          );
    
    // 串口输出调试信息
    HalUARTWrite(0, "TX JSON: ", 8);
    int offset = 0;
    while(offset < json_len) {
      // 每次发送128字节（避免超过串口缓冲区）
      int chunk_size = (json_len - offset) > 128 ? 128 : (json_len - offset);
      HalUARTWrite(0, (uint8*)(json_str + offset), chunk_size);
      offset += chunk_size;
    }
    HalUARTWrite(0, "\r\n", 2);
    
    // 无线发送
    AF_DataRequest(&SampleApp_P2P_DstAddr, &SampleApp_epDesc,
                   SAMPLEAPP_P2P_CLUSTERID,
                   json_len,
                   (uint8*)json_str,
                   &SampleApp_TransID,
                   AF_DISCV_ROUTE,
                   AF_DEFAULT_RADIUS);

}

/*void SampleApp_Send_P2P_Message(void) {
  // 定义传感器数据结构（全部使用整数类型）
  typedef struct {
    float ds18b20_temp;
    int dht11_temp;  // 改为整数类型
    int dht11_hum;   // 改为整数类型
    char timestamp[20];
  } SensorData;
  
  SensorData data;
  char json_str[256];
  static uint8 node_id[2];
  
  // 节点ID生成
#ifdef ZDAPP_CONFIG_PAN_ID
  node_id[0] = (ZDAPP_CONFIG_PAN_ID >> 8) & 0xFF;
  node_id[1] = ZDAPP_CONFIG_PAN_ID & 0xFF;
#else
  node_id[0] = 0x11; 
  node_id[1] = 0x23;
#endif
  
  const uint8 device_id[6] = {0x23,0x21,0x54,0x10,0x13,0x06};
  
  // 读取传感器数据
  data.ds18b20_temp = floatReadDs18B20();  // 读取DS18B20数据
  DHT11();    // 调用DHT11读取函数
  
  data.dht11_temp = (wendu_shi - 0x30) * 10 + (wendu_ge - 0x30);
  data.dht11_hum = (shidu_shi - 0x30) * 10 + (shidu_ge - 0x30);
  
  
  // 手动拼接时间戳
  uint32 timestamp = osal_getClock();
  uint8 sec = timestamp % 60;
  uint8 min = (timestamp / 60) % 60;
  uint8 hr = (timestamp / 3600) % 24;
  sprintf(data.timestamp, "%02d:%02d:%02d", hr, min, sec);
  
  // 构建JSON数据包（使用整数格式）
  char ds18b20_str[8];
  
  // 格式化传感器数据
  sprintf(ds18b20_str, "%.1f", data.ds18b20_temp);  // DS18B20保留一位小数
  
  int json_len = sprintf(json_str,
                         "{\"PIN_ID\":\"%02X%02X\",\"ID\":\"%02X%02X%02X%02X%02X%02X\",\"Devices\":"
                           "{\"DS18B20-Temp\":%.1f,\"DHT11-Temp\":%d,\"DHT11-Humidity\":%d,\"Time\":\"%s\"}}",
                           node_id[0], node_id[1],
                           device_id[0], device_id[1], device_id[2], device_id[3], device_id[4], device_id[5],
                           data.ds18b20_temp, 
                           data.dht11_temp,    // 整数形式温度
                           data.dht11_hum,     // 整数形式湿度
                           data.timestamp);
  // 串口输出调试信息
  HalUARTWrite(0, "TX JSON: ", 8);
  int offset = 0;
  while(offset < json_len) {
    // 每次发送128字节（避免超过串口缓冲区）
    int chunk_size = (json_len - offset) > 128 ? 128 : (json_len - offset);
    HalUARTWrite(0, (uint8*)(json_str + offset), chunk_size);
    offset += chunk_size;
  }
  HalUARTWrite(0, "\r\n", 2);
  
  // 无线发送
  AF_DataRequest(&SampleApp_P2P_DstAddr, &SampleApp_epDesc,
                 SAMPLEAPP_P2P_CLUSTERID,
                 json_len,
                 (uint8*)json_str,
                 &SampleApp_TransID,
                 AF_DISCV_ROUTE,
                 AF_DEFAULT_RADIUS);
  
}*/

/*********************************************************************
*********************************************************************/
