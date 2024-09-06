#ifndef _BL_USBCOMM_H_
#define _BL_USBCOMM_H_
#include <libusb-1.0/libusb.h>
#include "Poco/Event.h"
#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "Msg_Prot.h"

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

enum ABC_Cmd
{
	ABC_Null = 0xff,
	ABC_NOP = 0x00,
	ABC_Negative_Response = 0x7f,
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

enum Aux_Bootloader_NRC
{
	AuxBL_POSITIVE = 0x00,
	AuxBL_NRC_SIZE_ZERO = 0x81,
	AuxBL_NRC_SIZE_nZERO = 0x82,
	AuxBL_NRC_SIZE_EXCEED = 0x83,
	AuxBL_NRC_ADDR_OUTRANGE = 0x84,
	AuxBL_NRC_DATA_OUTRANGE = 0x85,
};

typedef union
{
	unsigned char byte[3];
	unsigned int dword;
	struct
	{
		unsigned char LB;
		unsigned char MB;
		unsigned char HB;
	};
} USB_BL_Protocol_Addr_t;

typedef struct
{
	unsigned char cmd_id;
	USB_BL_Protocol_Addr_t addr;
	unsigned char size;
	unsigned char data[BL_USB_DATASIZE];
} USB_BL_Protocol_t;

typedef struct
{
	USB_BL_Protocol_t AuxBL_MsgFbk;
	Poco::Event AuxBL_MsgFbk_wakeup;
} AuxBL_MsgFbk_t;



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
} usb_msg_nop_t;

typedef struct
{
	USB_TaskResp_msg_t nop_fbk;
	Poco::Event nop_fbk_wake;
} usb_msg_nop_fbk_t;

// typedef struct
// {
// 	unsigned char cmd_id;
// 	unsigned char sub_func;
// 	unsigned char data[BL_USB_DATASIZE];
// } USB_msg_rx_t;

// typedef struct
// {
// 	unsigned char response_code;
// 	unsigned char response_nrc;
// 	unsigned char data[BL_USB_DATASIZE];
// } USB_msg_tx_t;


// ONS_RTC Protocol
typedef struct
{
	unsigned short Id;			// Message ID according to the Global USB Message list.
	unsigned short Size;		// Total size of message include the header size.
	unsigned int Serial_Number; // Message S/N this number is  for each message sent.
	unsigned int Time_Stamp;	// Current system time in millisecond.
} AuxBL_Message_Header_t;

typedef struct
{
	AuxBL_Message_Header_t Msg_Hdr;
} AuxBL_Msg_Prot_Protocol_Validation_Req_Msg_t;

typedef struct
{
	AuxBL_Message_Header_t Msg_Hdr;
} AuxBL_Msg_Prot_Perform_Init_Req_Msg_t;

typedef struct
{
	AuxBL_Message_Header_t Msg_Hdr;
	USB_BL_Protocol_Addr_t addr;
	unsigned char size;
} AuxBL_Msg_Prot_Read_ConfigReg_Msg_t;

typedef struct
{
	AuxBL_Message_Header_t Msg_Hdr;
	USB_BL_Protocol_Addr_t addr;
	unsigned char size;
	unsigned char data[32];
} AuxBL_Msg_Prot_Write_ConfigReg_Msg_t;

typedef struct
{
	AuxBL_Message_Header_t Msg_Hdr;
	unsigned char reset_mode;
} AuxBL_Msg_Prot_Reset_PIC_Msg_t;

typedef struct
{
	AuxBL_Message_Header_t Msg_Hdr;
	unsigned char segment_sel;
} AuxBL_Msg_Prot_Read_SegChksum_Msg_t;

typedef struct
{
	int RTC_Base_Ver;
	int RTC_Major_Ver;
	int RTC_Minor_Ver;
	int Msg_Prot_Base;
	int Msg_Prot_Major;
	int Msg_Prot_Minor;
	Poco::Event AuxBL_MsgFbk_wakeup;
} AuxBL_ONS_Validation_t;

typedef struct
{
	INT_16 Status;
	Poco::Event AuxBL_MsgFbk_wakeup;
} AuxBL_ONS_Init_Status_t;

typedef struct
{
	unsigned char addr[3];
	unsigned char size;
	unsigned char data[32];
	Poco::Event AuxBL_MsgFbk_wakeup;
} AuxBL_ONS_ReadConfigReg_t;

typedef struct
{
	unsigned char addr[3];
	unsigned char size;
	unsigned char data[32];
	Poco::Event AuxBL_MsgFbk_wakeup;
} AuxBL_ONS_WriteConfigReg_t;

typedef struct
{
	unsigned char reset_pic_mode;
	Poco::Event AuxBL_MsgFbk_wakeup;
} AuxBL_ONS_Reset_PIC_t;

typedef struct
{
	unsigned short chksum;
	Poco::Event AuxBL_MsgFbk_wakeup;
} AuxBL_ONS_Read_SegChksum_t;

typedef struct
{
	unsigned short RTC_Base;
	unsigned short RTC_Major;
	unsigned short RTC_Minor;
	unsigned short MsgProt_Base;
	unsigned short MsgProt_Major;
	unsigned short MsgProt_Minor;
	unsigned short BL_Base;
	unsigned short BL_Major;
	unsigned short BL_Minor;
} ONS_VersionNum_t;

int BL_USBComm_Driver_GetDeviceList(int *suitable_device, int en_printscr);
int BL_USBComm_Driver_SelectTargetDevice(unsigned short idVendor, unsigned short idProduct);
int BL_USBComm_Driver_getTargetDevice(void);
int BL_USBComm_Driver_openTargetDevice(UsbDevice dev);
int BL_USBComm_Driver_closeDeviceHandle(void);

int BL_USBComm_Driver_Threads_Control(BL_USBThreads_Control cmd);
int BL_USBComm_Driver_Init_Threads(void);
void *BL_USBComm_Rx_Thread_Main(void *p);
void *BL_USBComm_Tx_Thread_Main(void *p);
void *BL_USBComm_Task_Service(void *p);
void *BL_USBComm_AutoReConnect(void *p);

int BL_USBComm_Rx_Terminate(void);
int BL_USBComm_Tx_Terminate(void);
int BL_USBComm_Task_Terminate(void);
int BL_USBComm_AutoReConnect_Terminate(void);

int BL_USBComm_Get_RTC_Device_Status(void);

int BL_USBComm_Driver_Init_LIBUSB(void);
// int BL_USBComm_Driver_Find_BL_App_Device_with_Open(char **str_product, char **str_manufature);

int BL_USBComm_Driver_TransData_bulk();
int BL_USBComm_Driver_TransData_interrupt();
int testing_TransData(int i);

// char BL_USBComm_PushBuffer(unsigned char* usb_msg, unsigned char msg_size);

void BL_USBComm_TransStateInit(void);

char BL_USB_FI_TxBuffer(USB_BL_Protocol_t send_msg, unsigned char msg_size);
int BL_USB_FO_TxBuffer(void);
char BL_USB_FI_RxBuffer(unsigned char data_in[], int data_length);
char BL_USB_FO_RxBuffer(USB_BL_Protocol_t *msg_cmd, unsigned char *msg_size);

char BL_RxMsgParser(USB_BL_Protocol_t msg_fbk);
void BL_RxMsg_WakeupSet(USB_BL_Protocol_t msg_fbk);

int AuxBL_NOP_Message(int msg_addr, int data_size, int *fbk_addr, unsigned char *fbk_datasize);
int AuxBL_NOP_Feedback(USB_BL_Protocol_t receiv_msg);

int AuxBL_ReadFlashMemory_Message(int msg_addr, int req_size, int *fbk_addr, unsigned char *fbk_data, unsigned char *fbk_datasize);
int AuxBL_ReadFlashMemory_Feedback(USB_BL_Protocol_t receiv_msg);

int AuxBL_Clear_GenFlash_Message(int msg_addr, int data_size, int *fbk_addr, unsigned char *fbk_datasize);
int AuxBL_Clear_GenFlash_Feedback(USB_BL_Protocol_t receiv_msg);

// int AuxBL_WriteFlashMemory_Message(int msg_addr, int wr_size, unsigned char *wr_datamsg, int *fbk_addr, unsigned char *fbk_datasize);
int AuxBL_WriteFlashMemory_Message(int msg_addr, int wr_size, unsigned char *wr_datamsg, int *fbk_addr, unsigned char *fbk_datasize, unsigned char *fbk_chksum);
int AuxBL_WriteFlashMemory_Feedback(USB_BL_Protocol_t receiv_msg);

int AuxBL_ReadConfigReg_Message(int msg_addr, int req_size, int *fbk_addr, unsigned char *fbk_data, unsigned char *fbk_datasize);
int AuxBL_ReadConfigReg_Feedback(USB_BL_Protocol_t receiv_msg);

int AuxBL_WriteConfigReg_Message(int msg_addr, int wr_size, unsigned char wr_datamsg, int *fbk_addr, unsigned char *fbk_datasize);
int AuxBL_WriteConfigReg_Feedback(USB_BL_Protocol_t receiv_msg);

int AuxBL_Reset_Message(int msg_addr, int data_size, int *fbk_addr, unsigned char *fbk_datasize);
int AuxBL_Reset_Feedback(USB_BL_Protocol_t receiv_msg);

int AuxBL_Read_Segment_Checksum_Message(unsigned char segment_sel, unsigned short *fbk_chksum);
int AuxBL_Segment_Checksum_Feedback(USB_BL_Protocol_t receiv_msg);

int AuxBL_Read_ONS_VersionNumber_Message(int offset_addr, unsigned char version_msg_sel, int *fbk_addr, unsigned char *fbk_data, unsigned char *fbk_datasize);
int AuxBL_ONS_VersionNumber_Feedback(USB_BL_Protocol_t receiv_msg);

int AuxBL_AuxBL_NRC_Feedback(USB_BL_Protocol_t receiv_msg);

// ONS protocol.
unsigned long BL_USB_GetCurrentTime(void);
char BL_USB_FI_TxBuffer_ONS_Protocol(unsigned char *send_msg, unsigned char msg_size);
char BL_USB_FO_ONSRTC_RxBuffer(unsigned char *ons_msg);
char BL_ONSRTC_RxMsgParser(unsigned char *ons_msg);

// ONS Protocol pack/unpack.
int AuxBL_Msg_Prot_Pack_Validation_Req(void);
int AuxBL_Msg_Prot_Pack_Preform_INIT_Req(void);
int AuxBL_Msg_Prot_Pack_Read_ConfigReg_Req(unsigned int configreg_addr, unsigned char size);
int AuxBL_Msg_Prot_Unpack_Read_ConfigReg(ONS_BYTE *Msg_Buff, unsigned char *fbk_addr, unsigned char *fbk_size, unsigned char *fbk_data);
int AuxBL_Msg_Prot_Pack_Write_ConfigReg_Req(unsigned int configreg_addr, unsigned char size, unsigned char data);
int AuxBL_Msg_Prot_Unpack_Write_ConfigReg(ONS_BYTE *Msg_Buff, unsigned char *fbk_addr, unsigned char *fbk_size, unsigned char *fbk_data);
int AuxBL_Msg_Prot_Pack_Reset_PIC_Req(unsigned char reset_mode);
int AuxBL_Msg_Prot_Unpack_Reset_PIC(ONS_BYTE *Msg_Buff, unsigned char *reset_mode);
int AuxBL_Msg_Prot_Pack_Read_Segment_Checksum_Req(unsigned char segment_sel);
int AuxBL_Msg_Prot_Unpack_Read_Segment_Checksum(ONS_BYTE *Msg_Buff, unsigned short *fbk_chksum);

int AuxBL_ONS_VerionReq(int *rtc_base, int *rtc_major, int *rtc_minor, int *prot_base, int *prot_major, int *prot_minor);
int AuxBL_ONS_InitReq(int *init_status);
int AuxBL_ONS_Read_ConfigReg_Req(unsigned int Req_Addr, unsigned int *Fbk_Addr, unsigned char *FbkVal);
int AuxBL_ONS_Write_ConfigReg_Req(unsigned int Req_Addr, unsigned char ConfigByte, unsigned int *Fbk_Addr, unsigned char *FbkVal);
int AuxBL_ONS_Reset_PIC_Req(unsigned char reset_mode, unsigned char *Fbk_reset_mode);
int AuxBL_ONS_Segment_Checksum_Req(unsigned char segment_sel, unsigned short *fbk_chksum);


int msg_nop(unsigned char sub_func);

char USB_Msg_From_RxBuffer(usb_msg_u8 *msg_cmd, unsigned char *msg_size);
char USB_Msg_To_TxBulkBuffer(ptr_usb_msg_u8 send_msg, unsigned char msg_size);

#endif