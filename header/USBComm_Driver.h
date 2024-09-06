// ---------------------------------------------------------------------------
//  Filename: USBComm_Driver.h
//  Created by: Nissim Avichail
//  Date:  30/06/15
//  Orisol (c)
// ---------------------------------------------------------------------------
#ifndef _USBCOMM_DRIVER_H_
#define _USBCOMM_DRIVER_H_

#include "ONS_General.h"
#ifdef __cplusplus
extern "C"
{
#endif

    // ----------------
    //   Prototypes
    //------------------
    int USBComm_Driver_Set_Ipv4(char *szNewIpv4);
    int USBComm_Driver_Set_Port(char *szNewPort);
    void USBComm_Driver_Set_Debug(int iRtcId, int Log_message_levels);
    int USBComm_Driver_Init(int iRtcId);
    int USBComm_Driver_Open_USB_Device(int iRtcId);
    int USBComm_Driver_Is_Kernel_Driver_Active(int iRtcId);
    int USBComm_Driver_Detach_Kernel_Driver(int iRtcId);
    int USBComm_Driver_Release_Interface(int iRtcId);
    int USBComm_Driver_Claim_Interface(int iRtcId);
    void USBComm_Driver_Close_USB_Device(int iRtcId);
    void USBComm_Driver_Exit(int iRtcId);
    int USBComm_Driver_Bulk_Send_Message(int iRtcId, ONS_BYTE *outgoing_message, int message_size, int *actual);
    int USBComm_Driver_Bulk_Receive_Message(int iRtcId, ONS_BYTE *incomming_message, int *actual);
    int USBComm_Driver_Tx_Mutex_Lock(void);
    int USBComm_Driver_Rx_Mutex_Lock(void);
    int USBComm_Driver_Tx_Mutex_UNLock(void);
    int USBComm_Driver_Rx_Mutex_UNLock(void);
    int USBComm_Driver_Create_Tx_Thread(int iRtcId);
    int USBComm_Driver_Terminate_Tx_Thread (void);
    int USBComm_Driver_Create_Rx_Thread(int iRtcId);
    int USBComm_Driver_Terminate_Rx_Thread (void);
    int USBComm_Driver_Tx_Thread_kill(int iRtcId);
    int USBComm_Driver_Rx_Thread_kill(int iRtcId);
    void USBComm_Driver_Sleep_Millisecond(unsigned int Time_Millisecond);
    int USBComm_Driver_Find_RTC_Device(int iRtcId, char **str_product, char **str_manufature);

#ifdef _ROLE_VIRTUAL_RTC
    int USBCheckFirstInit(void);
#endif
#ifdef __cplusplus
}
#endif
#endif //_USBCOMM_DRIVER_H_
