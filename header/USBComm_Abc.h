#ifndef _USBCOMM_ABC_H_
#define _USBCOMM_ABC_H_
#include <libusb-1.0/libusb.h>
#include "Poco/Event.h"
#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
//#include "Msg_Prot.h"

#define USB_VID_Bootloader_RTC 0xf000
#define USB_PID_Bootloader_RTC 0x5109
#define USB_VID_ONS_RTC 0x04d8
#define USB_PID_ONS_RTC 0x0204
#define USB_VID_ABC_RTC 0x1d6b
#define USB_PID_ABC_RTC 0x5109

#define BL_USB_CMD_ID_OFFSET 0x40
#define TRANSACTION_SIZE 8 // row,Transaction
#define MESSAGE_MAX 64	   // column
#define ADDR_MASK 0x00ffffff
#define ADDR_GenSeg_Test1 0x0557C0
#define ADDR_Config_FICD 0xF8000E
#define BIT_RSTPRI 0x04
#define BL_USB_DATASIZE 48
#define MSG_DATA_SIZE 62

typedef unsigned char *ptr_usb_msg_u8;
typedef unsigned char usb_msg_u8;

// enum libusb_error {
//     LIBUSB_SUCCESS             = 0,
//     LIBUSB_ERROR_IO            = -1,
//     LIBUSB_ERROR_INVALID_PARAM = -2,
//     LIBUSB_ERROR_ACCESS        = -3,
//     LIBUSB_ERROR_NO_DEVICE     = -4,
//     LIBUSB_ERROR_NOT_FOUND     = -5,
//     LIBUSB_ERROR_BUSY          = -6,
//     LIBUSB_ERROR_TIMEOUT       = -7,
//     LIBUSB_ERROR_OVERFLOW      = -8,
//     LIBUSB_ERROR_PIPE          = -9,
//     LIBUSB_ERROR_INTERRUPTED   = -10,
//     LIBUSB_ERROR_NO_MEM        = -11,
//     LIBUSB_ERROR_NOT_SUPPORTED = -12,
//     LIBUSB_ERROR_OTHER         = -99,
// };

enum UsbDevice
{
	Device_NULL = -1,
	Device_ABC_RTC = 1,
};

enum Protocol_Command
{
	Cmd_Echo = 0x00,
	Cmd_MAX,
};

enum Protocol_PositiveResponse
{
	RespPos_Echo = 0x40,
};

enum Protocol_NegativeResponse
{
	RespNeg = 0x7f,
};

enum Reponse_Code
{
	POSITIVE_CODE = 0x00,
	NRC_SIZE_ZERO = 0x81,
	NRC_SIZE_nZERO = 0x82,
	NRC_SIZE_EXCEED = 0x83,
	NRC_ADDR_OUTRANGE = 0x84,
	NRC_DATA_OUTRANGE = 0x85,
	NRC_CMD_NOT_FOUND = 0x86,
};

typedef struct
{
	libusb_device_handle *Dev_Handle;		// USB device handle
	libusb_context *Ctx;					// USB libusb session
	pthread_mutex_t Rx_Buffer_Mutex;		// Mutex to prevent read/Write problem.
	pthread_mutex_t Tx_Buffer_Mutex;		// Mutex to prevent read/Write problem.
	pthread_t USBComm_Rx_Thread;			// USB->host receive thread
	pthread_t USBComm_Tx_Thread;			// host->USB transmit thread
	pthread_t USBComm_Task_Thread;			// check every 1ms.
	pthread_t USBComm_AutoReConnect_Thread; // check every 0.5sec to connect.
	unsigned char device_EPn_IN;
	unsigned char device_EPn_OUT;
	int LIBUSB_Init_Success;
	int LIBUSB_RxTx_Pthread_Success;
	int USB_Device_Connect_Success;
	UsbDevice USB_TargetDevice;
} BL_USBComm_Driver_Elment_t;

enum BL_USBThreads_Control
{
	Threads_reset = 0,
	Threads_set = 1,  // wake.
	Threads_wait = 2, // sleep.
};

typedef struct
{
	Poco::Event g_USBComm_Rx_idle;
	Poco::Event g_USBComm_Tx_idle;
	Poco::Event g_USBComm_Task_idle;
} BL_USBComm_Driver_Idle_t;

typedef struct
{
	bool Init;
	bool flag_ISR;
	bool Mutex; // check TxMutex evey 1ms.
	bool Stuck;
	char Ptr_buff;
	char Ptr_comp;
	unsigned char Buff[TRANSACTION_SIZE][MESSAGE_MAX]; //[row][column]
	unsigned char MsgSize[TRANSACTION_SIZE];
} USB_Transaction_State_t;

enum Aux_Bootloader_Cmd
{
	AuxBL_Null = 0xff,
	AuxBL_NOP = 0x00,
	AuxBL_Read_SegmentChecksum = 0x01,
	AuxBL_Clear_GenSeg_Flash = 0x02,
	AuxBL_Read_PIC_FlashMemory = 0x05,
	AuxBL_Write_PIC_FlashMemory = 0x06,
	AuxBL_Read_ONS_VersionMsg = 0x07,
	AuxBL_Read_PIC_Config = 0x0a,
	AuxBL_Write_PIC_Config = 0x0b,
	AuxBL_Reset_PIC = 0x10,
	AuxBL_Reset_PIC_WriteConfig = 0x11,
	AuxBL_Negative_Response = 0x7f,
};


// protocol:
//                               byte0  byte1 byte2 byte3     byte4 byte5.....
//   Host->Client(cmd):          cmd_id AddrL AddrM AddrH     size  data[n]
//   Client->Host(feedback):     offset AddrL AddrM AddrH     size  data[n]
// 1.AuxBL_NOP                   0x00   0xrr  0xss  0xtt      0
//   feedback                    0x40   0xrr  0xss  0xtt+0x40 0
// 2.AuxBL_Read_SegmentChecksum  0x01   0xff  0xff  0xff      1     0/1    0:general segment, 1:auxiliary segment.
//   feedback                    0x41   0x00  0x00  0x00      2     chksumL  chksumH
// 3.AuxBL_Clear_GenSeg_Flash    0x02   0xff  0xff  0xff      0
//   feedback                    0x42   0x00  0x00  0x00      0
// 4.AuxBL_Read_PIC_FlashMemory  0x05   0xrr  0xss  0xtt      16~32
//   feedback                    0x45   0xrr  0xss  0xtt      16~32 data[0]...data[size-1]
// 5.AuxBL_Write_PIC_FlashMemory 0x06   0xuu  0xvv  0xww      16~32 data[0]...data[size-1]
//   feedback                    0x46   0x00  0x00  0x00      0
// 6.AuxBL_Read_ONS_VersionMsg   0x07   0xii  0xjj  0xkk      1          0/1    0:version number(18bytes), 1:version note(48bytes), addr:memory_offset.
//   feedback                    0x47   0xii  0xjj  0xkk      18~48      data[n]
// 7.AuxBL_Read_PIC_Config       0x0a   0xii  0xjj  0xkk      1
//   feedback                    0x4a   0xii  0xjj  0xkk      1          data[0]
// 8.AuxBL_Write_PIC_Config      0x0b   0xii  0xjj  0xkk      1          data[0]
//   feedback                    0x4b   0x00  0x00  0x00      0
// 9.AuxBL_Reset_PIC             0x10   0xff  0xff  0xff      0
//   feedback                    0x50   0x00  0x00  0x00      0


typedef struct
{
	unsigned char cmd_id;
	unsigned char sub_func;
	unsigned char data[MSG_DATA_SIZE];
} USB_Task_msg_t;

typedef struct
{
	unsigned char cmd_id_rep;
	unsigned char sub_func;
	unsigned char data[MSG_DATA_SIZE];
} USB_TaskResp_msg_t;

typedef struct
{
	unsigned char resp_id;
	unsigned char cmd_id;
	unsigned char neg_code;
	unsigned char ignore;
	unsigned char data[60];
} USB_NegResponse_msg_t;


typedef struct
{
	unsigned char cmd_id;
	unsigned char sub_func;
	unsigned char data[2];
} usb_msg_echo_t;

typedef struct
{
	USB_TaskResp_msg_t echo_fbk;
	Poco::Event echo_fbk_wake;
} usb_msg_echo_fbk_t;

int USBComm_Driver_GetDeviceList(int *suitable_device, int en_print);
int USBComm_Driver_SelectTargetDevice(unsigned short idVendor, unsigned short idProduct);
int USBComm_Driver_getTargetDevice(void);
int USBComm_Driver_openTargetDevice(UsbDevice dev);
int USBComm_Driver_closeDeviceHandle(void);

void USBComm_Driver_Threads_reset();
void USBComm_Driver_Threads_set();
int USBComm_Driver_Init_Threads(void);
void *USBComm_Rx_Thread_Main_Abc(void *p);
void *USBComm_Tx_Thread_Main_Abc(void *p);
void *USBComm_Task_Service_Abc(void *p);
void *USBComm_AutoReConnect_Abc(void *p);

int USBComm_Rx_Terminate(void);
int USBComm_Tx_Terminate(void);
int USBComm_Task_Terminate(void);
int USBComm_AutoReConnect_Terminate(void);

int USBComm_Get_RTC_Device_Status(void);
int USBComm_Driver_Init_LIBUSB(void);

int BL_USBComm_Driver_TransData_bulk();
int BL_USBComm_Driver_TransData_interrupt();

void BL_USBComm_TransStateInit(void);
char USB_TxBulkBuffer_To_Bus(void);
char USB_RxBulkBuffer_Get_From_Bus(unsigned char data_in[], int data_length);
char USB_Msg_From_RxBuffer(usb_msg_u8 *msg_cmd, unsigned char *msg_size);
char USB_Msg_To_TxBulkBuffer(ptr_usb_msg_u8 send_msg, unsigned char msg_size);

unsigned long GetCurrentTime_us(void);
int usb_message_echo(unsigned char sub_func);


#endif