//#include "return_code.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "Timers.h"
#include <libusb-1.0/libusb.h>
#include <iomanip>
#include "logger_wrapper.h"
#include "BL_USBComm.h"
#include "bl_ctrl.h"
#include "Msg_Prot.h"
#include "interface_driver_to_ui.h"

USB_Transaction_State_t TxTransState;
USB_Transaction_State_t RxTransState;

AuxBL_MsgFbk_t BL_MSG_Null;
AuxBL_MsgFbk_t BL_MSG_NOP;
AuxBL_MsgFbk_t BL_MSG_Read_GenChecksum;
AuxBL_MsgFbk_t BL_MSG_Clear_GenSeg_Flash;
AuxBL_MsgFbk_t BL_MSG_Read_PIC_FlashMemory;
AuxBL_MsgFbk_t BL_MSG_Write_PIC_FlashMemory;
AuxBL_MsgFbk_t BL_MSG_Read_ONS_VersionNumber;
AuxBL_MsgFbk_t BL_MSG_Read_PIC_Config;
AuxBL_MsgFbk_t BL_MSG_Write_PIC_Config;
AuxBL_MsgFbk_t BL_MSG_Reset_PIC;

AuxBL_ONS_Validation_t BL_MSG_ONS_Validation;
AuxBL_ONS_Init_Status_t BL_MSG_ONS_Init;
AuxBL_ONS_ReadConfigReg_t BL_MSG_ONS_ReadConfigReg;
AuxBL_ONS_WriteConfigReg_t BL_MSG_ONS_WriteConfigReg;
AuxBL_ONS_Reset_PIC_t BL_MSG_ONS_Reset_PIC;
AuxBL_ONS_Read_SegChksum_t BL_MSG_ONS_Read_Segment_Checksum;


usb_msg_nop_fbk_t g_msg_nop_fbk;

char str_BLusblog[256];
char libusb_error_string[64];
static BL_USBComm_Driver_Elment_t USBComm_Driver_Elment = {.LIBUSB_Init_Success = -1, .LIBUSB_RxTx_Pthread_Success = -1, .USB_Device_Connect_Success = -1, .USB_TargetDevice = Device_NULL};
BL_USBComm_Driver_Idle_t USBComm_Driver_Idle;
bool g_b_USBComm_Rx_run = true;
bool g_b_USBComm_Tx_run = true;
bool g_b_USBComm_Task_run = true;
bool g_b_AutoReConnect_run = true;

// char BL_USB_WriteBuffer(void);

int BL_USBComm_Driver_Init_Threads(void)
{
	int res_rx_pth = pthread_create(&USBComm_Driver_Elment.USBComm_Rx_Thread, NULL, BL_USBComm_Rx_Thread_Main, NULL);
	int res_tx_pth = pthread_create(&USBComm_Driver_Elment.USBComm_Tx_Thread, NULL, BL_USBComm_Tx_Thread_Main, NULL);
	int res_rxser_pth = pthread_create(&USBComm_Driver_Elment.USBComm_Task_Thread, NULL, BL_USBComm_Task_Service, NULL);
	int res_autoconn_pth = pthread_create(&USBComm_Driver_Elment.USBComm_AutoReConnect_Thread, NULL, BL_USBComm_AutoReConnect, NULL);

	if (res_rx_pth != 0)
	{
		sprintf(str_BLusblog, "%s[%d] create BL_USB RxThread, errno:%d,%s", __func__, __LINE__, res_rx_pth, strerror(res_rx_pth));
		string tmp_string(str_BLusblog);
		goDriverLogger.Log("error", tmp_string);
	}
	if (res_tx_pth != 0)
	{
		sprintf(str_BLusblog, "%s[%d] create BL_USB TxThread, errno:%d,%s", __func__, __LINE__, res_tx_pth, strerror(res_tx_pth));
		string tmp_string(str_BLusblog);
		goDriverLogger.Log("error", tmp_string);
	}
	if (res_rxser_pth != 0)
	{
		sprintf(str_BLusblog, "%s[%d] create BL_USB TxThread, errno:%d,%s", __func__, __LINE__, res_rxser_pth, strerror(res_rxser_pth));
		string tmp_string(str_BLusblog);
		goDriverLogger.Log("error", tmp_string);
	}
	if (res_autoconn_pth != 0)
	{
		sprintf(str_BLusblog, "%s[%d] create BL_USB TxThread, errno:%d,%s", __func__, __LINE__, res_autoconn_pth, strerror(res_autoconn_pth));
		string tmp_string(str_BLusblog);
		goDriverLogger.Log("error", tmp_string);
	}

	BL_USBComm_Driver_Threads_Control(Threads_reset);

	if (res_rx_pth == 0 && res_tx_pth == 0 && res_rxser_pth == 0)
	{
		USBComm_Driver_Elment.LIBUSB_RxTx_Pthread_Success = 0;
		return 0;
	}
	else
		return -1;
}

int BL_USBComm_Rx_Terminate(void)
{
	g_b_USBComm_Rx_run = false;
	USBComm_Driver_Idle.g_USBComm_Rx_idle.set();

	if (pthread_join(USBComm_Driver_Elment.USBComm_Rx_Thread, NULL))
		return 0;
	else
		return -1;
}

int BL_USBComm_Tx_Terminate(void)
{
	g_b_USBComm_Tx_run = false;
	USBComm_Driver_Idle.g_USBComm_Tx_idle.set();

	if (pthread_join(USBComm_Driver_Elment.USBComm_Tx_Thread, NULL))
		return 0;
	else
		return -1;
}

int BL_USBComm_Task_Terminate(void)
{
	g_b_USBComm_Task_run = false;
	USBComm_Driver_Idle.g_USBComm_Task_idle.set();

	if (pthread_join(USBComm_Driver_Elment.USBComm_Task_Thread, NULL))
		return 0;
	else
		return -1;
}

int BL_USBComm_AutoReConnect_Terminate(void)
{
	g_b_AutoReConnect_run = false;
	if (pthread_join(USBComm_Driver_Elment.USBComm_AutoReConnect_Thread, NULL))
		return 0;
	else
		return -1;
}

int BL_USBComm_Driver_Threads_Control(BL_USBThreads_Control cmd)
{
	if (cmd == Threads_reset)
	{
		USBComm_Driver_Idle.g_USBComm_Rx_idle.reset();
		USBComm_Driver_Idle.g_USBComm_Tx_idle.reset();
		USBComm_Driver_Idle.g_USBComm_Task_idle.reset();
		// printf("USBThreads_reset\n");
	}
	else if (cmd == Threads_set)
	{
		USBComm_Driver_Idle.g_USBComm_Rx_idle.set();
		USBComm_Driver_Idle.g_USBComm_Tx_idle.set();
		USBComm_Driver_Idle.g_USBComm_Task_idle.set();
		// printf("USBThreads_set\n");
	}
	// else if( cmd == Threads_wait )
	//{
	//
	// }
	return 0;
}

int BL_USBComm_Driver_Init_LIBUSB(void)
{
	int init_res = libusb_init(&USBComm_Driver_Elment.Ctx);
	if (init_res != 0)
	{
		strcpy(libusb_error_string, libusb_strerror((libusb_error)init_res));
		sprintf(str_BLusblog, "%s[%d],errno:%d,%s", __func__, __LINE__, init_res, libusb_error_string);
		string tmp_string(str_BLusblog);
		goDriverLogger.Log("ERROR", tmp_string);
		USBComm_Driver_Elment.LIBUSB_Init_Success = -1;
	}
	else
		USBComm_Driver_Elment.LIBUSB_Init_Success = 0;

	return init_res;
}

int BL_USBComm_Driver_GetDeviceList(int *suitable_device, int en_printscr)
{
	libusb_device **usbDevices;
	libusb_device_descriptor device_desc;
	libusb_config_descriptor *config_desc;
	libusb_interface_descriptor *interface_desc;
	libusb_device_handle *usb_hdl;

	int res = -1;
	unsigned char str_iProduct[200], str_iManufacturer[200];
	int numDevices;
	int targetDevice = -1;
	int targetDevice_Select = -1;
	int r;
	bool err_trig = false;

	numDevices = libusb_get_device_list(NULL, &usbDevices);
	if (numDevices < 1)
	{
		libusb_free_device_list(usbDevices, 0);
		return -1;
	}
	for (int i = 0; i < numDevices; i++)
	{
		r = libusb_get_device_descriptor(usbDevices[i], &device_desc);
		if (r < 0)
		{
			if (en_printscr == 1)
				printf("usb_get_device_descriptor_res:%d\n", r);
			continue;
		}

		r = libusb_open(usbDevices[i], &usb_hdl);
		if (r < 0)
		{
			err_trig = true;
			if (en_printscr == 1)
				printf("libusb_open error code:%d\n", r);
			continue;
		}

		memset(str_iProduct, 0, sizeof(str_iProduct));
		memset(str_iManufacturer, 0, sizeof(str_iManufacturer));
		libusb_get_string_descriptor_ascii(usb_hdl, device_desc.iProduct, str_iProduct, 200); // this api real get usb_product info from usblib_firmware.
		libusb_get_string_descriptor_ascii(usb_hdl, device_desc.iManufacturer, str_iManufacturer, 200);
		// printf("  %d     %04X    %04X    %s,%s\n", i, device_desc.idVendor, device_desc.idProduct, str_iManufacturer, str_iProduct);

		if (en_printscr == 1)
		{
			cout << setw(4) << setfill(' ') << std::dec << i + 1 << setw(4) << "|"
				 << std::hex << std::setw(4) << setfill('0') << device_desc.idVendor << " |"
				 << std::hex << std::setw(4) << setfill('0') << device_desc.idProduct << " |"
				 << std::setw(32) << setfill(' ') << str_iManufacturer << "|" << std::setw(32) << setfill(' ') << str_iProduct << endl;
		}

		if (targetDevice == -1)
			targetDevice = BL_USBComm_Driver_SelectTargetDevice(device_desc.idVendor, device_desc.idProduct);
		libusb_close(usb_hdl);
	}

	if (en_printscr == 1 && err_trig == true)
	{
		printf("\n\nlibusb_error list:\n");
		char libusb_errorlist[]="LIBUSB_SUCCESS = 0 , LIBUSB_ERROR_IO = -1 , LIBUSB_ERROR_INVALID_PARAM = -2 , LIBUSB_ERROR_ACCESS = -3 , LIBUSB_ERROR_NO_DEVICE = -4 , LIBUSB_ERROR_NOT_FOUND = -5 , LIBUSB_ERROR_BUSY = -6 , LIBUSB_ERROR_TIMEOUT = -7 ,LIBUSB_ERROR_OVERFLOW = -8 , LIBUSB_ERROR_PIPE = -9 , LIBUSB_ERROR_INTERRUPTED = -10 , LIBUSB_ERROR_NO_MEM = -11 ,LIBUSB_ERROR_NOT_SUPPORTED = -12 , LIBUSB_ERROR_OTHER = -99";
		printf("%s\n",libusb_errorlist);

	}
	libusb_free_device_list(usbDevices, 1);
	*suitable_device = targetDevice;
	return numDevices;
}

int BL_USBComm_Driver_SelectTargetDevice(unsigned short idVendor, unsigned short idProduct)
{
	int res = -1;

	if (idVendor == USB_VID_ABC_RTC && idProduct == USB_PID_ABC_RTC)
	{
		USBComm_Driver_Elment.USB_TargetDevice = Device_ABC_RTC;
	}
	res = (int)USBComm_Driver_Elment.USB_TargetDevice;
	return res;
}

int BL_USBComm_Driver_getTargetDevice(void)
{
	return (int)USBComm_Driver_Elment.USB_TargetDevice;
}

int BL_USBComm_Driver_openTargetDevice(UsbDevice dev)
{
	int res = -1;
	if (dev == Device_ABC_RTC)
	{
		USBComm_Driver_Elment.Dev_Handle = libusb_open_device_with_vid_pid(NULL, USB_VID_ABC_RTC, USB_PID_ABC_RTC);
	}
	else
	{
		USBComm_Driver_Elment.USB_Device_Connect_Success = -1;
		return -1;
	}

	if (USBComm_Driver_Elment.Dev_Handle == NULL)
	{
		libusb_close(USBComm_Driver_Elment.Dev_Handle);
		sprintf(str_BLusblog, "%s[%d] libusb OPEN_DEVICE Fault", __func__, __LINE__);
		string tmp_string(str_BLusblog);
		goDriverLogger.Log("error", tmp_string);

		USBComm_Driver_Elment.USB_TargetDevice = Device_NULL;
	}
	else
	{
		res = libusb_kernel_driver_active(USBComm_Driver_Elment.Dev_Handle, 0);
		if (res == 1)
			res = libusb_detach_kernel_driver(USBComm_Driver_Elment.Dev_Handle, 0);
		// usually, after usb device in 'CONFIGURED_STATE'(ready to connect with libusb),
		// first time connect with device the "libusb_kernel_driver_active = 1",
		// after that, the driver restart, "libusb_kernel_driver_active = 0"
		// the check processing eventually needed.(khoo)

		res = libusb_claim_interface(USBComm_Driver_Elment.Dev_Handle, 0);

		if (res == 0)
		{
			USBComm_Driver_Elment.USB_Device_Connect_Success = 0;
			USBComm_Driver_Elment.device_EPn_IN = 0x81;
			USBComm_Driver_Elment.device_EPn_OUT = 0x01;
			if (dev == Device_ABC_RTC)
				USBComm_Driver_Elment.USB_TargetDevice = Device_ABC_RTC;
			BL_USBComm_TransStateInit();
		}
		else
			USBComm_Driver_Elment.USB_Device_Connect_Success = -1;
	}
	return res;
}

int BL_USBComm_Get_RTC_Device_Status(void)
{
	if (USBComm_Driver_Elment.USB_Device_Connect_Success == 0)
	{
		return (int)USBComm_Driver_Elment.USB_TargetDevice;
	}
	else
		return -1;
}

int BL_USBComm_Driver_closeDeviceHandle(void)
{
	if (USBComm_Driver_Elment.USB_Device_Connect_Success == 0)
	{
		libusb_close(USBComm_Driver_Elment.Dev_Handle);
		USBComm_Driver_Elment.USB_Device_Connect_Success = -1;
		return 0;
	}
	return -1;
}

void *BL_USBComm_AutoReConnect(void *p)
{
	int res;
	int trywait_TIMEOUT = 500;
	int dev_num, target_device;
	int res_open_dev;
	(void)p; // avoid compiler unused-warning
	Poco::Event wait_ms;
	pthread_detach(pthread_self());

	while (g_b_AutoReConnect_run == true)
	{
		wait_ms.tryWait(trywait_TIMEOUT);

		if (g_b_AutoReConnect_run == false)
			break;

		if (USBComm_Driver_Elment.USB_Device_Connect_Success == -1)
		{
			dev_num = BL_USBComm_Driver_GetDeviceList(&target_device, 0);
			if (target_device > 0)
			{
				res_open_dev = BL_USBComm_Driver_openTargetDevice((UsbDevice)target_device);
				//sprintf(str_BLusblog, "%s[%d] %s res:%d", __func__, __LINE__, (target_device > Device_ONS_RTC ? "BL_RTC" : "ONS_RTC"), res_open_dev);
				sprintf(str_BLusblog, "%s[%d] target_dev:%d res:%d", __func__, __LINE__, target_device, res_open_dev);

				string tmp_string(str_BLusblog);
				goDriverLogger.Log("info", tmp_string);
			}
		}
		else
			continue;
	}
	printf("BL_USBComm_AutoReConnect_Terminated\n");
	pthread_exit(NULL);
}

void *BL_USBComm_Rx_Thread_Main(void *p)
{
	int res;
	char dump_data[255];
	unsigned char rx_msg[64];
	int rx_res = 0;
	int rx_actual_size;
	int res_closeHdl;
	(void)p; // avoid compiler unused-warning

	pthread_detach(pthread_self());
	while (g_b_USBComm_Rx_run == true)
	{
		USBComm_Driver_Idle.g_USBComm_Rx_idle.wait();
		if (g_b_USBComm_Rx_run == false)
			break;

		while (1)
		{
			rx_res = libusb_bulk_transfer(USBComm_Driver_Elment.Dev_Handle, USBComm_Driver_Elment.device_EPn_IN, rx_msg, 64, &rx_actual_size, 0);
			if (rx_res == 0)
			{
				pthread_mutex_lock(&USBComm_Driver_Elment.Rx_Buffer_Mutex);
				BL_USB_FI_RxBuffer(rx_msg, rx_actual_size);
				pthread_mutex_unlock(&USBComm_Driver_Elment.Rx_Buffer_Mutex);
			}
			else
			{
				strcpy(libusb_error_string, libusb_strerror((libusb_error)rx_res));
				sprintf(str_BLusblog, "%s[%d],errno:%d,%s", __func__, __LINE__, rx_res, libusb_error_string);
				string tmp_string(str_BLusblog);
				goDriverLogger.Log("ERROR", tmp_string);
				break;
			}
			// if rx_res = -4, should be re-connect. refer to enum libusb_error.
			usleep(500);
		}

		res_closeHdl = BL_USBComm_Driver_closeDeviceHandle();
		USBComm_Driver_Elment.USB_TargetDevice = Device_NULL;

		sprintf(str_BLusblog, "%s[%d] BL_USB DevHandle close:%d, TargetDev:%d", __func__, __LINE__, res_closeHdl, USBComm_Driver_Elment.USB_TargetDevice);
		string tmp_string(str_BLusblog);
		goDriverLogger.Log("info", tmp_string);
	}
	printf("BL_USBComm_Rx_Thread_Terminated\n");
	pthread_exit(NULL);
}

void *BL_USBComm_Tx_Thread_Main(void *p)
{
	int res;
	(void)p; // avoid compiler unused-warning
	Poco::Event wait_ms;
	pthread_detach(pthread_self());

	while (g_b_USBComm_Tx_run == true)
	{
		USBComm_Driver_Idle.g_USBComm_Tx_idle.wait();
		if (g_b_USBComm_Tx_run == false)
			break;

		while (BL_USBComm_Driver_getTargetDevice() != Device_NULL)
		{
			// BL_USB_WriteBuffer();
			pthread_mutex_lock(&USBComm_Driver_Elment.Tx_Buffer_Mutex);
			res = BL_USB_FO_TxBuffer();
			pthread_mutex_unlock(&USBComm_Driver_Elment.Tx_Buffer_Mutex);
			wait_ms.tryWait(1);

			if (res != 1 && res != 0)
			{
				strcpy(libusb_error_string, libusb_strerror((libusb_error)res));
				sprintf(str_BLusblog, "%s[%d],errno:%d,%s", __func__, __LINE__, res, libusb_error_string);
				string tmp_string(str_BLusblog);
				goDriverLogger.Log("ERROR", tmp_string);
			}
		}
	}
	printf("BL_USBComm_Tx_Thread_Terminated\n");
	pthread_exit(NULL);
}

bool USB_Msg_Parser(USB_TaskResp_msg_t* task_msg);

void *BL_USBComm_Task_Service(void *p)
{
	int res;
	char dump_data[255];
	(void)p; // avoid compiler unused-warning
	Poco::Event wait_ms;

	char msg_res;
	char USB_Parser_res;
    USB_TaskResp_msg_t Task_msg;
    USB_TaskResp_msg_t* pTask_msg;
    usb_msg_u8 msg[MESSAGE_MAX];

	usb_msg_nop_fbk_t* ptr_nop_fbk;
	ptr_nop_fbk = &g_msg_nop_fbk;

	unsigned char msg_length;
    bool new_msg;
	pTask_msg = &Task_msg;

	pthread_detach(pthread_self());
	while (g_b_USBComm_Task_run == true)
	{
		USBComm_Driver_Idle.g_USBComm_Task_idle.wait();

		if (g_b_USBComm_Task_run == false)
			break;

		while (BL_USBComm_Driver_getTargetDevice() != Device_NULL)
		{
			pthread_mutex_lock(&USBComm_Driver_Elment.Rx_Buffer_Mutex);

			switch (USBComm_Driver_Elment.USB_TargetDevice)
			{
			case Device_NULL:
				printf("USB Rx_Service-Device Null\n");
				res = -1;
				break;

			case Device_ABC_RTC:
				new_msg = USB_Msg_Parser(&Task_msg);
				if (new_msg ==true)
				{
					if (Task_msg.cmd_id_rep==ABC_NOP+0x40)
					{
						ptr_nop_fbk->nop_fbk.cmd_id_rep =Task_msg.cmd_id_rep;
						ptr_nop_fbk->nop_fbk.sub_func =Task_msg.sub_func;
						memcpy(ptr_nop_fbk->nop_fbk.data, pTask_msg->data, 60);
						ptr_nop_fbk->nop_fbk_wake.set();
					}
				}
				// msg_res = USB_Msg_From_RxBuffer(msg, &msg_length);
				// if (msg_res == 0)
				// {
				// 	USB_Parser_res = BL_RxMsgParser(BL_resp);

				// 	if (USB_Parser_res == 0)
				// 	{
				// 		BL_RxMsg_WakeupSet(BL_resp);
				// 	}
				// }
				res = 0;
				break;

			default:
				sprintf(str_BLusblog, "%s[%d]Device Define UNKNOWN", __func__, __LINE__);
				string tmp_string(str_BLusblog);
				goDriverLogger.Log("error", tmp_string);
				break;
			}

			pthread_mutex_unlock(&USBComm_Driver_Elment.Rx_Buffer_Mutex);
			wait_ms.tryWait(1);
		}
	}
	printf("BL_USBComm_Task_Service_Terminated\n");
	return NULL;
}

char BL_USB_FO_RxBuffer(USB_BL_Protocol_t *msg_cmd, unsigned char *msg_size)
{
	// ddd;
	// need to modify 2 path for ONS_RTC, BL_RTC. or up to one-level.

	USB_BL_Protocol_t BL_Message;
	char *ptr_RxBuff;
	unsigned char i_size;
	ptr_RxBuff = &RxTransState.Ptr_comp;

	if (RxTransState.Init != 1)
		return -1;
	if ((RxTransState.Ptr_buff ^ RxTransState.Ptr_comp) == 0)
		return 1;

	if (RxTransState.Ptr_buff != -1 && RxTransState.Ptr_comp == -1)
	{
		RxTransState.Ptr_comp = 0;
		return -2;
	}
	printf("ptr_RxBuff_in:%d\n",*ptr_RxBuff);
	//  command id check.
	BL_Message.cmd_id = RxTransState.Buff[(unsigned char)*ptr_RxBuff][0];
	BL_Message.addr.LB = RxTransState.Buff[(unsigned char)*ptr_RxBuff][1];
	BL_Message.addr.MB = RxTransState.Buff[(unsigned char)*ptr_RxBuff][2];
	BL_Message.addr.HB = RxTransState.Buff[(unsigned char)*ptr_RxBuff][3];
	BL_Message.size = RxTransState.Buff[(unsigned char)*ptr_RxBuff][4];
	for (i_size = 0; i_size < BL_USB_DATASIZE; i_size++)
		BL_Message.data[i_size] = RxTransState.Buff[(unsigned char)*ptr_RxBuff][i_size + 5];

	*msg_cmd = BL_Message;
	*msg_size = RxTransState.MsgSize[(unsigned char)*ptr_RxBuff];

	for (int j = 0; j < *msg_size; j++)
	{
		printf("idx=%d, value:%02x\n",j,RxTransState.Buff[(unsigned char)*ptr_RxBuff][j]);
	}
	RxTransState.Ptr_comp++;
	RxTransState.Ptr_comp &= 0x07;
	return 0;
}

char BL_RxMsgParser(USB_BL_Protocol_t msg_fbk)
{
	char res = -1;
	unsigned char BL_cmd_id_shift = msg_fbk.cmd_id - BL_USB_CMD_ID_OFFSET;

	if (msg_fbk.cmd_id == AuxBL_Negative_Response)
	{
		AuxBL_AuxBL_NRC_Feedback(msg_fbk);
		return AuxBL_Negative_Response;
	}
	switch (BL_cmd_id_shift)
	{
	case AuxBL_Null:
		printf("cmd fbk:AuxBL_Null\n");
		res = 0;
		break;
	case AuxBL_NOP:
		AuxBL_NOP_Feedback(msg_fbk);
		res = 0;
		break;
	case AuxBL_Read_PIC_FlashMemory:
		AuxBL_ReadFlashMemory_Feedback(msg_fbk);
		res = 0;
		break;
	case AuxBL_Clear_GenSeg_Flash:
		AuxBL_Clear_GenFlash_Feedback(msg_fbk);
		res = 0;
		break;
	case AuxBL_Write_PIC_FlashMemory:
		AuxBL_WriteFlashMemory_Feedback(msg_fbk);
		res = 0;
		break;
	case AuxBL_Read_PIC_Config:
		AuxBL_ReadConfigReg_Feedback(msg_fbk);
		res = 0;
		break;
	case AuxBL_Write_PIC_Config:
		AuxBL_WriteConfigReg_Feedback(msg_fbk);
		res = 0;
		break;
	case AuxBL_Reset_PIC:
		AuxBL_Reset_Feedback(msg_fbk);
		res = 0;
		break;
	case AuxBL_Read_SegmentChecksum:
		AuxBL_Segment_Checksum_Feedback(msg_fbk);
		res = 0;
		break;
	case AuxBL_Read_ONS_VersionMsg:
		AuxBL_ONS_VersionNumber_Feedback(msg_fbk);
		res = 0;
		break;

	default:
		sprintf(str_BLusblog, "%s[%d]cmd_id UNKNOWN", __func__, __LINE__);
		string tmp_string(str_BLusblog);
		goDriverLogger.Log("error", tmp_string);
		break;
	}

	return res;
}

void BL_RxMsg_WakeupSet(USB_BL_Protocol_t msg_fbk)
{
	unsigned char BL_cmd_id_shift = msg_fbk.cmd_id - BL_USB_CMD_ID_OFFSET;
	switch (BL_cmd_id_shift)
	{
	case AuxBL_Null:
		BL_MSG_Null.AuxBL_MsgFbk_wakeup.set();
		break;
	case AuxBL_NOP:
		BL_MSG_NOP.AuxBL_MsgFbk_wakeup.set();
		break;
	case AuxBL_Read_PIC_FlashMemory:
		BL_MSG_Read_PIC_FlashMemory.AuxBL_MsgFbk_wakeup.set();
		break;
	case AuxBL_Clear_GenSeg_Flash:
		BL_MSG_Clear_GenSeg_Flash.AuxBL_MsgFbk_wakeup.set();
		break;
	case AuxBL_Write_PIC_FlashMemory:
		BL_MSG_Write_PIC_FlashMemory.AuxBL_MsgFbk_wakeup.set();
		break;
	case AuxBL_Read_PIC_Config:
		BL_MSG_Read_PIC_Config.AuxBL_MsgFbk_wakeup.set();
		break;
	case AuxBL_Write_PIC_Config:
		BL_MSG_Write_PIC_Config.AuxBL_MsgFbk_wakeup.set();
		break;
	case AuxBL_Reset_PIC:
		BL_MSG_Reset_PIC.AuxBL_MsgFbk_wakeup.set();
		break;
	case AuxBL_Read_SegmentChecksum:
		BL_MSG_Read_GenChecksum.AuxBL_MsgFbk_wakeup.set();
		break;
	case AuxBL_Read_ONS_VersionMsg:
		BL_MSG_Read_ONS_VersionNumber.AuxBL_MsgFbk_wakeup.set();
		break;

	default:
		sprintf(str_BLusblog, "%s[%d]WakeupSet cmd_id UNKNOWN", __func__, __LINE__);
		string tmp_string(str_BLusblog);
		goDriverLogger.Log("error", tmp_string);
		break;
	}
}

char BL_USB_FO_ONSRTC_RxBuffer(unsigned char *ons_msg)
{
	USB_BL_Protocol_t BL_Message;
	char *ptr_RxBuff;
	unsigned char i_size;
	ptr_RxBuff = &RxTransState.Ptr_comp;

	if (RxTransState.Init != 1)
		return -1;
	if ((RxTransState.Ptr_buff ^ RxTransState.Ptr_comp) == 0)
		return 1;

	if (RxTransState.Ptr_buff != -1 && RxTransState.Ptr_comp == -1)
	{
		RxTransState.Ptr_comp = 0;
		return -2;
	}
	memcpy(ons_msg, RxTransState.Buff[(unsigned char)*ptr_RxBuff], RxTransState.MsgSize[(unsigned char)*ptr_RxBuff]);
	RxTransState.Ptr_comp++;
	RxTransState.Ptr_comp &= 0x07;
	return 0;
}

char BL_ONSRTC_RxMsgParser(unsigned char *ons_msg)
{
	char res = -1;
	unsigned char msg_data[64];
	char str_RTCdebug[52];
	static int rtc_status_fbk = 0;
	Generic_Message_t *p_ons_msg;
	std::string log;

	p_ons_msg = (Generic_Message_t *)ons_msg;
	memcpy(msg_data, (p_ons_msg->Data), 52);

	switch (p_ons_msg->Message_Header.Id)
	{
	case MSG_PROT_PROTOCOL_VALIDATION_STATUS:
		Msg_Prot_Unpack_Protocol_Validation_Status(msg_data,
												   &BL_MSG_ONS_Validation.RTC_Base_Ver, &BL_MSG_ONS_Validation.RTC_Major_Ver, &BL_MSG_ONS_Validation.RTC_Minor_Ver,
												   &BL_MSG_ONS_Validation.Msg_Prot_Base, &BL_MSG_ONS_Validation.Msg_Prot_Major, &BL_MSG_ONS_Validation.Msg_Prot_Minor);
		BL_MSG_ONS_Validation.AuxBL_MsgFbk_wakeup.set();
		res = 0;
		break;

	case MSG_PROT_RTC_DEBUG_MSG:
		Msg_Prot_Unpack_Debug_Data_Msg(msg_data, str_RTCdebug);
		log = "ID_23,debug message:" + std::string(str_RTCdebug);
		goDriverLogger.Log("info", log);
		res = 0;
		break;

	case MSG_PROT_RTC_INIT_STATUS:
		Msg_Prot_Unpack_RTC_Init_Status(msg_data, &BL_MSG_ONS_Init.Status);
		BL_MSG_ONS_Init.AuxBL_MsgFbk_wakeup.set();
		res = 0;
		break;

	case MSG_PROT_RTC_STATUS:
		rtc_status_fbk++;
		rtc_status_fbk &= 0x0f;

		if (rtc_status_fbk == 0)
		{
			log = "ack Msg_ID:" + std::to_string(p_ons_msg->Message_Header.Id);
			goDriverLogger.Log("debug", log);
		}
		res = 0;
		break;

	case MSG_PROT_READ_CONFIGREG_FBK:
		AuxBL_Msg_Prot_Unpack_Read_ConfigReg(msg_data, BL_MSG_ONS_ReadConfigReg.addr, &BL_MSG_ONS_ReadConfigReg.size, BL_MSG_ONS_ReadConfigReg.data);
		BL_MSG_ONS_ReadConfigReg.AuxBL_MsgFbk_wakeup.set();
		res = 0;
		break;

	case MSG_PROT_WRITE_CONFIGREG_FBK:
		AuxBL_Msg_Prot_Unpack_Write_ConfigReg(msg_data, BL_MSG_ONS_WriteConfigReg.addr, &BL_MSG_ONS_WriteConfigReg.size, BL_MSG_ONS_WriteConfigReg.data);
		BL_MSG_ONS_WriteConfigReg.AuxBL_MsgFbk_wakeup.set();
		res = 0;
		break;

	case MSG_PROT_RESET_PIC_FBK:
		AuxBL_Msg_Prot_Unpack_Reset_PIC(msg_data, &BL_MSG_ONS_Reset_PIC.reset_pic_mode);
		BL_MSG_ONS_Reset_PIC.AuxBL_MsgFbk_wakeup.set();
		res = 0;
		break;

	case MSG_PROT_READ_SEGMENT_CHECKSUM_FBK:
		AuxBL_Msg_Prot_Unpack_Read_Segment_Checksum(msg_data, &BL_MSG_ONS_Read_Segment_Checksum.chksum);
		BL_MSG_ONS_Read_Segment_Checksum.AuxBL_MsgFbk_wakeup.set();
		res = 0;
		break;

	default:
		log = "Unknown Msg_ID:" + std::to_string(p_ons_msg->Message_Header.Id);
		goDriverLogger.Log("info", log);
		res = 0;
		break;
	}
	return res;
}

int BL_USBComm_Driver_TransData_bulk()
{
	int act_leng;
	static char k = 0;
	unsigned char test_msg[32] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	test_msg[0] = k;
	k++;
	libusb_bulk_transfer(USBComm_Driver_Elment.Dev_Handle, USBComm_Driver_Elment.device_EPn_OUT, test_msg, 32, &act_leng, 0);
	return 0;
}

int BL_USBComm_Driver_TransData_interrupt()
{
	int act_leng;
	unsigned char test_msg[32] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	libusb_interrupt_transfer(USBComm_Driver_Elment.Dev_Handle, USBComm_Driver_Elment.device_EPn_OUT, test_msg, 32, &act_leng, 0);
	return 0;
}

int testing_TransData(int i)
{
	Poco::Event wait_ms;

	for (int j = 0; j < i; j++)
	{
		BL_USBComm_Driver_TransData_bulk();
		wait_ms.tryWait(50);
		cout << "j:" << j << endl;
	}
	cout << "j_end" << endl;
	return 0;
}

/*
char BL_USBComm_PushBuffer(unsigned char* usb_msg, unsigned char msg_size)
{
	unsigned char i;

	if( TxHandleState.TxPtr_buff == -1)
	{
		TxHandleState.TxPtr_buff = 0;
		TxHandleState.TxPtr_send = 0;
	}

	for(i=0;i<msg_size;i++)
		TxHandleState.TxBuf[(unsigned char)TxHandleState.TxPtr_buff][i]=*(usb_msg+i);
	TxHandleState.TxMsgSize[(unsigned char)TxHandleState.TxPtr_buff]=msg_size;

	TxHandleState.TxPtr_buff++;
	TxHandleState.TxPtr_buff&=0x07;

	if( (TxHandleState.TxPtr_buff^TxHandleState.TxPtr_send) == 0 )
		TxHandleState.TxStuck = 1;
	return 0;
}*/

/*
char BL_USB_WriteBuffer(void)
{
	char tx_ptr=TxHandleState.TxPtr_send;
	int trans_leng;
	unsigned char tx_msg[TXMSG_MAX];
	unsigned char tx_size;

	if( TxHandleState.TxInit != 1 )
		return -1;

	if( (TxHandleState.TxPtr_buff^TxHandleState.TxPtr_send) == 0 )
		return 1;

	//if( BL_USB_TxMutex_Get() == 1 )
	//    return 2; // usb data transmit, need to wait.

	//if(  TxHandleState.TxStuck == 1 )
	//    return -2; // host usb stuck. need reset usb.

	tx_size = TxHandleState.TxMsgSize[(unsigned char)tx_ptr];
	memcpy(tx_msg, TxHandleState.TxBuf[(unsigned char)tx_ptr], tx_size);
	libusb_bulk_transfer (USBComm_Driver_Elment.Dev_Handle, USBComm_Driver_Elment.device_EPn_OUT, tx_msg , tx_size, &trans_leng, 0);

	TxHandleState.TxPtr_send++;
	TxHandleState.TxPtr_send&=0x07;
	return 0;
}*/

void BL_USBComm_TransStateInit(void)
{
	TxTransState.Init = 1;
	TxTransState.Mutex = 0;
	TxTransState.Stuck = 0;
	TxTransState.Ptr_buff = -1;
	TxTransState.Ptr_comp = -1;

	RxTransState.Init = 1;
	RxTransState.Mutex = 0;
	RxTransState.Stuck = 0;
	RxTransState.Ptr_buff = -1;
	RxTransState.Ptr_comp = -1;

	BL_USBComm_Driver_Threads_Control(Threads_set);
}

char BL_USB_FI_TxBuffer(USB_BL_Protocol_t send_msg, unsigned char msg_size) // FirstIn
{
	unsigned char i;
	char *ptr_TxBuff;
	ptr_TxBuff = &TxTransState.Ptr_buff;

	if (TxTransState.Ptr_buff == -1)
	{
		TxTransState.Ptr_buff = 0;
		TxTransState.Ptr_comp = 0;
	}
	pthread_mutex_lock(&USBComm_Driver_Elment.Tx_Buffer_Mutex);

	TxTransState.Buff[(unsigned char)*ptr_TxBuff][0] = send_msg.cmd_id;
	TxTransState.Buff[(unsigned char)*ptr_TxBuff][1] = send_msg.addr.LB;
	TxTransState.Buff[(unsigned char)*ptr_TxBuff][2] = send_msg.addr.MB;
	TxTransState.Buff[(unsigned char)*ptr_TxBuff][3] = send_msg.addr.HB;
	TxTransState.Buff[(unsigned char)*ptr_TxBuff][4] = send_msg.size;
	for (i = 0; i < BL_USB_DATASIZE; i++)
		TxTransState.Buff[(unsigned char)*ptr_TxBuff][5 + i] = send_msg.data[i];

	TxTransState.MsgSize[(unsigned char)TxTransState.Ptr_buff] = msg_size;

	TxTransState.Ptr_buff++;
	TxTransState.Ptr_buff &= 0x07;

	pthread_mutex_unlock(&USBComm_Driver_Elment.Tx_Buffer_Mutex);

	if ((TxTransState.Ptr_buff ^ TxTransState.Ptr_comp) == 0)
		TxTransState.Stuck = 1;
	return 0;
}

int BL_USB_FO_TxBuffer(void) // FirstOut
{
	char tx_ptr = TxTransState.Ptr_comp;
	unsigned char tx_size;
	unsigned char tx_msg[MESSAGE_MAX];
	int transferred;
	int tx_res;
	if (TxTransState.Init != 1)
		return -1;

	if ((TxTransState.Ptr_buff ^ TxTransState.Ptr_comp) == 0)
		return 1;

	tx_size = TxTransState.MsgSize[(unsigned char)tx_ptr];
	memcpy(tx_msg, TxTransState.Buff[(unsigned char)tx_ptr], tx_size);
	tx_res = libusb_bulk_transfer(USBComm_Driver_Elment.Dev_Handle, USBComm_Driver_Elment.device_EPn_OUT, tx_msg, (int)tx_size, &transferred, 0);

	TxTransState.Mutex = 0;
	TxTransState.Ptr_comp++;
	TxTransState.Ptr_comp &= 0x07;
	return tx_res;
}

char BL_USB_FI_RxBuffer(unsigned char rx_data[], int rx_length)
{
	int i_RxLength = 0;
	char *ptr_RxBuff;
	ptr_RxBuff = &RxTransState.Ptr_buff;

	if (USBComm_Driver_Elment.USB_Device_Connect_Success == -1)
		return -1;

	if (RxTransState.Ptr_buff == -1)
		RxTransState.Ptr_buff = 0;

	memcpy(RxTransState.Buff[(unsigned char)*ptr_RxBuff], rx_data, MESSAGE_MAX);
	RxTransState.MsgSize[(unsigned char)*ptr_RxBuff] = (unsigned char)rx_length;

	RxTransState.Ptr_buff++;
	RxTransState.Ptr_buff &= 0x07;

	return 0;
}

int AuxBL_NOP_Message(int msg_addr, int data_size, int *fbk_addr, unsigned char *fbk_datasize)
{
	int res;
	int nop_resp;
	int nop_trywait_TIMEOUT = 50;
	USB_BL_Protocol_t NopMsg;
	NopMsg.cmd_id = AuxBL_NOP;
	NopMsg.addr.dword = ADDR_MASK & (unsigned int)msg_addr;
	NopMsg.size = ((unsigned char)data_size) & 0xff;
	memset(NopMsg.data, 0, sizeof(NopMsg.data));
	AuxBL_MsgFbk_t *ptr_msg;
	ptr_msg = &BL_MSG_NOP;

	timetag_t t_auxbl_nop;
	time_tag(tag_all, &t_auxbl_nop);

	BL_MSG_NOP.AuxBL_MsgFbk_wakeup.reset();
	BL_USB_FI_TxBuffer(NopMsg, 8);

	while (1)
	{
		nop_resp = BL_MSG_NOP.AuxBL_MsgFbk_wakeup.tryWait(nop_trywait_TIMEOUT);

		if (nop_resp == 1)
		{
			*fbk_addr = ptr_msg->AuxBL_MsgFbk.addr.dword;
			*fbk_datasize = ptr_msg->AuxBL_MsgFbk.size;
			sprintf(str_BLusblog, "%s[%d] addr:0x%06X, size:%d", __func__, __LINE__, *fbk_addr, *fbk_datasize);
			string tmp_string(str_BLusblog);
			goDriverLogger.Log("info", tmp_string);
			res = 0;
		}
		else
		{
			sprintf(str_BLusblog, "%s[%d] time out", __func__, __LINE__);
			string tmp_string(str_BLusblog);
			goDriverLogger.Log("error", tmp_string);
			res = -1;
		}
		break;
	}

	time_tag(tag_delta, &t_auxbl_nop);
	if (res == -1)
		printf("AuxBL_NOP, execute time:%dms. \n", t_auxbl_nop.delta_tn);
	return res;
}

int AuxBL_NOP_Feedback(USB_BL_Protocol_t receiv_msg)
{
	BL_MSG_NOP.AuxBL_MsgFbk.cmd_id = receiv_msg.cmd_id;
	BL_MSG_NOP.AuxBL_MsgFbk.addr.dword = receiv_msg.addr.dword & ADDR_MASK;
	BL_MSG_NOP.AuxBL_MsgFbk.size = receiv_msg.size;
	return 0;
}

int AuxBL_ReadFlashMemory_Message(int msg_addr, int req_size, int *fbk_addr, unsigned char *fbk_data, unsigned char *fbk_datasize)
{
	int res = -1;
	int rdmem_resp;
	int rdmem_trywait_TIMEOUT = 50;
	USB_BL_Protocol_t RdMemMsg;
	RdMemMsg.cmd_id = AuxBL_Read_PIC_FlashMemory;
	RdMemMsg.addr.dword = ADDR_MASK & (unsigned int)msg_addr;
	RdMemMsg.size = ((unsigned char)req_size) & 0xff;
	memset(RdMemMsg.data, 0, sizeof(RdMemMsg.data));
	AuxBL_MsgFbk_t *ptr_msg;
	ptr_msg = &BL_MSG_Read_PIC_FlashMemory;

	timetag_t t_auxbl_rdmem;
	time_tag(tag_all, &t_auxbl_rdmem);

	BL_MSG_Read_PIC_FlashMemory.AuxBL_MsgFbk_wakeup.reset();
	BL_USB_FI_TxBuffer(RdMemMsg, 8);

	while (1)
	{
		rdmem_resp = BL_MSG_Read_PIC_FlashMemory.AuxBL_MsgFbk_wakeup.tryWait(rdmem_trywait_TIMEOUT);

		if (rdmem_resp == 1)
		{
			memcpy(fbk_data, BL_MSG_Read_PIC_FlashMemory.AuxBL_MsgFbk.data, BL_USB_DATASIZE);
			*fbk_addr = ptr_msg->AuxBL_MsgFbk.addr.dword;
			*fbk_datasize = ptr_msg->AuxBL_MsgFbk.size;
			sprintf(str_BLusblog, "%s[%d] addr:0x%06X, size:%d", __func__, __LINE__, *fbk_addr, *fbk_datasize);
			string tmp_string(str_BLusblog);
			goDriverLogger.Log("info", tmp_string);
			res = 0;
		}
		else
		{
			sprintf(str_BLusblog, "%s[%d] time out", __func__, __LINE__);
			string tmp_string(str_BLusblog);
			goDriverLogger.Log("error", tmp_string);
			res = -1;
		}

		break;
	}

	time_tag(tag_delta, &t_auxbl_rdmem);
	if (res == -1)
		printf("AuxBL_Read_Mem, execute time:%dms. \n", t_auxbl_rdmem.delta_tn);
	return res;
}

int AuxBL_ReadFlashMemory_Feedback(USB_BL_Protocol_t receiv_msg)
{
	int i;
	BL_MSG_Read_PIC_FlashMemory.AuxBL_MsgFbk.cmd_id = receiv_msg.cmd_id;
	BL_MSG_Read_PIC_FlashMemory.AuxBL_MsgFbk.addr.dword = receiv_msg.addr.dword & ADDR_MASK;
	BL_MSG_Read_PIC_FlashMemory.AuxBL_MsgFbk.size = receiv_msg.size;
	for (i = 0; i < BL_USB_DATASIZE; i++)
		BL_MSG_Read_PIC_FlashMemory.AuxBL_MsgFbk.data[i] = receiv_msg.data[i];
	return 0;
}

int AuxBL_Clear_GenFlash_Message(int msg_addr, int data_size, int *fbk_addr, unsigned char *fbk_datasize)
{
	int res;
	int clrgen_resp;
	int clrgen_trywait_TIMEOUT = 100;
	USB_BL_Protocol_t Clr_Gen_Msg;
	Clr_Gen_Msg.cmd_id = AuxBL_Clear_GenSeg_Flash;
	Clr_Gen_Msg.addr.dword = ADDR_MASK & (unsigned int)msg_addr;
	Clr_Gen_Msg.size = ((unsigned char)data_size) & 0xff;
	memset(Clr_Gen_Msg.data, 0, sizeof(Clr_Gen_Msg.data));
	AuxBL_MsgFbk_t *ptr_msg;
	ptr_msg = &BL_MSG_Clear_GenSeg_Flash;

	timetag_t t_auxbl_clrgen;
	time_tag(tag_all, &t_auxbl_clrgen);

	BL_MSG_Clear_GenSeg_Flash.AuxBL_MsgFbk_wakeup.reset();
	BL_USB_FI_TxBuffer(Clr_Gen_Msg, 8);

	while (1)
	{
		clrgen_resp = BL_MSG_Clear_GenSeg_Flash.AuxBL_MsgFbk_wakeup.tryWait(clrgen_trywait_TIMEOUT);
		if (clrgen_resp == 1)
		{
			*fbk_addr = ptr_msg->AuxBL_MsgFbk.addr.dword;
			*fbk_datasize = ptr_msg->AuxBL_MsgFbk.size;

			sprintf(str_BLusblog, "%s[%d] addr:0x%06X, size:%d", __func__, __LINE__, *fbk_addr, *fbk_datasize);
			string tmp_string(str_BLusblog);
			goDriverLogger.Log("info", tmp_string);
			res = 0;
		}
		else
		{
			sprintf(str_BLusblog, "%s[%d] time out", __func__, __LINE__);
			string tmp_string(str_BLusblog);
			goDriverLogger.Log("error", tmp_string);
			res = -1;
		}
		break;
	}
	time_tag(tag_delta, &t_auxbl_clrgen);
	if (res == -1)
		printf("AuxBL_Clr_GenFlash, execute time:%dms. \n", t_auxbl_clrgen.delta_tn);
	return res;
}

int AuxBL_Clear_GenFlash_Feedback(USB_BL_Protocol_t receiv_msg)
{
	BL_MSG_Clear_GenSeg_Flash.AuxBL_MsgFbk.cmd_id = receiv_msg.cmd_id;
	BL_MSG_Clear_GenSeg_Flash.AuxBL_MsgFbk.addr.dword = receiv_msg.addr.dword & ADDR_MASK;
	BL_MSG_Clear_GenSeg_Flash.AuxBL_MsgFbk.size = receiv_msg.size;
	return 0;
}

int AuxBL_WriteFlashMemory_Message(int msg_addr, int wr_size, unsigned char *wr_datamsg, int *fbk_addr, unsigned char *fbk_datasize, unsigned char *fbk_chksum)
{
	int res = -1;
	int wrmem_resp;
	int wrmem_trywait_TIMEOUT = 100;
	unsigned char wrmem_sum = 0;
	USB_BL_Protocol_t WrMemMsg;
	WrMemMsg.cmd_id = AuxBL_Write_PIC_FlashMemory;
	WrMemMsg.addr.dword = ADDR_MASK & (unsigned int)msg_addr;
	WrMemMsg.size = ((unsigned char)wr_size) & 0xff;
	memset(WrMemMsg.data, 0, sizeof(WrMemMsg.data));
	for (unsigned char i = 0; i < WrMemMsg.size; i++)
	{
		WrMemMsg.data[i] = *wr_datamsg;
		wrmem_sum += WrMemMsg.data[i];
		wr_datamsg++;
	}

	AuxBL_MsgFbk_t *ptr_msg;
	ptr_msg = &BL_MSG_Write_PIC_FlashMemory;

	timetag_t t_auxbl_wrmem;
	time_tag(tag_all, &t_auxbl_wrmem);

	BL_MSG_Write_PIC_FlashMemory.AuxBL_MsgFbk_wakeup.reset();
	BL_USB_FI_TxBuffer(WrMemMsg, 40);

	while (1)
	{
		wrmem_resp = BL_MSG_Write_PIC_FlashMemory.AuxBL_MsgFbk_wakeup.tryWait(wrmem_trywait_TIMEOUT);

		if (wrmem_resp == 1)
		{
			*fbk_addr = ptr_msg->AuxBL_MsgFbk.addr.dword;
			*fbk_datasize = ptr_msg->AuxBL_MsgFbk.size;
			*fbk_chksum = ptr_msg->AuxBL_MsgFbk.data[0] + wrmem_sum;
			// sprintf(str_BLusblog, "%s[%d]AuxBL_WriteFlashMemory fbk:[ 0x%06X, 0x%02X ]", __func__, __LINE__,*fbk_addr,*fbk_chksum);
			// string tmp_string(str_BLusblog);
			// goDriverLogger.Log("info", tmp_string);
			res = 0;
		}
		else
		{
			sprintf(str_BLusblog, "%s[%d] time out", __func__, __LINE__);
			string tmp_string(str_BLusblog);
			goDriverLogger.Log("error", tmp_string);
			res = -1;
		}

		break;
	}

	time_tag(tag_delta, &t_auxbl_wrmem);
	if (res == -1)
		printf("AuxBL_Write_Mem, execute time:%dms. \n", t_auxbl_wrmem.delta_tn);
	return res;
}

int AuxBL_WriteFlashMemory_Feedback(USB_BL_Protocol_t receiv_msg)
{
	int i;
	BL_MSG_Write_PIC_FlashMemory.AuxBL_MsgFbk.cmd_id = receiv_msg.cmd_id;
	BL_MSG_Write_PIC_FlashMemory.AuxBL_MsgFbk.addr.dword = receiv_msg.addr.dword & ADDR_MASK;
	BL_MSG_Write_PIC_FlashMemory.AuxBL_MsgFbk.size = receiv_msg.size;
	BL_MSG_Write_PIC_FlashMemory.AuxBL_MsgFbk.data[0] = receiv_msg.data[0];
	// printf("addr:%d,size:%d\n",BL_MSG_Read_PIC_FlashMemory.AuxBL_MsgFbk.addr.dword,BL_MSG_Read_PIC_FlashMemory.AuxBL_MsgFbk.size);
	return 0;
}

int AuxBL_Read_ONS_VersionNumber_Message(int offset_addr, unsigned char version_msg_sel, int *fbk_addr, unsigned char *fbk_data, unsigned char *fbk_datasize)
{
	int res = -1;
	int rd_vernum_resp;
	int rrd_vernum_trywait_TIMEOUT = 50;
	USB_BL_Protocol_t RdONSVerNumMsg;
	RdONSVerNumMsg.cmd_id = AuxBL_Read_ONS_VersionMsg;
	RdONSVerNumMsg.addr.dword = ADDR_MASK & (unsigned int)offset_addr;
	RdONSVerNumMsg.size = ((unsigned char)1) & 0xff;
	memset(RdONSVerNumMsg.data, 0, sizeof(RdONSVerNumMsg.data));
	RdONSVerNumMsg.data[0] = version_msg_sel;

	AuxBL_MsgFbk_t *ptr_msg;
	ptr_msg = &BL_MSG_Read_ONS_VersionNumber;

	timetag_t t_auxbl_rdonsver;
	time_tag(tag_all, &t_auxbl_rdonsver);

	BL_MSG_Read_ONS_VersionNumber.AuxBL_MsgFbk_wakeup.reset();
	BL_USB_FI_TxBuffer(RdONSVerNumMsg, 8);

	while (1)
	{
		rd_vernum_resp = BL_MSG_Read_ONS_VersionNumber.AuxBL_MsgFbk_wakeup.tryWait(rrd_vernum_trywait_TIMEOUT);

		if (rd_vernum_resp == 1)
		{
			memcpy(fbk_data, BL_MSG_Read_ONS_VersionNumber.AuxBL_MsgFbk.data, BL_USB_DATASIZE);
			*fbk_addr = ptr_msg->AuxBL_MsgFbk.addr.dword;
			*fbk_datasize = ptr_msg->AuxBL_MsgFbk.size;
			sprintf(str_BLusblog, "%s[%d] offset addr:0x%06X, size:%d", __func__, __LINE__, *fbk_addr, *fbk_datasize);
			string tmp_string(str_BLusblog);
			goDriverLogger.Log("info", tmp_string);
			res = 0;
		}
		else
		{
			sprintf(str_BLusblog, "%s[%d] time out", __func__, __LINE__);
			string tmp_string(str_BLusblog);
			goDriverLogger.Log("error", tmp_string);
			res = -1;
		}

		break;
	}

	time_tag(tag_delta, &t_auxbl_rdonsver);
	if (res == -1)
		printf("AuxBL_Read_ONS_VersionNumber, execute time:%dms. \n", t_auxbl_rdonsver.delta_tn);
	return res;
}

int AuxBL_ONS_VersionNumber_Feedback(USB_BL_Protocol_t receiv_msg)
{
	int i;
	BL_MSG_Read_ONS_VersionNumber.AuxBL_MsgFbk.cmd_id = receiv_msg.cmd_id;
	BL_MSG_Read_ONS_VersionNumber.AuxBL_MsgFbk.addr.dword = receiv_msg.addr.dword & ADDR_MASK;
	BL_MSG_Read_ONS_VersionNumber.AuxBL_MsgFbk.size = receiv_msg.size;
	for (i = 0; i < BL_USB_DATASIZE; i++)
		BL_MSG_Read_ONS_VersionNumber.AuxBL_MsgFbk.data[i] = receiv_msg.data[i];
	return 0;
}

int AuxBL_ReadConfigReg_Message(int msg_addr, int req_size, int *fbk_addr, unsigned char *fbk_data, unsigned char *fbk_datasize)
{
	int res = -1;
	int rdconfig_resp;
	int rdconfig_trywait_TIMEOUT = 50;
	USB_BL_Protocol_t RdConfigRegMsg;
	RdConfigRegMsg.cmd_id = AuxBL_Read_PIC_Config;
	RdConfigRegMsg.addr.dword = ADDR_MASK & (unsigned int)msg_addr;
	RdConfigRegMsg.size = ((unsigned char)req_size) & 0xff;
	memset(RdConfigRegMsg.data, 0, sizeof(RdConfigRegMsg.data));
	AuxBL_MsgFbk_t *ptr_msg;
	ptr_msg = &BL_MSG_Read_PIC_Config;

	timetag_t t_auxbl_rdconfig;
	time_tag(tag_all, &t_auxbl_rdconfig);

	BL_MSG_Read_PIC_Config.AuxBL_MsgFbk_wakeup.reset();
	BL_USB_FI_TxBuffer(RdConfigRegMsg, 8);

	while (1)
	{
		rdconfig_resp = BL_MSG_Read_PIC_Config.AuxBL_MsgFbk_wakeup.tryWait(rdconfig_trywait_TIMEOUT);
		if (rdconfig_resp == 1)
		{
			*fbk_addr = ptr_msg->AuxBL_MsgFbk.addr.dword;
			*fbk_datasize = ptr_msg->AuxBL_MsgFbk.size;
			*fbk_data = ptr_msg->AuxBL_MsgFbk.data[0];
			sprintf(str_BLusblog, "%s[%d] config reg:[ 0x%06X, 0x%02X ]", __func__, __LINE__, *fbk_addr, *fbk_data);
			string tmp_string(str_BLusblog);
			goDriverLogger.Log("info", tmp_string);

			res = 0;
		}
		else
		{
			sprintf(str_BLusblog, "%s[%d] time out", __func__, __LINE__);
			string tmp_string(str_BLusblog);
			goDriverLogger.Log("error", tmp_string);
			res = -1;
		}

		break;
	}

	time_tag(tag_delta, &t_auxbl_rdconfig);
	if (res == -1)
		printf("AuxBL_Read_PIC_Config, execute time:%dms. \n", t_auxbl_rdconfig.delta_tn);
	return res;
}

int AuxBL_ReadConfigReg_Feedback(USB_BL_Protocol_t receiv_msg)
{
	int i;
	BL_MSG_Read_PIC_Config.AuxBL_MsgFbk.cmd_id = receiv_msg.cmd_id;
	BL_MSG_Read_PIC_Config.AuxBL_MsgFbk.addr.dword = receiv_msg.addr.dword & ADDR_MASK;
	BL_MSG_Read_PIC_Config.AuxBL_MsgFbk.size = receiv_msg.size;
	for (i = 0; i < BL_USB_DATASIZE; i++)
		BL_MSG_Read_PIC_Config.AuxBL_MsgFbk.data[i] = receiv_msg.data[i];
	return 0;
}

int AuxBL_WriteConfigReg_Message(int msg_addr, int wr_size, unsigned char wr_datamsg, int *fbk_addr, unsigned char *fbk_datasize)
{
	int res = -1;
	int wrconfig_resp;
	int wrconfig_trywait_TIMEOUT = 100;
	USB_BL_Protocol_t WrConfigMsg;
	WrConfigMsg.cmd_id = AuxBL_Write_PIC_Config;
	WrConfigMsg.addr.dword = ADDR_MASK & (unsigned int)msg_addr;
	WrConfigMsg.size = ((unsigned char)wr_size) & 0xff;
	memset(WrConfigMsg.data, 0, sizeof(WrConfigMsg.data));
	WrConfigMsg.data[0] = wr_datamsg;

	AuxBL_MsgFbk_t *ptr_msg;
	ptr_msg = &BL_MSG_Write_PIC_Config;

	timetag_t t_auxbl_wrconfig;
	time_tag(tag_all, &t_auxbl_wrconfig);

	BL_MSG_Write_PIC_Config.AuxBL_MsgFbk_wakeup.reset();
	BL_USB_FI_TxBuffer(WrConfigMsg, 8);

	while (1)
	{
		wrconfig_resp = BL_MSG_Write_PIC_Config.AuxBL_MsgFbk_wakeup.tryWait(wrconfig_trywait_TIMEOUT);
		if (wrconfig_resp == 1)
		{
			*fbk_addr = ptr_msg->AuxBL_MsgFbk.addr.dword;
			*fbk_datasize = ptr_msg->AuxBL_MsgFbk.size;
			sprintf(str_BLusblog, "%s[%d] config reg:[ 0x%06X, 0x%02X ]", __func__, __LINE__, *fbk_addr, *fbk_datasize);
			string tmp_string(str_BLusblog);
			goDriverLogger.Log("info", tmp_string);
			res = 0;
		}
		else
		{
			sprintf(str_BLusblog, "%s[%d] time out", __func__, __LINE__);
			string tmp_string(str_BLusblog);
			goDriverLogger.Log("error", tmp_string);
			res = -1;
		}
		break;
	}
	time_tag(tag_delta, &t_auxbl_wrconfig);
	if (res == -1)
		printf("AuxBL_Write_PIC_Config, execute time:%dms. \n", t_auxbl_wrconfig.delta_tn);
	return res;
}

int AuxBL_WriteConfigReg_Feedback(USB_BL_Protocol_t receiv_msg)
{
	int i;
	BL_MSG_Write_PIC_Config.AuxBL_MsgFbk.cmd_id = receiv_msg.cmd_id;
	BL_MSG_Write_PIC_Config.AuxBL_MsgFbk.addr.dword = receiv_msg.addr.dword & ADDR_MASK;
	BL_MSG_Write_PIC_Config.AuxBL_MsgFbk.size = receiv_msg.size;
	return 0;
}

int AuxBL_Reset_Message(int msg_addr, int data_size, int *fbk_addr, unsigned char *fbk_datasize)
{
	int res = -1;
	int reset_resp;
	int reset_trywait_TIMEOUT = 100;
	USB_BL_Protocol_t ResetMsg;
	ResetMsg.cmd_id = AuxBL_Reset_PIC;
	ResetMsg.addr.dword = ADDR_MASK & (unsigned int)msg_addr;
	ResetMsg.size = ((unsigned char)data_size) & 0xff;
	memset(ResetMsg.data, 0, sizeof(ResetMsg.data));
	AuxBL_MsgFbk_t *ptr_msg;
	ptr_msg = &BL_MSG_Reset_PIC;

	timetag_t t_auxbl_reset;
	time_tag(tag_all, &t_auxbl_reset);

	BL_MSG_Reset_PIC.AuxBL_MsgFbk_wakeup.reset();
	BL_USB_FI_TxBuffer(ResetMsg, 8);

	while (1)
	{
		reset_resp = BL_MSG_Reset_PIC.AuxBL_MsgFbk_wakeup.tryWait(reset_trywait_TIMEOUT);
		if (reset_resp == 1)
		{
			*fbk_addr = ptr_msg->AuxBL_MsgFbk.addr.dword;
			*fbk_datasize = ptr_msg->AuxBL_MsgFbk.size;
			sprintf(str_BLusblog, "%s[%d] addr:[ 0x%06X ]", __func__, __LINE__, *fbk_addr);
			string tmp_string(str_BLusblog);
			goDriverLogger.Log("info", tmp_string);
			res = 0;
		}
		else
		{
			sprintf(str_BLusblog, "%s[%d] time out", __func__, __LINE__);
			string tmp_string(str_BLusblog);
			goDriverLogger.Log("error", tmp_string);
			res = -1;
		}
		break;
	}
	time_tag(tag_delta, &t_auxbl_reset);
	if (res == -1)
		printf("AuxBL_Reset_PIC, execute time:%dms. \n", t_auxbl_reset.delta_tn);
	return res;
}

int AuxBL_Reset_Feedback(USB_BL_Protocol_t receiv_msg)
{
	int i;
	BL_MSG_Reset_PIC.AuxBL_MsgFbk.cmd_id = receiv_msg.cmd_id;
	BL_MSG_Reset_PIC.AuxBL_MsgFbk.addr.dword = receiv_msg.addr.dword & ADDR_MASK;
	BL_MSG_Reset_PIC.AuxBL_MsgFbk.size = receiv_msg.size;
	return 0;
}

int AuxBL_Read_Segment_Checksum_Message(unsigned char segment_sel, unsigned short *fbk_chksum)
{
	int res = -1;
	int rd_seg_chksum_resp;
	int rd_seg_chksum_trywait_TIMEOUT = 300;
	unsigned short res_chksum;
	USB_BL_Protocol_t Rd_Seg_Chksum_Msg;
	Rd_Seg_Chksum_Msg.cmd_id = AuxBL_Read_SegmentChecksum;
	Rd_Seg_Chksum_Msg.addr.dword = ADDR_MASK;
	Rd_Seg_Chksum_Msg.size = 1;
	memset(Rd_Seg_Chksum_Msg.data, 0, sizeof(Rd_Seg_Chksum_Msg.data));
	Rd_Seg_Chksum_Msg.data[0] = segment_sel;
	AuxBL_MsgFbk_t *ptr_msg;
	ptr_msg = &BL_MSG_Read_GenChecksum;

	timetag_t t_auxbl_rd_seg_chksum;
	time_tag(tag_all, &t_auxbl_rd_seg_chksum);

	BL_MSG_Read_GenChecksum.AuxBL_MsgFbk_wakeup.reset();
	BL_USB_FI_TxBuffer(Rd_Seg_Chksum_Msg, 8);

	while (1)
	{
		rd_seg_chksum_resp = BL_MSG_Read_GenChecksum.AuxBL_MsgFbk_wakeup.tryWait(rd_seg_chksum_trywait_TIMEOUT);
		if (rd_seg_chksum_resp == 1)
		{
			res_chksum = 0x00ff & (ptr_msg->AuxBL_MsgFbk.data[0]);
			res_chksum |= 0xff00 & (ptr_msg->AuxBL_MsgFbk.data[1] << 8);
			*fbk_chksum = res_chksum;
			sprintf(str_BLusblog, "%s[%d] chksum:0x%04x", __func__, __LINE__, *fbk_chksum);
			string tmp_string(str_BLusblog);
			goDriverLogger.Log("info", tmp_string);
			res = 0;
		}
		else
		{
			sprintf(str_BLusblog, "%s[%d] time out", __func__, __LINE__);
			string tmp_string(str_BLusblog);
			goDriverLogger.Log("error", tmp_string);
			*fbk_chksum = 0xffff;
			res = -1;
		}
		break;
	}
	time_tag(tag_delta, &t_auxbl_rd_seg_chksum);
	if (res == -1)
		printf("AuxBL_Read_PIC_Segment_Checksum, execute time:%dms. \n", t_auxbl_rd_seg_chksum.delta_tn);
	return res;
}

int AuxBL_Segment_Checksum_Feedback(USB_BL_Protocol_t receiv_msg)
{
	int i;
	BL_MSG_Read_GenChecksum.AuxBL_MsgFbk.cmd_id = receiv_msg.cmd_id;
	BL_MSG_Read_GenChecksum.AuxBL_MsgFbk.addr.dword = receiv_msg.addr.dword & ADDR_MASK;
	BL_MSG_Read_GenChecksum.AuxBL_MsgFbk.size = receiv_msg.size;
	for (i = 0; i < BL_USB_DATASIZE; i++)
		BL_MSG_Read_GenChecksum.AuxBL_MsgFbk.data[i] = receiv_msg.data[i];
	return 0;
}

int AuxBL_AuxBL_NRC_Feedback(USB_BL_Protocol_t receiv_msg)
{
	int i;
	printf("NagativeResponse: cmd_id[0x%02x]\n", receiv_msg.data[0]);

	if (receiv_msg.data[1] == AuxBL_NRC_SIZE_ZERO)
		printf("NRC: %s\n", "AuxBL_NRC_SIZE_ZERO");
	else if (receiv_msg.data[1] == AuxBL_NRC_SIZE_nZERO)
		printf("NRC: %s\n", "AuxBL_NRC_SIZE_nZERO");
	else if (receiv_msg.data[1] == AuxBL_NRC_SIZE_EXCEED)
		printf("NRC: %s\n", "AuxBL_NRC_SIZE_EXCEED");
	else if (receiv_msg.data[1] == AuxBL_NRC_ADDR_OUTRANGE)
		printf("NRC: %s\n", "AuxBL_NRC_ADDR_OUTRANGE");
	return 0;
}

// ONS protocol
int AuxBL_Msg_Prot_Pack_Validation_Req(void)
{
	AuxBL_Msg_Prot_Protocol_Validation_Req_Msg_t protocol_validation_msg;
	protocol_validation_msg.Msg_Hdr.Id = MSG_PROT_PROTOCOL_VALIDATION_REQ;
	protocol_validation_msg.Msg_Hdr.Serial_Number = 0x5500;
	protocol_validation_msg.Msg_Hdr.Size = sizeof(protocol_validation_msg.Msg_Hdr);
	protocol_validation_msg.Msg_Hdr.Time_Stamp = BL_USB_GetCurrentTime();
	return (int)BL_USB_FI_TxBuffer_ONS_Protocol((unsigned char *)&protocol_validation_msg, 12);
}

int AuxBL_Msg_Prot_Pack_Preform_INIT_Req(void)
{
	AuxBL_Msg_Prot_Perform_Init_Req_Msg_t rtc_init_msg;
	rtc_init_msg.Msg_Hdr.Id = MSG_PROT_PERFORM_INIT_REQ;
	rtc_init_msg.Msg_Hdr.Serial_Number = 0x5501;
	rtc_init_msg.Msg_Hdr.Size = sizeof(rtc_init_msg.Msg_Hdr);
	rtc_init_msg.Msg_Hdr.Time_Stamp = BL_USB_GetCurrentTime();
	return (int)BL_USB_FI_TxBuffer_ONS_Protocol((unsigned char *)&rtc_init_msg, 12);
}

int AuxBL_Msg_Prot_Pack_Read_ConfigReg_Req(unsigned int configreg_addr, unsigned char size)
{
	unsigned int addr_temp = configreg_addr;
	unsigned char data_blk[64];
	unsigned char *p_msg;

	AuxBL_Msg_Prot_Read_ConfigReg_Msg_t read_config_reg_msg;
	p_msg = (unsigned char *)&read_config_reg_msg;

	read_config_reg_msg.Msg_Hdr.Id = MSG_PROT_READ_CONFIGREG_CMD;
	read_config_reg_msg.Msg_Hdr.Serial_Number = 0x5502;
	read_config_reg_msg.Msg_Hdr.Size = sizeof(read_config_reg_msg.Msg_Hdr);
	read_config_reg_msg.Msg_Hdr.Time_Stamp = BL_USB_GetCurrentTime();
	read_config_reg_msg.addr.dword = configreg_addr & ADDR_MASK;
	read_config_reg_msg.size = size & 0x0f;

	memcpy(data_blk, p_msg, sizeof(read_config_reg_msg.Msg_Hdr));
	memcpy(data_blk + sizeof(read_config_reg_msg.Msg_Hdr), p_msg + sizeof(read_config_reg_msg.Msg_Hdr), sizeof(read_config_reg_msg.addr.byte));
	memcpy(data_blk + sizeof(read_config_reg_msg.Msg_Hdr) + sizeof(read_config_reg_msg.addr.byte), p_msg + sizeof(read_config_reg_msg.Msg_Hdr) + sizeof(read_config_reg_msg.addr.dword), 1);
	return (int)BL_USB_FI_TxBuffer_ONS_Protocol(data_blk, 16);
}

int AuxBL_Msg_Prot_Unpack_Read_ConfigReg(ONS_BYTE *Msg_Buff, unsigned char *fbk_addr, unsigned char *fbk_size, unsigned char *fbk_data)
{
	AuxBL_ONS_ReadConfigReg_t *msg_p;
	msg_p = (AuxBL_ONS_ReadConfigReg_t *)Msg_Buff;

	//*fbk_addr = msg_p->addr[0];
	//*fbk_addr+1 = msg_p->addr;
	//*fbk_addr+2 = msg_p->addr[2];
	memcpy(fbk_addr, msg_p, 3);
	*fbk_size = msg_p->size;
	*fbk_data = msg_p->data[0];
	return MSG_PROT_SUCCESS;
}

int AuxBL_Msg_Prot_Pack_Write_ConfigReg_Req(unsigned int configreg_addr, unsigned char size, unsigned char data)
{
	unsigned int addr_temp = configreg_addr;
	unsigned char data_blk[64];
	unsigned char *p_msg;

	AuxBL_Msg_Prot_Write_ConfigReg_Msg_t write_config_reg_msg;
	p_msg = (unsigned char *)&write_config_reg_msg;

	write_config_reg_msg.Msg_Hdr.Id = MSG_PROT_WRITE_CONFIGREG_CMD;
	write_config_reg_msg.Msg_Hdr.Serial_Number = 0x5503;
	write_config_reg_msg.Msg_Hdr.Size = sizeof(write_config_reg_msg.Msg_Hdr);
	write_config_reg_msg.Msg_Hdr.Time_Stamp = BL_USB_GetCurrentTime();
	write_config_reg_msg.addr.dword = configreg_addr & ADDR_MASK;
	write_config_reg_msg.size = size & 0x0f;
	write_config_reg_msg.data[0] = data & 0xff;
	write_config_reg_msg.data[1] = 0x00;
	write_config_reg_msg.data[2] = 0x00;
	write_config_reg_msg.data[3] = 0x00;

	memcpy(data_blk, p_msg, sizeof(write_config_reg_msg.Msg_Hdr));
	memcpy(data_blk + sizeof(write_config_reg_msg.Msg_Hdr), p_msg + sizeof(write_config_reg_msg.Msg_Hdr), sizeof(write_config_reg_msg.addr.byte));
	memcpy(data_blk + sizeof(write_config_reg_msg.Msg_Hdr) + sizeof(write_config_reg_msg.addr.byte), p_msg + sizeof(write_config_reg_msg.Msg_Hdr) + sizeof(write_config_reg_msg.addr.dword), 1);
	memcpy(data_blk + sizeof(write_config_reg_msg.Msg_Hdr) + sizeof(write_config_reg_msg.addr.byte) + 1, p_msg + sizeof(write_config_reg_msg.Msg_Hdr) + sizeof(write_config_reg_msg.addr.dword) + 1, 4);
	return (int)BL_USB_FI_TxBuffer_ONS_Protocol(data_blk, 20);
}

int AuxBL_Msg_Prot_Unpack_Write_ConfigReg(ONS_BYTE *Msg_Buff, unsigned char *fbk_addr, unsigned char *fbk_size, unsigned char *fbk_data)
{
	AuxBL_ONS_ReadConfigReg_t *msg_p;
	msg_p = (AuxBL_ONS_ReadConfigReg_t *)Msg_Buff;

	memcpy(fbk_addr, msg_p, 3);
	*fbk_size = msg_p->size;
	*fbk_data = msg_p->data[0];
	return MSG_PROT_SUCCESS;
}

int AuxBL_Msg_Prot_Pack_Reset_PIC_Req(unsigned char reset_mode)
{
	// reset_mode = 0, reset pic directly.
	// reset_mode = 1, check and write configuration bit before reset.
	unsigned char data_blk[64];
	unsigned char *p_msg;
	memset(data_blk, 0, sizeof(data_blk));
	AuxBL_Msg_Prot_Reset_PIC_Msg_t reset_pic_msg;
	p_msg = (unsigned char *)&reset_pic_msg;

	reset_pic_msg.Msg_Hdr.Id = MSG_PROT_RESET_PIC_CMD;
	reset_pic_msg.Msg_Hdr.Serial_Number = 0x5503;
	reset_pic_msg.Msg_Hdr.Size = sizeof(reset_pic_msg.Msg_Hdr);
	reset_pic_msg.Msg_Hdr.Time_Stamp = BL_USB_GetCurrentTime();
	reset_pic_msg.reset_mode = reset_mode;

	memcpy(data_blk, p_msg, sizeof(reset_pic_msg.Msg_Hdr));
	memcpy(data_blk + sizeof(reset_pic_msg.Msg_Hdr), p_msg + sizeof(reset_pic_msg.Msg_Hdr), 1);
	return (int)BL_USB_FI_TxBuffer_ONS_Protocol(data_blk, 16);
}

int AuxBL_Msg_Prot_Unpack_Reset_PIC(ONS_BYTE *Msg_Buff, unsigned char *reset_mode)
{
	unsigned char temp;
	AuxBL_ONS_Reset_PIC_t *msg_p;
	msg_p = (AuxBL_ONS_Reset_PIC_t *)Msg_Buff;
	temp = msg_p->reset_pic_mode;
	temp = temp - 0x40;
	*reset_mode = temp;
	return MSG_PROT_SUCCESS;
}

int AuxBL_Msg_Prot_Pack_Read_Segment_Checksum_Req(unsigned char segment_sel)
{
	unsigned char data_blk[64];
	unsigned char *p_msg;
	memset(data_blk, 0, sizeof(data_blk));
	AuxBL_Msg_Prot_Read_SegChksum_Msg_t read_chksum_msg;
	p_msg = (unsigned char *)&read_chksum_msg;

	read_chksum_msg.Msg_Hdr.Id = MSG_PROT_READ_SEGMENT_CHECKSUM_CMD;
	read_chksum_msg.Msg_Hdr.Serial_Number = 0x5502;
	read_chksum_msg.Msg_Hdr.Size = sizeof(read_chksum_msg.Msg_Hdr);
	read_chksum_msg.Msg_Hdr.Time_Stamp = BL_USB_GetCurrentTime();
	read_chksum_msg.segment_sel = segment_sel;

	memcpy(data_blk, p_msg, sizeof(read_chksum_msg.Msg_Hdr));
	memcpy(data_blk + sizeof(read_chksum_msg.Msg_Hdr), p_msg + sizeof(read_chksum_msg.Msg_Hdr), 2);
	return (int)BL_USB_FI_TxBuffer_ONS_Protocol(data_blk, 16);
}

int AuxBL_Msg_Prot_Unpack_Read_Segment_Checksum(ONS_BYTE *Msg_Buff, unsigned short *fbk_chksum)
{
	AuxBL_ONS_Read_SegChksum_t *msg_p;
	msg_p = (AuxBL_ONS_Read_SegChksum_t *)Msg_Buff;
	*fbk_chksum = msg_p->chksum;
	return MSG_PROT_SUCCESS;
}

unsigned long BL_USB_GetCurrentTime(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (unsigned long)((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
}

char BL_USB_FI_TxBuffer_ONS_Protocol(unsigned char *send_msg, unsigned char msg_size) // FirstIn
{
	unsigned char i;
	char *ptr_TxBuff;
	ptr_TxBuff = &TxTransState.Ptr_buff;

	if (TxTransState.Ptr_buff == -1)
	{
		TxTransState.Ptr_buff = 0;
		TxTransState.Ptr_comp = 0;
	}
	pthread_mutex_lock(&USBComm_Driver_Elment.Tx_Buffer_Mutex);

	// memcpy(USBComm_Tx_Buffer[Tx_Write_To_Buffer_Index], Message, Message_Size); //Access Buffer
	memcpy(TxTransState.Buff[(unsigned char)*ptr_TxBuff], send_msg, (size_t)msg_size); // Access Buffer
	TxTransState.MsgSize[(unsigned char)TxTransState.Ptr_buff] = msg_size;

	TxTransState.Ptr_buff++;
	TxTransState.Ptr_buff &= 0x07;

	pthread_mutex_unlock(&USBComm_Driver_Elment.Tx_Buffer_Mutex);

	if ((TxTransState.Ptr_buff ^ TxTransState.Ptr_comp) == 0)
		TxTransState.Stuck = 1;
	return 0;
}

int AuxBL_ONS_VerionReq(int *rtc_base, int *rtc_major, int *rtc_minor, int *prot_base, int *prot_major, int *prot_minor)
{
	int res = -1;
	int ver_resp;
	int ver_trywait_TIMEOUT = 50;
	timetag_t t_auxbl_onsver;
	time_tag(tag_all, &t_auxbl_onsver);

	BL_MSG_ONS_Validation.AuxBL_MsgFbk_wakeup.reset();
	AuxBL_Msg_Prot_Pack_Validation_Req();

	while (1)
	{
		ver_resp = BL_MSG_ONS_Validation.AuxBL_MsgFbk_wakeup.tryWait(ver_trywait_TIMEOUT);
		if (ver_resp == 1)
		{
			*rtc_base = BL_MSG_ONS_Validation.RTC_Base_Ver;
			*rtc_major = BL_MSG_ONS_Validation.RTC_Major_Ver;
			*rtc_minor = BL_MSG_ONS_Validation.RTC_Minor_Ver;
			*prot_base = BL_MSG_ONS_Validation.Msg_Prot_Base;
			*prot_major = BL_MSG_ONS_Validation.Msg_Prot_Major;
			*prot_minor = BL_MSG_ONS_Validation.Msg_Prot_Minor;

			sprintf(str_BLusblog, "%s[%d] [%d,%d,%d], [%d,%d,%d]", __func__, __LINE__, *rtc_base, *rtc_major, *rtc_minor, *prot_base, *prot_major, *prot_minor);
			string tmp_string(str_BLusblog);
			goDriverLogger.Log("info", tmp_string);
			res = 0;
		}
		else
		{
			sprintf(str_BLusblog, "%s[%d] time out", __func__, __LINE__);
			string tmp_string(str_BLusblog);
			goDriverLogger.Log("info", tmp_string);
			res = -1;
		}
		break;
	}

	time_tag(tag_delta, &t_auxbl_onsver);
	if (res == -1)
		printf("AuxBL_ONS_Validation, execute time:%dms. \n", t_auxbl_onsver.delta_tn);
	return res;
}

int AuxBL_ONS_InitReq(int *init_status)
{
	int res = -1;
	int init_resp;
	int init_trywait_TIMEOUT = 50;
	timetag_t t_auxbl_onsinit;
	time_tag(tag_all, &t_auxbl_onsinit);

	BL_MSG_ONS_Init.AuxBL_MsgFbk_wakeup.reset();
	AuxBL_Msg_Prot_Pack_Preform_INIT_Req();

	while (1)
	{
		init_resp = BL_MSG_ONS_Init.AuxBL_MsgFbk_wakeup.tryWait(init_trywait_TIMEOUT);
		if (init_resp == 1)
		{
			*init_status = (int)BL_MSG_ONS_Init.Status;
			sprintf(str_BLusblog, "%s[%d] status:%d", __func__, __LINE__, *init_status);
			string tmp_string(str_BLusblog);
			goDriverLogger.Log("info", tmp_string);
			res = 0;
		}
		else
		{
			sprintf(str_BLusblog, "%s[%d]  time out", __func__, __LINE__);
			string tmp_string(str_BLusblog);
			goDriverLogger.Log("info", tmp_string);
			res = -1;
		}
		break;
	}

	time_tag(tag_delta, &t_auxbl_onsinit);
	if (res == -1)
		printf("AuxBL_ONS_InitReq, execute time:%dms. \n", t_auxbl_onsinit.delta_tn);
	return res;
}

int AuxBL_ONS_Read_ConfigReg_Req(unsigned int Req_Addr, unsigned int *Fbk_Addr, unsigned char *FbkVal)
{
	int res = -1;
	int rdconfig_resp;
	int rdconfig_trywait_TIMEOUT = 50;
	timetag_t t_auxbl_rdconfig;
	time_tag(tag_all, &t_auxbl_rdconfig);
	unsigned int fbk_addr = 0;

	BL_MSG_ONS_ReadConfigReg.AuxBL_MsgFbk_wakeup.reset();
	AuxBL_Msg_Prot_Pack_Read_ConfigReg_Req(Req_Addr, 1);

	while (1)
	{
		rdconfig_resp = BL_MSG_ONS_ReadConfigReg.AuxBL_MsgFbk_wakeup.tryWait(rdconfig_trywait_TIMEOUT);
		if (rdconfig_resp == 1)
		{
			fbk_addr = 0x000000ff & (unsigned int)BL_MSG_ONS_ReadConfigReg.addr[0];
			fbk_addr |= 0x0000ff00 & ((unsigned int)BL_MSG_ONS_ReadConfigReg.addr[1] << 8);
			fbk_addr |= 0x00ff0000 & ((unsigned int)BL_MSG_ONS_ReadConfigReg.addr[2] << 16);
			*Fbk_Addr = fbk_addr;
			*FbkVal = BL_MSG_ONS_ReadConfigReg.data[0];

			sprintf(str_BLusblog, "%s[%d] config reg:[ 0x%06X, 0x%02X ]", __func__, __LINE__, *Fbk_Addr, *FbkVal);
			string tmp_string(str_BLusblog);
			goDriverLogger.Log("info", tmp_string);
			res = 0;
		}
		else
		{
			sprintf(str_BLusblog, "%s[%d] time out", __func__, __LINE__);
			string tmp_string(str_BLusblog);
			goDriverLogger.Log("info", tmp_string);
			res = -1;
		}
		break;
	}

	time_tag(tag_delta, &t_auxbl_rdconfig);
	if (res == -1)
		printf("AuxBL_ONS_Read_ConfigReg_Req, execute time:%dms. \n", t_auxbl_rdconfig.delta_tn);
	return res;
}

int AuxBL_ONS_Write_ConfigReg_Req(unsigned int Req_Addr, unsigned char ConfigByte, unsigned int *Fbk_Addr, unsigned char *FbkVal)
{
	int res = -1;
	int wrconfig_resp;
	int wrconfig_trywait_TIMEOUT = 50;
	timetag_t t_auxbl_wrconfig;
	time_tag(tag_all, &t_auxbl_wrconfig);
	unsigned int fbk_addr = 0;

	BL_MSG_ONS_WriteConfigReg.AuxBL_MsgFbk_wakeup.reset();
	AuxBL_Msg_Prot_Pack_Write_ConfigReg_Req(Req_Addr, 1, ConfigByte);

	while (1)
	{
		wrconfig_resp = BL_MSG_ONS_WriteConfigReg.AuxBL_MsgFbk_wakeup.tryWait(wrconfig_trywait_TIMEOUT);
		if (wrconfig_resp == 1)
		{
			fbk_addr = 0x000000ff & (unsigned int)BL_MSG_ONS_WriteConfigReg.addr[0];
			fbk_addr |= 0x0000ff00 & ((unsigned int)BL_MSG_ONS_WriteConfigReg.addr[1] << 8);
			fbk_addr |= 0x00ff0000 & ((unsigned int)BL_MSG_ONS_WriteConfigReg.addr[2] << 16);
			*Fbk_Addr = fbk_addr;
			*FbkVal = BL_MSG_ONS_WriteConfigReg.data[0];

			sprintf(str_BLusblog, "%s[%d] config reg:[ 0x%06X, 0x%02X ]", __func__, __LINE__, *Fbk_Addr, *FbkVal);
			string tmp_string(str_BLusblog);
			goDriverLogger.Log("info", tmp_string);
			res = 0;
		}
		else
		{
			sprintf(str_BLusblog, "%s[%d] time out", __func__, __LINE__);
			string tmp_string(str_BLusblog);
			goDriverLogger.Log("info", tmp_string);
			res = -1;
		}
		break;
	}

	time_tag(tag_delta, &t_auxbl_wrconfig);
	if (res == -1)
		printf("AuxBL_ONS_Write_ConfigReg_Req, execute time:%dms. \n", t_auxbl_wrconfig.delta_tn);
	return res;
}

int AuxBL_ONS_Reset_PIC_Req(unsigned char reset_mode, unsigned char *Fbk_reset_mode)
{
	int res = -1;
	int resetpic_resp;
	int resetpic_trywait_TIMEOUT = 50;
	timetag_t t_auxbl_resetpic;
	time_tag(tag_all, &t_auxbl_resetpic);

	BL_MSG_ONS_Reset_PIC.AuxBL_MsgFbk_wakeup.reset();
	AuxBL_Msg_Prot_Pack_Reset_PIC_Req(reset_mode);

	while (1)
	{
		resetpic_resp = BL_MSG_ONS_Reset_PIC.AuxBL_MsgFbk_wakeup.tryWait(resetpic_trywait_TIMEOUT);
		if (resetpic_resp == 1)
		{
			*Fbk_reset_mode = BL_MSG_ONS_Reset_PIC.reset_pic_mode;

			sprintf(str_BLusblog, "%s[%d] reset mode:%d", __func__, __LINE__, *Fbk_reset_mode);
			string tmp_string(str_BLusblog);
			goDriverLogger.Log("info", tmp_string);
			res = 0;
		}
		else
		{
			sprintf(str_BLusblog, "%s[%d] time out", __func__, __LINE__);
			string tmp_string(str_BLusblog);
			goDriverLogger.Log("info", tmp_string);
			res = -1;
		}
		break;
	}

	time_tag(tag_delta, &t_auxbl_resetpic);
	if (res == -1)
		printf("AuxBL_ONS_Reset_PIC_Req, execute time:%dms. \n", t_auxbl_resetpic.delta_tn);
	return res;
}

int AuxBL_ONS_Segment_Checksum_Req(unsigned char segment_sel, unsigned short *fbk_chksum)
{
	int res = -1;
	int rd_chksum_resp;
	int rd_chksum_trywait_TIMEOUT = 200;
	timetag_t t_auxbl_rdchksum;
	time_tag(tag_all, &t_auxbl_rdchksum);

	BL_MSG_ONS_Read_Segment_Checksum.AuxBL_MsgFbk_wakeup.reset();
	AuxBL_Msg_Prot_Pack_Read_Segment_Checksum_Req(segment_sel);

	while (1)
	{
		rd_chksum_resp = BL_MSG_ONS_Read_Segment_Checksum.AuxBL_MsgFbk_wakeup.tryWait(rd_chksum_trywait_TIMEOUT);
		if (rd_chksum_resp == 1)
		{
			*fbk_chksum = BL_MSG_ONS_Read_Segment_Checksum.chksum;

			sprintf(str_BLusblog, "%s[%d] checksum: 0x%04X", __func__, __LINE__, *fbk_chksum);
			string tmp_string(str_BLusblog);
			goDriverLogger.Log("info", tmp_string);
			res = 0;
		}
		else
		{
			sprintf(str_BLusblog, "%s[%d] time out", __func__, __LINE__);
			string tmp_string(str_BLusblog);
			goDriverLogger.Log("info", tmp_string);
			res = -1;
		}
		break;
	}
	time_tag(tag_delta, &t_auxbl_rdchksum);
	if (res == -1)
		printf("AuxBL_ONS_Read_Chksum_Req, execute time:%dms. \n", t_auxbl_rdchksum.delta_tn);
	return res;
}

char USB_Msg_To_TxBulkBuffer(ptr_usb_msg_u8 send_msg, unsigned char msg_size) // FirstIn
{
    char *ptr_TxBuff;
    ptr_TxBuff = &TxTransState.Ptr_buff;

    if (TxTransState.Ptr_buff == -1)
    {
        TxTransState.Ptr_buff = 0;
        TxTransState.Ptr_comp = 0;
    }
	pthread_mutex_lock(&USBComm_Driver_Elment.Tx_Buffer_Mutex);
    memcpy(TxTransState.Buff[(unsigned char)TxTransState.Ptr_buff], send_msg, msg_size);
    TxTransState.MsgSize[(unsigned char)TxTransState.Ptr_buff] = msg_size;
    TxTransState.Ptr_buff++;
    TxTransState.Ptr_buff &= 0x07;
	pthread_mutex_unlock(&USBComm_Driver_Elment.Tx_Buffer_Mutex);

    if ((TxTransState.Ptr_buff ^ TxTransState.Ptr_comp) == 0)
        TxTransState.Stuck = 1;
    return 0;
}

int msg_nop(unsigned char sub_func)
{
	int res;
	int res_wake;
	int nop_trywait_TIMEOUT = 50;
	usb_msg_nop_t msg_nop;
	msg_nop.cmd_id = ABC_NOP;
	msg_nop.sub_func = sub_func;
	memset(msg_nop.data, 0, sizeof(msg_nop.data));
	usb_msg_nop_fbk_t* ptr_nop_fbk;
	ptr_nop_fbk = &g_msg_nop_fbk;
	timetag_t t_nop;
	time_tag(tag_all, &t_nop);
	ptr_nop_fbk->nop_fbk_wake.reset();
	USB_Msg_To_TxBulkBuffer((ptr_usb_msg_u8)&msg_nop, 8);
	while (1)
	{
		res_wake = ptr_nop_fbk->nop_fbk_wake.tryWait(nop_trywait_TIMEOUT);
		if (res_wake == 1)
		{
			sprintf(str_BLusblog, "%s[%d] resp_id:0x%02x, sub_func:0x%02x,data:%s", __func__, __LINE__, 
				ptr_nop_fbk->nop_fbk.cmd_id_rep,ptr_nop_fbk->nop_fbk.sub_func,ptr_nop_fbk->nop_fbk.data);
			string tmp_string(str_BLusblog);
			goDriverLogger.Log("info", tmp_string);
			res = 0;
		}
		else
		{
			sprintf(str_BLusblog, "%s[%d] time out", __func__, __LINE__);
			string tmp_string(str_BLusblog);
			goDriverLogger.Log("error", tmp_string);
			res = -1;
		}
		break;
	}
	time_tag(tag_delta, &t_nop);
	if (res == -1)
		printf("ABC_NOP, execute time:%dms. \n", t_nop.delta_tn);
	return res;
}

bool USB_Msg_Parser(USB_TaskResp_msg_t *task_msg)
{
	usb_msg_u8 msg[MESSAGE_MAX];
	USB_TaskResp_msg_t *p_taskmsg;
	USB_NegResponse_msg_t *p_negresp;
	unsigned char msg_length;
	bool b_new_msg = false;
	char nrc_res;
	char msg_res;
	p_taskmsg = (USB_TaskResp_msg_t *)msg;
	p_negresp = (USB_NegResponse_msg_t *)msg;
	msg_res = USB_Msg_From_RxBuffer(msg, &msg_length);
	if (msg_res == 0)
	{
		if (p_taskmsg->cmd_id_rep == ABC_Negative_Response)
		{
			sprintf(str_BLusblog, "%s[%d] cmd_id=0x%02x neg_code=0x%02x, msg:%s",
					__func__, __LINE__, p_negresp->cmd_id, p_negresp->neg_code, p_negresp->data);
			string tmp_string(str_BLusblog);
			goDriverLogger.Log("info", tmp_string);
		}
		else
		{
			if (p_taskmsg->cmd_id_rep == ABC_NOP + 0x40)
			{
				memcpy(task_msg, (usb_msg_u8 *)msg, MESSAGE_MAX);
				b_new_msg = true;
			}
		}
	}
	return b_new_msg;
}

char USB_Msg_From_RxBuffer(usb_msg_u8 *msg_cmd, unsigned char *msg_size)
{
    char res = -1;
    char *ptr_RxBuff;
    ptr_RxBuff = &RxTransState.Ptr_comp;

    if (RxTransState.Init != 1)
    {
        res = -1;
        return res;
    }
    if ((RxTransState.Ptr_buff ^ RxTransState.Ptr_comp) == 0)
    {
        res = 1;
        return res;
    }
    if (RxTransState.Ptr_buff != -1 && RxTransState.Ptr_comp == -1)
    {
        RxTransState.Ptr_comp = 0;
        res = -2;
        return res;
    }
	memcpy(msg_cmd, (ptr_usb_msg_u8)RxTransState.Buff[(unsigned char)*ptr_RxBuff], MESSAGE_MAX);
    *msg_size = RxTransState.MsgSize[(unsigned char)*ptr_RxBuff];
    RxTransState.Ptr_comp++;
    RxTransState.Ptr_comp &= 0x07;
    res = 0;
    return res;
}