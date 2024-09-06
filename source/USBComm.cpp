// ---------------------------------------------------------------------------
//  Filename: USBComm.c
//  Created by: Jack (the best looking one among all Jacks)
//  Date:  21/07/2020
//  Orisol (c)
//  Note: This version is modified from the previous version for Host and virtual RTC on Linux "only".
//  It does NOT work for physical RTC (PIC) and is not able to compile on Windows.
//  On Linux, UHF-host and virtual RTC share the code. Once anyone has to modify the code, please sync code between UHF-host and virtual RTC.
// ---------------------------------------------------------------------------

#define _CRT_SECURE_NO_WARNINGS
#include <bl_return_code.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "Definition.h"
#include "ONS_General.h"

#ifdef _ROLE_VIRTUAL_RTC
#include "Timers.h"
#endif

#include "USBComm.h"
#include "USBComm_Driver.h"

#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>

#ifdef USE_SYSLOG
#include "ONS_Syslog.h" //ONS_SYSLOG
#include "Debug.h"
#endif // USE_SYSLOG
#include <libusb-1.0/libusb.h>
#include "logger_wrapper.h"
#include "interface_to_host.h"

// -----------------
// Internal Define
// -----------------

#define TX_THREAD_SLEEP_IN_MILLISECONDS 2 // 5 Milliseconds
#define RX_THREAD_SLEEP_IN_MILLISECONDS 1
// ---------------------
// internal Prototypes
// ---------------------

static int giRtcId; // 0=physical RTC connect over USB, 1=virtual RTC connect over TCP

// static void* USBComm_Init_USB( void* );
static int USBComm_Init_USB(int iRtcId);
//#define TX_MAX_RETRIES 3

static int USBComm_Init_Threads(int iRtcId);
static void USBComm_Init_Buffers(void);
static int USBComm_Kill_Threads(void);
static int USBComm_Terminate_USB(void);
static int USBComm_Bulk_Transmit_Messages(void);
static int USBComm_Bulk_Receive_Messages(void);
static void USBComm_Reset_Tx_Buffer(void);
static void USBComm_Reset_Rx_Buffer(void);

//---------------------
// internal variables
// --------------------
static ONS_BYTE USBComm_Rx_Buffer[USBCOMM_RX_BUFFER_SIZE][USBCOMM_MAX_RX_MESSAGE_SIZE]; // Buffer for incoming messages received from USB.
static ONS_BYTE USBComm_Tx_Buffer[USBCOMM_TX_BUFFER_SIZE][USBCOMM_MAX_TX_MESSAGE_SIZE]; // Buffer for outgoing messages to transmit to USB.
static USBComm_Status_t Current_Status;													// Holds the current status of the Module.
int Rx_Write_To_Buffer_Index;															// Index of the next free buffer entry.
int Rx_Read_From_Buffer_Index;															// Index to the next message to read.
int Tx_Write_To_Buffer_Index;															// Index of the next free buffer entry.
int Tx_Read_From_Buffer_Index;															// Index to the next message to read.
static int Rx_Actual_Message_Size[USBCOMM_RX_BUFFER_SIZE];								// Hold the actual message size for each incoming message.
static int Tx_Actual_Message_Size[USBCOMM_TX_BUFFER_SIZE];								// Hold the actual message size for each outgoing message.
static ONS_BYTE gaOutGoingMessage[USBCOMM_MAX_TX_MESSAGE_SIZE];
static ONS_BYTE incomming_message[USBCOMM_MAX_RX_MESSAGE_SIZE];
static int System_In_Termination_State;
static int USB_Comm_Termination_Required;
static int USBComm_Init_Threads_Pass = -1;
char str_usblog[256];

#if defined(_ROLE_HOST)
static pthread_t USBComm_Init_Thread; // The USB first initialization thread
#endif

// ---------------------------------------------------------------------------
//  Function name		: USBComm_Init
//
//  Purpose				: Initialization of the UUSBCommmodule
//
//  Inputs				: iDevice
//
//  Outputs				:
//						:
//  Returns				: Success or Error code
//
//  Description			: Activates the Module initialization sequence.
//						: Host : call the USB initialization thread,
//						: transmit and receive threads initialization
//						: and resets transmit and receive buffers.
//						: Note : All the initialization process error codes
//						: are critical errors
//
// ---------------------------------------------------------------------------
int USBComm_Init(int iRtcId)
{
	int res = USBCOMM_SUCCESS;

	giRtcId = iRtcId;

	// Set the status to default values
	Current_Status.RX_Buffer_Full = ONS_FALSE;
	Current_Status.RX_Current_Lost_Msgs = 0;
	Current_Status.RX_Total_Lost_Msgs = 0;
	Current_Status.RX_Total_Msgs = 0;
	Current_Status.Rx_Total_Data_Size = 0;
	Current_Status.USBComm_Module_State = USBCOMM_INIT_STATE;
	Current_Status.TX_Buffer_Epmty = ONS_TRUE;
	Current_Status.TX_Current_Buffer_Full_Msgs = 0;
	Current_Status.TX_Total_Buffer_Full_Msgs = 0;
	Current_Status.TX_Total_Msgs = 0;
	Current_Status.Tx_Total_Data_Size = 0;
	Current_Status.Tx_Num_Of_Retry = 0;
	Current_Status.Rx_Num_Of_Retry = 0;
	Current_Status.libusb_error_code = 0;

	USBComm_Init_Buffers();

	res = USBComm_Init_Threads(iRtcId);
	if (res != USBCOMM_SUCCESS)
	{
		// TBD -define critical errors in libusb and in usbcom
		Current_Status.USBComm_Module_State = USBCOMM_ERROR_STATE; // failed to initialize the threads, set the status to Error mode.
		printf("USBComm_Init_Threads fail\n");
		return res;
	}

	res = USBComm_Init_USB(iRtcId);
	if (res != 0)
	{
		// TBD -define critical errors in libusb and in usbcom
		//		Current_Status.USBComm_Module_State = USBCOMM_ERROR_STATE;		// USB initialization failed, set the status to Error mode.
	}

	sprintf(str_usblog, "%s[%d], usb_init_res:%d", __func__, __LINE__, res);
	string tmp_string(str_usblog);
	goDriverLogger.Log("debug", tmp_string);
	return res;
}

// --------------------------------------------------------------------------
//  Function name		: USBComm_Termination
//
//  Purpose				: Handles the termination of the USBComm module.
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				: Success or Error code
//
//  Description			: Releases the USB interface, close the USB device handler
//						: and exit the libusb, resets transmit and receive buffers
//						: and kill transmit and receive threads.
//						: Note : All the termination process error codes
//						: are critical errors
// ---------------------------------------------------------------------------
int USBComm_Termination(void)
{
	int res = USBCOMM_SUCCESS;
	res = USBComm_Kill_Threads();
	if (res != USBCOMM_SUCCESS)
	{
		// TBD -define critical errors in libusb and in usbcom
		Current_Status.USBComm_Module_State = USBCOMM_ERROR_STATE; // failed to kill the threads, set the status to Error mode.
		return res;
	}
	res = USBComm_Terminate_USB();
	if (res != USBCOMM_SUCCESS)
	{
		// TBD -define critical errors in libusb and in usbcom
		Current_Status.USBComm_Module_State = USBCOMM_ERROR_STATE; // failed to initialize the threads, set the status to Error mode.
		return res;
	}
	return res;
}

// --------------------------------------------------------------------------
//  Function name		: USBComm_Send_Message
//
//  Purpose				: Handles the sending of a message
//
//  Inputs				: Message - pointer to TX message
//						: Message_Size - the TX message size
//
//  Outputs				:
//
//  Returns				: Success or Error code
//
//  Description			: Verify that the transmit message size not exceed maximum
//						: and that the buffer not full.
//						: After verification the message inserted into the the buffer
//						: the USBCome_Tx_Thread responsible for sending
//						: the message through the USB bus.
//
// ---------------------------------------------------------------------------
int USBComm_Send_Message(USBComm_Buffer_t Message, int Message_Size)
{
	// Verification stage
	// TBD -define critical errors in libusb and in usbcom
	if (Current_Status.USBComm_Module_State == USBCOMM_ERROR_STATE)
	{
		return USBCOMM_ERROR_MODULE_IN_ERROR_STATE;
	}

	if (Current_Status.USBComm_Module_State == USBCOMM_INIT_STATE)
	{
		return USBCOMM_ERROR_MODULE_NOT_READY;
	}

	if (Message_Size > USBCOMM_MAX_TX_MESSAGE_SIZE)
	{
		return USBCOMM_ERROR_MESSAGE_SIZE;
	}

	USBComm_Driver_Tx_Mutex_Lock(); // lock Tx mutex

	if (USBComm_Is_Tx_Buffer_Full())
	{
		Current_Status.TX_Total_Buffer_Full_Msgs++;
		Current_Status.TX_Current_Buffer_Full_Msgs++;
		USBComm_Driver_Tx_Mutex_UNLock(); // Unlock Tx mute
		return USBCOMM_ERROR_BUFFER_FULL;
	}

	Current_Status.TX_Current_Buffer_Full_Msgs = 0;

	memcpy(USBComm_Tx_Buffer[Tx_Write_To_Buffer_Index], Message, Message_Size); // Access Buffer
	Tx_Actual_Message_Size[Tx_Write_To_Buffer_Index] = Message_Size;
	Current_Status.TX_Buffer_Epmty = ONS_FALSE;
	Tx_Write_To_Buffer_Index = (Tx_Write_To_Buffer_Index + 1) % USBCOMM_TX_BUFFER_SIZE; // Update cyclic Index

	USBComm_Driver_Tx_Mutex_UNLock(); // Unlock Tx mutex

	return USBCOMM_SUCCESS;
}

// --------------------------------------------------------------------------
//  Function name		: USBComm_Is_Tx_Buffer_Full
//
//  Purpose				: Check if the transmit buffer full
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				: ONS_TRUE or ONS_FALSE
//
//  Description			: Check if the transmit buffer full
//						: Note: before using the USBComm_Send_Message() function it is not mendetory to use this function,
//                      : the USBComm_Get_Message() check the buffer verify that the buffer not empty.
//
// ---------------------------------------------------------------------------
int USBComm_Is_Tx_Buffer_Full(void)
{
	int res = ONS_FALSE;

	if ((Tx_Read_From_Buffer_Index == Tx_Write_To_Buffer_Index) // check if read and write set to the same index
		&& (Current_Status.TX_Buffer_Epmty == ONS_FALSE))		// and the buffer not empty

		res = ONS_TRUE;

	return res;
}

// --------------------------------------------------------------------------
//  Function name		: USBComm_Get_Message
//
//  Purpose				: Get the received message.
//
//  Inputs				: Buffer_Size  - size the area the give for the message
//
//  Outputs				: Message      					- the RX message pointer
//						: New_Message_Received			- ONS_TRUE if new message receive
//						: Message_Size - the real size if the incoming message
//
//  Returns				: Success or Error code
//
//  Description			: Check if the there is a message in the Rx Buffer
//						: and copy it according to the given message size,
//						: it also send the real size of the message
//
// ---------------------------------------------------------------------------
int USBComm_Get_Message(USBComm_Buffer_t Message,
						int Buffer_Size,
						int *New_Message_Received,
						int *Message_Size)
{

	*New_Message_Received = ONS_FALSE;

	// Verification stage
	if (Current_Status.USBComm_Module_State == USBCOMM_ERROR_STATE)
	{
		return USBCOMM_ERROR_MODULE_IN_ERROR_STATE;
	}

	if (Current_Status.USBComm_Module_State == USBCOMM_INIT_STATE)
	{
		return USBCOMM_ERROR_MODULE_NOT_READY;
	}

	USBComm_Driver_Rx_Mutex_Lock(); // lock Rx mutex

	if (USBComm_Is_Rx_Buffer_Empty())
	{
		USBComm_Driver_Rx_Mutex_UNLock(); // unlock Rx mutex
		return USBCOMM_SUCCESS;			  // No new message
	}

	*Message_Size = Rx_Actual_Message_Size[Rx_Read_From_Buffer_Index];
	if (*Message_Size > Buffer_Size)
	{
		USBComm_Driver_Rx_Mutex_UNLock(); // unlock Rx mutex
		return USBCOMM_ERROR_BUFFER_SIZE;
	}
	memcpy(Message, USBComm_Rx_Buffer[Rx_Read_From_Buffer_Index], *Message_Size);
	Rx_Actual_Message_Size[Rx_Read_From_Buffer_Index] = 0;
	Rx_Read_From_Buffer_Index = (Rx_Read_From_Buffer_Index + 1) % USBCOMM_RX_BUFFER_SIZE; // Update cyclic Index

	USBComm_Driver_Rx_Mutex_UNLock(); // unlock Rx mutex

	*New_Message_Received = ONS_TRUE;
	Current_Status.RX_Buffer_Full = ONS_FALSE;
	Current_Status.RX_Current_Lost_Msgs = 0;

	return USBCOMM_SUCCESS;
}

// --------------------------------------------------------------------------
//  Function name		: USBComm_Peek_Message
//
//  Purpose				: Give an option to look at the Rx message in the buffer
//
//  Inputs				: Message_View - pointer
//
//  Outputs				: Message_Size  - the new state of the component
//						:
//  Returns				: Success or Error code
//
//  Description			: Gives the pointer to next message from Rx Buffer without deleting it.
//						: The message stays in the buffer.
//						: The caller can only look at here(read only).
//
// ---------------------------------------------------------------------------
int USBComm_Peek_Message(USBComm_Buffer_t *Message,
						 unsigned int *New_Message_Received,
						 unsigned int *Message_Size)
{
	*New_Message_Received = ONS_FALSE;

	// Verification stage
	// TBD -define critical errors in libusb and in usbcom
	if (Current_Status.USBComm_Module_State == USBCOMM_ERROR_STATE)
	{
		return USBCOMM_ERROR_MODULE_IN_ERROR_STATE;
	}

	if (Current_Status.USBComm_Module_State == USBCOMM_INIT_STATE)
	{
		return USBCOMM_ERROR_MODULE_NOT_READY;
	}
	USBComm_Driver_Rx_Mutex_Lock(); // lock Rx mutex

	if (USBComm_Is_Rx_Buffer_Empty())
	{
		USBComm_Driver_Rx_Mutex_UNLock(); // unlock Rx mutex
		return USBCOMM_SUCCESS;			  // No new message
	}

	*Message = (USBComm_Buffer_t)(USBComm_Rx_Buffer[Rx_Read_From_Buffer_Index]);

	*Message_Size = Rx_Actual_Message_Size[Rx_Read_From_Buffer_Index];
	if (*Message_Size > 0)
	{
		*New_Message_Received = ONS_TRUE;
	}

	USBComm_Driver_Rx_Mutex_UNLock(); // unlock Rx mutex

	return USBCOMM_SUCCESS;
}

// --------------------------------------------------------------------------
//  Function name		: USBComm_Remove_Rx_Message
//
//  Purpose				: remove received message from buffer (if we already used it)
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				: ONS_TRUE or ONS_FALSE
//
//  Description			: Check if the receiving buffer empty
//						: Note: before using the USBComm_Get_Message() function it is not mandatory to use this function,
//                      : the USBComm_Get_Message() check the buffer verify that the buffer not empty.
//
// ---------------------------------------------------------------------------
int USBComm_Remove_Rx_Message(void)
{
	if (Current_Status.USBComm_Module_State == USBCOMM_ERROR_STATE)
	{
		return USBCOMM_ERROR_MODULE_IN_ERROR_STATE;
	}

	if (Current_Status.USBComm_Module_State == USBCOMM_INIT_STATE)
	{
		return USBCOMM_ERROR_MODULE_NOT_READY;
	}

	USBComm_Driver_Rx_Mutex_Lock(); // lock Rx mutex

	if (USBComm_Is_Rx_Buffer_Empty())
	{
		USBComm_Driver_Rx_Mutex_UNLock(); // unlock Rx mutex
		return USBCOMM_SUCCESS;			  // No new message
	}

	Rx_Actual_Message_Size[Rx_Read_From_Buffer_Index] = 0;
	Rx_Read_From_Buffer_Index = (Rx_Read_From_Buffer_Index + 1) % USBCOMM_RX_BUFFER_SIZE; // Update cyclic Index

	USBComm_Driver_Rx_Mutex_UNLock(); // unlock Rx mutex

	Current_Status.RX_Buffer_Full = ONS_FALSE;
	Current_Status.RX_Current_Lost_Msgs = 0;

	return USBCOMM_SUCCESS;
}

// --------------------------------------------------------------------------
//  Function name		: USBComm_Is_Rx_Buffer_Empty
//
//  Purpose				: Check if the receiving buffer empty
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				: ONS_TRUE or ONS_FALSE
//
//  Description			: Check if the receiving buffer empty
//						: Note: before using the USBComm_Get_Message() function it is not mandatory to use this function,
//                      : the USBComm_Get_Message() check the buffer verify that the buffer not empty.
//
// ---------------------------------------------------------------------------
int USBComm_Is_Rx_Buffer_Empty(void)
{
	int res = ONS_FALSE;

	if ((Rx_Read_From_Buffer_Index == Rx_Write_To_Buffer_Index) // check if read and write set to the same index
		&& (Current_Status.RX_Buffer_Full == ONS_FALSE))		// and the buffer not empty

		res = ONS_TRUE;

	return res;
}

// --------------------------------------------------------------------------
//  Function name		: USBComm_Get_Status
//
//  Purpose				: to give the module status
//
//  Inputs				:
//
//  Outputs				: module status
//
//  Returns				:
//
//  Description			: gives the module status structure
//
// ---------------------------------------------------------------------------
void USBComm_Get_Status(USBComm_Status_t *Status)
{
	*Status = Current_Status;
}

// --------------------------------------------------------------------------
//  Function name		: USBComm_Get_Module_State
//
//  Purpose				: to give the module  state
//
//  Inputs				:
//
//  Outputs				: Module state
//
//  Returns				:
//
//  Description			: gives the module  state
//
// ---------------------------------------------------------------------------
void USBComm_Get_Module_State(int *Module_State)
{
	*Module_State = Current_Status.USBComm_Module_State;
}

// --------------------------------------------------------------------------
//  Function name		: USBComm_Set_Init_Complete
//
//  Purpose				: Set the module state to ready
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				:
//
//  Description			: Set the module state to ready
//
// ---------------------------------------------------------------------------
void USBComm_Set_Init_Complete(void)
{
	Current_Status.USBComm_Module_State = USBCOMM_READY_STATE;
}

// --------------------------------------------------------------------------
//  Function name		: USBComm_Init_USB
//
//  Purpose				: Initialize the USB library.
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				:
//
//  Description			: initialize the USB library, set the debug level,
//						: detach the kernel drive of the device if needed, open the device port and get the device handler, using libusb library.
//						: open the device port and get the device handler.
//						: Check the current EP for given PID and VID
// ---------------------------------------------------------------------------

static int USBComm_Init_USB(int iRtcId)
{
	int iReturn;
	char str1, str2;
	char *str_prodct = NULL;
	char *str_manufature = NULL;
	// procedure:
	// 1. libusb_init:It must be called at the beginning of the program, before other libusb routines are used.
	//                This function returns 0 on success or LIBUSB_ERROR on failure.
	// 2. try find rtc_usb in usb_list.

	iReturn = USBComm_Driver_Init(iRtcId); // initialize the library for the session we just declared
	if (iReturn != 0)
	{
		Current_Status.USBComm_Module_State = USBCOMM_ERROR_STATE;
		Current_Status.libusb_error_code = iReturn;
		strcpy(Current_Status.libusb_strerror, libusb_strerror((libusb_error)Current_Status.libusb_error_code));
		sprintf(str_usblog, "%s[%d],errno:%d,%s", __func__, __LINE__, Current_Status.libusb_error_code, Current_Status.libusb_strerror);
		string tmp_string(str_usblog);
		goDriverLogger.Log("debug", tmp_string);
		return iReturn;
	}

	USBComm_Driver_Set_Debug(iRtcId, 3); // set debug level to 3, as suggested in the documentation

	iReturn = USBComm_Driver_Find_RTC_Device(iRtcId, &str_prodct, &str_manufature);
	if (iReturn != USBCOMM_SUCCESS)
	{
		sprintf(str_usblog, "%s[%d]cannot find UsbDevice in list", __func__, __LINE__);
		string tmp_string(str_usblog);
		goDriverLogger.Log("error", tmp_string);
		return iReturn;
	}
	else
	{
		sprintf(str_usblog, "%s[%d]find device success, [%s,%s]", __func__, __LINE__, str_prodct, str_manufature);
		string tmp_string(str_usblog);
		goDriverLogger.Log("debug", tmp_string);
	}

	iReturn = USBComm_Driver_Open_USB_Device(iRtcId);
	if (iReturn != USBCOMM_SUCCESS)
	{
		Current_Status.USBComm_Module_State = USBCOMM_ERROR_STATE;

		sprintf(str_usblog, "%s[%d],errno:%d,%s", __func__, __LINE__, USBCOMM_ERROR_CAN_NOT_OPEN_DEVICE, "the device could not be found(not attach)");
		string tmp_string(str_usblog);
		goDriverLogger.Log("error", tmp_string);

		send_message_to_host("usb init USBCOMM_ERROR_CAN_NOT_OPEN_DEVICE", ERR_FAIL_TO_INIT_USB, "the device could not be found(not attach)");
		return iReturn;
	}

	iReturn = USBComm_Driver_Is_Kernel_Driver_Active(iRtcId);
	if (iReturn < 0)
	{
		Current_Status.libusb_error_code = iReturn;
		strcpy(Current_Status.libusb_strerror, libusb_strerror((libusb_error)Current_Status.libusb_error_code));
		sprintf(str_usblog, "%s[%d],errno:%d,%s", __func__, __LINE__, Current_Status.libusb_error_code, Current_Status.libusb_strerror);
		string tmp_string(str_usblog);
		goDriverLogger.Log("debug", tmp_string);
	}

	if (iReturn == 1)
	{
		iReturn = USBComm_Driver_Detach_Kernel_Driver(iRtcId); // detach kernel driver
		if (iReturn != 0)
		{
			Current_Status.USBComm_Module_State = USBCOMM_ERROR_STATE;
			Current_Status.libusb_error_code = iReturn;
			strcpy(Current_Status.libusb_strerror, libusb_strerror((libusb_error)Current_Status.libusb_error_code));
			sprintf(str_usblog, "%s[%d],errno:%d,%s", __func__, __LINE__, Current_Status.libusb_error_code, Current_Status.libusb_strerror);
			string tmp_string(str_usblog);
			goDriverLogger.Log("debug", tmp_string);
			return iReturn;
		}
	}

	if (iReturn != 0)
	{
		Current_Status.USBComm_Module_State = USBCOMM_ERROR_STATE;
		Current_Status.libusb_error_code = iReturn;
		strcpy(Current_Status.libusb_strerror, libusb_strerror((libusb_error)Current_Status.libusb_error_code));
		sprintf(str_usblog, "%s[%d],errno:%d,%s", __func__, __LINE__, Current_Status.libusb_error_code, Current_Status.libusb_strerror);
		string tmp_string(str_usblog);
		goDriverLogger.Log("debug", tmp_string);
		return iReturn;
	}

	iReturn = USBComm_Driver_Claim_Interface(iRtcId); // claim interface 0 (the first) of device (mine had just 1)
	if (iReturn != 0)
	{
		Current_Status.USBComm_Module_State = USBCOMM_ERROR_STATE;
		Current_Status.libusb_error_code = iReturn;
		strcpy(Current_Status.libusb_strerror, libusb_strerror((libusb_error)Current_Status.libusb_error_code));
		sprintf(str_usblog, "%s[%d],errno:%d,%s", __func__, __LINE__, Current_Status.libusb_error_code, Current_Status.libusb_strerror);
		string tmp_string(str_usblog);
		goDriverLogger.Log("debug", tmp_string);
		return iReturn;
	}

	Current_Status.USBComm_Module_State = USBCOMM_READY_STATE;
	return 0;
}

// --------------------------------------------------------------------------
//  Function name		: USBComm_Init_Threads
//
//  Purpose				: Initialize the USB receive thread and USB transmit thread.
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				: USBCOMM_SUCCESS or  error code
//
//  Description			: Initialize and activate receive thread and USB transmit thread.
//
// ---------------------------------------------------------------------------
static int USBComm_Init_Threads(int iRtcId)
{
	int res;

	res = USBComm_Driver_Create_Rx_Thread(iRtcId);
	if (res != 0)
	{
		return res;
	}

	res = USBComm_Driver_Create_Tx_Thread(iRtcId);
	if (res != 0)
	{

		if (USBComm_Driver_Rx_Thread_kill(iRtcId) != 0)
		{
#if defined(_ROLE_HOST)
			// Debug_Print_Error_Message_With_Code("Error while killing USBComm_Rx_Thread ", res );
			printf("Error while creating USBComm_Tx_Thread\n");
#endif
		}
		return res;
	}

	return USBCOMM_SUCCESS;
}

// --------------------------------------------------------------------------
//  Function name		: USBComm_Init_Buffers
//
//  Purpose				: Initialize the receive and transmit buffer.
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				:
//
//  Description			: Initialize the receive and transmit buffer
//						: the head and tail pointer set to the start of the buffer.
//
// ---------------------------------------------------------------------------
static void USBComm_Init_Buffers(void)
{
	USBComm_Reset_Rx_Buffer();
	USBComm_Reset_Tx_Buffer();
}

// --------------------------------------------------------------------------
//  Function name		: USBComm_Kill_Threads
//
//  Purpose				: Terminate the receive and transmit buffer.
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				:
//
//  Description			: Terminate the receive and transmit buffer.
//
// ---------------------------------------------------------------------------
static int USBComm_Kill_Threads(void)
{
	int res;

	res = USBComm_Driver_Rx_Thread_kill(giRtcId);
	if (res != 0)
	{
		return res;
	}

	res = USBComm_Driver_Tx_Thread_kill(giRtcId);
	if (res != 0)
	{
		return res;
	}

	return USBCOMM_SUCCESS;
}

// --------------------------------------------------------------------------
//  Function name		: USBComm_Terminate_USB
//
//  Purpose				: Reset the Rx Buffer.
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				:
//
//  Description			: Reset the Rx Buffer, the head and tail pointer
//						: set to the start of the Buffer.
//
// ---------------------------------------------------------------------------
static int USBComm_Terminate_USB()
{
	int res;

	printf("USBComm_Terminate_USB,0\n");
	res = USBComm_Driver_Release_Interface(giRtcId); // release the claimed interface
	printf("USBComm_Termination,3\n");

	if (res != 0)
	{
		return res;
	}
	USBComm_Driver_Close_USB_Device(giRtcId); // exit USB library
	return USBCOMM_SUCCESS;
}

// --------------------------------------------------------------------------
//  Function name		: USBComm_Bulk_Transmit_Messages
//
//  Purpose				: Send massage to the USB bus.
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				:
//
//  Description			: Send Tx message from the TX buffer, if exist, to the USB bus
//						: in bulk mode.
//
// ---------------------------------------------------------------------------
static int USBComm_Bulk_Transmit_Messages(void)
{
	int message_size;
	int res;
	int actual;

	//	TMP_Into_Transmit++;
	// TBD -define critical errors in libusb and in usbcom
	if (Current_Status.USBComm_Module_State == USBCOMM_ERROR_STATE)
	{
		return USBCOMM_ERROR_MODULE_IN_ERROR_STATE;
	}

	if (Current_Status.USBComm_Module_State == USBCOMM_INIT_STATE)
	{
		return USBCOMM_ERROR_MODULE_NOT_READY;
	}

	USBComm_Driver_Tx_Mutex_Lock();

	memcpy(gaOutGoingMessage, USBComm_Tx_Buffer[Tx_Read_From_Buffer_Index], Tx_Actual_Message_Size[Tx_Read_From_Buffer_Index]);

	message_size = Tx_Actual_Message_Size[Tx_Read_From_Buffer_Index];

	USBComm_Driver_Tx_Mutex_UNLock(); // unlock Tx mutex

	// Tmp_b_send_msg_
	// Transmit Message to USB bus
	res = USBComm_Driver_Bulk_Send_Message(giRtcId, gaOutGoingMessage, message_size, &actual);

	if (res == USBCOMM_ERROR_RETRY)
	{
		Current_Status.Tx_Num_Of_Retry++;
		return res;
	}

	USBComm_Driver_Tx_Mutex_Lock();

	Tx_Read_From_Buffer_Index = (Tx_Read_From_Buffer_Index + 1) % USBCOMM_TX_BUFFER_SIZE; // Update cyclic Index
	if (Tx_Read_From_Buffer_Index == Tx_Write_To_Buffer_Index)
	{
		Current_Status.TX_Buffer_Epmty = ONS_TRUE;
	}

	USBComm_Driver_Tx_Mutex_UNLock(); // unlock Tx mutex

	if (actual != message_size)
	{
		res = USBCOMM_ERROR_MESSAGE_SIZE;
	}
	else
	{
		res = USBCOMM_SUCCESS;
	}

	Current_Status.TX_Total_Msgs++;
	Current_Status.Tx_Total_Data_Size += actual;

	return res;
}

// --------------------------------------------------------------------------
//  Function name		: USBComm_Bulk_Recive_Messages
//
//  Purpose				: Send massage to the USB bus.
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				:
//
//  Description			: Receive Rx message from USB bus in bulk mode
//						: and insert it to the RX buffer.
//
// ---------------------------------------------------------------------------
static int USBComm_Bulk_Receive_Messages(void)
{
	int res;
	int actual;

	// TBD -define critical errors in libusb and in usbcom
	if (Current_Status.USBComm_Module_State == USBCOMM_ERROR_STATE)
	{
		return USBCOMM_ERROR_MODULE_IN_ERROR_STATE;
	}

	if (Current_Status.USBComm_Module_State == USBCOMM_INIT_STATE)
	{
		return USBCOMM_ERROR_MODULE_NOT_READY;
	}

	if (Current_Status.RX_Buffer_Full == ONS_TRUE)
	{
		Current_Status.RX_Current_Lost_Msgs++;
		Current_Status.RX_Total_Lost_Msgs++;
		Current_Status.RX_Total_Msgs++;
		// return USBCOMM_SUCCESS;

		return USBCOMM_ERROR_BUFFER_FULL;
	}

	// Receive Message from USB bus

	res = USBComm_Driver_Bulk_Receive_Message(giRtcId, incomming_message, &actual);

	if (res == USBCOMM_ERROR_RETRY)
	{
		Current_Status.Rx_Num_Of_Retry++;
		return res;
	}

	if (res != 0)
	{
		Current_Status.RX_Current_Lost_Msgs++;
		Current_Status.RX_Total_Lost_Msgs++;
		Current_Status.RX_Total_Msgs++;
		// TBD -define critical errors in libusb and in usbcom
		Current_Status.USBComm_Module_State = USBCOMM_ERROR_STATE; // critical error - abort USB receive transmit operation

		return res;
	}

	if (res == 0 && actual == 0)
	{
		return USBCOMM_SUCCESS;
	}

	Current_Status.RX_Total_Msgs++;
	Current_Status.Rx_Total_Data_Size += actual;

	USBComm_Driver_Rx_Mutex_Lock(); // lock Rx mutex

	Rx_Actual_Message_Size[Rx_Write_To_Buffer_Index] = actual;
	memcpy(USBComm_Rx_Buffer[Rx_Write_To_Buffer_Index], incomming_message, Rx_Actual_Message_Size[Rx_Write_To_Buffer_Index]);
	Rx_Write_To_Buffer_Index = (Rx_Write_To_Buffer_Index + 1) % USBCOMM_RX_BUFFER_SIZE; // Update cyclic Index

	if (Rx_Read_From_Buffer_Index == Rx_Write_To_Buffer_Index)
	{
		Current_Status.RX_Buffer_Full = ONS_TRUE;
	}

	USBComm_Driver_Rx_Mutex_UNLock(); // unlock Rx mutex

	return USBCOMM_SUCCESS;
}

// --------------------------------------------------------------------------
//  Function name		: USBComm_Reset_Tx_Buffer
//
//  Purpose				: Reset the Tx Buffer
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				:
//
//  Description			: Reset the Tx Buffer, the head and tail pointer
//						: set to the start of the Buffer.
//
// ---------------------------------------------------------------------------
static void USBComm_Reset_Tx_Buffer(void)
{
	USBComm_Driver_Tx_Mutex_Lock(); // lock Tx mutex

	Tx_Read_From_Buffer_Index = 0;
	Tx_Write_To_Buffer_Index = 0;
	Current_Status.TX_Buffer_Epmty = ONS_TRUE;

	USBComm_Driver_Tx_Mutex_UNLock(); // unlock Tx mutex
}

// --------------------------------------------------------------------------
//  Function name		: USBComm_Reset_Rx_Buffer
//
//  Purpose				: Reset the Rx Buffer.
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				:
//
//  Description			: Reset the Rx Buffer, the head and tail pointer
//						: set to the start of the Buffer.
//
// ---------------------------------------------------------------------------
static void USBComm_Reset_Rx_Buffer(void)
{
	int i;
	USBComm_Driver_Rx_Mutex_Lock(); // lock Rx Mutex

	Rx_Read_From_Buffer_Index = 0;
	Rx_Write_To_Buffer_Index = 0;
	Current_Status.RX_Buffer_Full = ONS_FALSE;
	for (i = 0; i < USBCOMM_RX_BUFFER_SIZE; i++)
		Rx_Actual_Message_Size[i] = 0;

	USBComm_Driver_Rx_Mutex_UNLock(); // unlock Rx Mutex
}

// --------------------------------------------------------------------------
//  Function name		: USBComm_Rx_Thread_Main
//
//  Purpose				: create the thread and read periodically from the USB bus
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				:
//
//  Description			: Create the Rx thread and read periodically from the USB bus
//
// ---------------------------------------------------------------------------
bool g_b_usb_rx_thread_run = true;
void USBComm_Rx_Thread_Terminate()
{
	g_b_usb_rx_thread_run = false;
}
void *USBComm_Rx_Thread_Main(void *p)
{
	int res;
	char dump_data[255];
	int index;
	(void)p; // avoid compiler unused-warning

	pthread_detach(pthread_self());

	while (ONS_TRUE && g_b_usb_rx_thread_run)
	{
		if (Current_Status.USBComm_Module_State != USBCOMM_READY_STATE)
		{
			// if (Current_Status.USBComm_Module_State == USBCOMM_INIT_STATE){
			usleep(500);
			continue;
		}
		/*if (Current_Status.USBComm_Module_State == USBCOMM_ERROR_STATE){
			break;
		}*/

		res = USBComm_Bulk_Receive_Messages(); // blocked until message arrived

		if (USBCOMM_SUCCESS != res)
		{
			// Debug_Print_Error_Message_With_Code("ERROR USBCOMM - Bulk receive message error!!!! ", res );
			//  debug messages

			USBComm_Driver_Rx_Mutex_Lock(); // lock Rx mutex
			sprintf(dump_data, "Rx_Write_To_Buffer_Index = %d, Rx_Read_From_Buffer_Index = %d \n", Rx_Write_To_Buffer_Index, Rx_Read_From_Buffer_Index);
			////Log_Write_String_Msg(__FILE__, __func__, __LINE__, -1, dump_data);
			for (index = 0; index < USBCOMM_RX_BUFFER_SIZE; index++)
			{
				sprintf(dump_data, "ID=%d, actual=%d, data: \n ", index, Rx_Actual_Message_Size[index]);
				////Log_Write_String_Msg(__FILE__, __func__, __LINE__, -1, dump_data);
				sprintf(dump_data, "%2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X \n",
						USBComm_Rx_Buffer[index][0], USBComm_Rx_Buffer[index][1], USBComm_Rx_Buffer[index][2], USBComm_Rx_Buffer[index][3], USBComm_Rx_Buffer[index][4], USBComm_Rx_Buffer[index][5], USBComm_Rx_Buffer[index][6], USBComm_Rx_Buffer[index][7], USBComm_Rx_Buffer[index][8], USBComm_Rx_Buffer[index][9],
						USBComm_Rx_Buffer[index][10], USBComm_Rx_Buffer[index][11], USBComm_Rx_Buffer[index][12], USBComm_Rx_Buffer[index][13], USBComm_Rx_Buffer[index][14], USBComm_Rx_Buffer[index][15], USBComm_Rx_Buffer[index][16], USBComm_Rx_Buffer[index][17], USBComm_Rx_Buffer[index][18], USBComm_Rx_Buffer[index][19],
						USBComm_Rx_Buffer[index][20], USBComm_Rx_Buffer[index][21], USBComm_Rx_Buffer[index][22], USBComm_Rx_Buffer[index][23], USBComm_Rx_Buffer[index][24], USBComm_Rx_Buffer[index][25], USBComm_Rx_Buffer[index][26], USBComm_Rx_Buffer[index][27], USBComm_Rx_Buffer[index][28], USBComm_Rx_Buffer[index][29],
						USBComm_Rx_Buffer[index][30], USBComm_Rx_Buffer[index][31], USBComm_Rx_Buffer[index][32], USBComm_Rx_Buffer[index][33], USBComm_Rx_Buffer[index][34], USBComm_Rx_Buffer[index][35], USBComm_Rx_Buffer[index][36], USBComm_Rx_Buffer[index][37], USBComm_Rx_Buffer[index][38], USBComm_Rx_Buffer[index][39],
						USBComm_Rx_Buffer[index][40], USBComm_Rx_Buffer[index][41], USBComm_Rx_Buffer[index][42], USBComm_Rx_Buffer[index][43], USBComm_Rx_Buffer[index][44], USBComm_Rx_Buffer[index][45], USBComm_Rx_Buffer[index][46], USBComm_Rx_Buffer[index][47], USBComm_Rx_Buffer[index][48], USBComm_Rx_Buffer[index][49],
						USBComm_Rx_Buffer[index][50], USBComm_Rx_Buffer[index][51], USBComm_Rx_Buffer[index][52], USBComm_Rx_Buffer[index][53], USBComm_Rx_Buffer[index][54], USBComm_Rx_Buffer[index][55], USBComm_Rx_Buffer[index][56], USBComm_Rx_Buffer[index][57], USBComm_Rx_Buffer[index][58], USBComm_Rx_Buffer[index][59],
						USBComm_Rx_Buffer[index][60], USBComm_Rx_Buffer[index][61], USBComm_Rx_Buffer[index][62], USBComm_Rx_Buffer[index][63]);
				// Log_Write_Str(dump_data);
			}
			USBComm_Driver_Rx_Mutex_UNLock(); // unlock Rx mutex

			printf("USB RX Status- Full=%d lost msg:current=%lu, Total=%lu  total messages=%lu Total data=%lu \n ", Current_Status.RX_Buffer_Full, Current_Status.RX_Current_Lost_Msgs, Current_Status.RX_Total_Lost_Msgs, Current_Status.RX_Total_Msgs, Current_Status.Rx_Total_Data_Size);

			Current_Status.libusb_error_code = res;
			strcpy(Current_Status.libusb_strerror, libusb_strerror((libusb_error)Current_Status.libusb_error_code));
			sprintf(str_usblog, "%s[%d],errno:%d,%s", __func__, __LINE__, Current_Status.libusb_error_code, Current_Status.libusb_strerror);
			string tmp_string(str_usblog);
			goDriverLogger.Log("debug", tmp_string);
		}
	}
	printf("USBComm_Rx_Thread_Terminate\n");
	return NULL;
}

#ifdef USE_SYSLOG
void USBComm_Print_Current_Status_to_log(void)
{
	if (Debug_Get_Syslog_Level() > SB_SEVERITY_NOTICE)
	{
		//		char    dump_data[255];
		char msg_txt[MAX_SYSLOG_DESC_SIZE];

		//	sprintf(dump_data, "USBComm_Print_Current_Status_to_log\n");
		//	Log_Write_String_Msg(__FILE__, __func__, __LINE__, -1, dump_data);
		snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "USBComm_Print_Current_Status_to_log");
		ONS_Syslog_Send_Msg(SB_SEVERITY_NOTICE, (char *)__FILENAME__, __LINE__, (char *)__func__, msg_txt);

		//	sprintf(dump_data, "USB RX Status- Full=%d lost msg:current=%lu, Total=%lu  total messages=%lu Toata data=%lu \n", Current_Status.RX_Buffer_Full, Current_Status.RX_Current_Lost_Msgs, Current_Status.RX_Total_Lost_Msgs, Current_Status.RX_Total_Msgs, Current_Status.Rx_Total_Data_Size);
		//	Log_Write_Str(dump_data);
		snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "USB RX Status - Full = %d lost msg : current = %lu, Total = %lu  total messages = %lu Toata data = %lu \n", Current_Status.RX_Buffer_Full, Current_Status.RX_Current_Lost_Msgs, Current_Status.RX_Total_Lost_Msgs, Current_Status.RX_Total_Msgs, Current_Status.Rx_Total_Data_Size);
		ONS_Syslog_Send_Msg(SB_SEVERITY_NOTICE, (char *)__FILENAME__, __LINE__, (char *)__func__, msg_txt);

		//	sprintf(dump_data, "USB TX Status- Empty=%d total=%lu, Buff full:current=%lu, Total=%lu  total messages=%lu Toata data=%lu \n ", Current_Status.TX_Buffer_Epmty, Current_Status.Tx_Total_Buffer_empty, Current_Status.TX_Current_Buffer_Full_Msgs, Current_Status.TX_Total_Buffer_Full_Msgs, Current_Status.TX_Total_Msgs, Current_Status.Tx_Total_Data_Size);
		//	Log_Write_Str(dump_data);
		snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "USB TX Status- Empty=%d total=%lu, Buff full:current=%lu, Total=%lu  total messages=%lu Toata data=%lu \n ", Current_Status.TX_Buffer_Epmty, Current_Status.Tx_Total_Buffer_empty, Current_Status.TX_Current_Buffer_Full_Msgs, Current_Status.TX_Total_Buffer_Full_Msgs, Current_Status.TX_Total_Msgs, Current_Status.Tx_Total_Data_Size);
		ONS_Syslog_Send_Msg(SB_SEVERITY_NOTICE, (char *)__FILENAME__, __LINE__, (char *)__func__, msg_txt);

		// sprintf(dump_data, "Rx_Write_To_Buffer_Index = %d, Rx_Read_From_Buffer_Index = %d Tx_Write_To_Buffer_Index = %d, Tx_Read_From_Buffer_Index = %d \n ", Rx_Write_To_Buffer_Index, Rx_Read_From_Buffer_Index, Tx_Write_To_Buffer_Index, Tx_Read_From_Buffer_Index);
		// Log_Write_Str(dump_data);
		snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Rx_Write_To_Buffer_Index = %d, Rx_Read_From_Buffer_Index = %d Tx_Write_To_Buffer_Index = %d, Tx_Read_From_Buffer_Index = %d \n ", Rx_Write_To_Buffer_Index, Rx_Read_From_Buffer_Index, Tx_Write_To_Buffer_Index, Tx_Read_From_Buffer_Index);
		ONS_Syslog_Send_Msg(SB_SEVERITY_NOTICE, (char *)__FILENAME__, __LINE__, (char *)__func__, msg_txt);
	}
}
#endif

// --------------------------------------------------------------------------
//  Function name		: USBComm_Tx_Thread_Main
//
//  Purpose				: In ready state, Write periodically to the USB bus
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				:
//
//  Description			:  In ready state, Write periodically to the USB bus
//
// ---------------------------------------------------------------------------
bool g_b_usb_tx_thread_run = true;
void USBComm_Tx_Thread_Terminate()
{
	g_b_usb_tx_thread_run = false;
}

void *USBComm_Tx_Thread_Main(void *p)
{
	int res;

	(void)p; // avoid compiler unused-warning

	pthread_detach(pthread_self());

	while (ONS_TRUE && g_b_usb_tx_thread_run)
	{
		if (Current_Status.USBComm_Module_State == USBCOMM_INIT_STATE)
		{
			usleep(500);
			continue;
		}
		if (Current_Status.USBComm_Module_State == USBCOMM_ERROR_STATE)
		{
			break;
		}

		if (!Current_Status.TX_Buffer_Epmty)
		{
			res = USBComm_Bulk_Transmit_Messages();

			if (USBCOMM_SUCCESS != res)
			{
				// Debug_Print_Error_Message_With_Code("ERROR USBCOMM - Bulk transmit message error!!!! ", res );
				// Log_Write_Error(__FILE__, __func__, __LINE__,  res ,"ERROR USBCOMM - Bulk transmit message error!!!! ");
			}
			// printf("USBComm_Tx_Thread_Main res=%d\n", res);
		}
		else
		{
			Current_Status.Tx_Total_Buffer_empty++;
			USBComm_Driver_Sleep_Millisecond(TX_THREAD_SLEEP_IN_MILLISECONDS);
		}
	}
	printf("USBComm_Tx_Thread_Terminate\n");
	return NULL;
}
/*
#if defined(_ROLE_GUEST)
// --------------------------------------------------------------------------
//  Function name		: USBComm_Rx_Int_Main
//
//  Purpose				: read periodically from the USB bus
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				:
//
//  Description			: read periodically from the USB bus
//
// ---------------------------------------------------------------------------
void  USBComm_Rx_Int_Main ( void )
{
	int res;
	res = USBComm_Bulk_Receive_Messages();
	if( !res )
	{
		USBTimer_SetInterruptTimeMode( USBTimer_Interrupt_Fast_Speed , USBTimer_Rx );
	}
	else
	{
		USBTimer_SetInterruptTimeMode( USBTimer_Interrupt_Slow_Speed , USBTimer_Rx);
	}
}

// --------------------------------------------------------------------------
//  Function name		: USBComm_Tx_Int_Main
//
//  Purpose				: write periodically to the USB bus
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				:
//
//  Description			: write periodically from the USB bus
//
// ---------------------------------------------------------------------------
void  USBComm_Tx_Int_Main ( void )
{
	int res;

	if(!Current_Status.TX_Buffer_Epmty)
	{
		res = USBComm_Bulk_Transmit_Messages();
		USBTimer_SetInterruptTimeMode ( USBTimer_Interrupt_Fast_Speed , USBTimer_Tx );
	}
	else
	{
		Current_Status.Tx_Total_Buffer_empty++;
		USBTimer_SetInterruptTimeMode ( USBTimer_Interrupt_Slow_Speed , USBTimer_Tx );
	}
}
#endif
*/
// --------------------------------------------------------------------------
//  Function name		: USBComm_Get_Version
//
//  Purpose				: get the module version
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				: version
//
//  Description			: get the module version
//
// ---------------------------------------------------------------------------
const char *USBComm_Get_Version(void)
{
	return USBCOMM_VERSION;
}

#ifdef USE_SYSLOG
// --------------------------------------------------------------------------
//  Function name		: USBComm_Print_Rx_Buffer_Data
//
//  Purpose				: Print Rx Buffer Data
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				:
//
//  Description			: Print Rx Buffer Data
//
// ---------------------------------------------------------------------------
void USBComm_Print_Rx_Buffer_Data(void)
{
	int index;
	char msg_txt[MAX_SYSLOG_DESC_SIZE];

	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Rx_Write_To_Buffer_Index = %d, Rx_Read_From_Buffer_Index = %d \n", Rx_Write_To_Buffer_Index, Rx_Read_From_Buffer_Index);
	ONS_Syslog_Send_Msg(SB_SEVERITY_NOTICE, (char *)__FILENAME__, __LINE__, (char *)__func__, msg_txt);
	USBComm_Driver_Rx_Mutex_Lock(); // lock Rx mutex

	for (index = 0; index < USBCOMM_RX_BUFFER_SIZE; index++)
	{

		snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "%d : %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X \n",
				 index, USBComm_Rx_Buffer[index][0], USBComm_Rx_Buffer[index][1], USBComm_Rx_Buffer[index][2], USBComm_Rx_Buffer[index][3], USBComm_Rx_Buffer[index][4], USBComm_Rx_Buffer[index][5], USBComm_Rx_Buffer[index][6], USBComm_Rx_Buffer[index][7], USBComm_Rx_Buffer[index][8], USBComm_Rx_Buffer[index][9],
				 USBComm_Rx_Buffer[index][10], USBComm_Rx_Buffer[index][11], USBComm_Rx_Buffer[index][12], USBComm_Rx_Buffer[index][13], USBComm_Rx_Buffer[index][14], USBComm_Rx_Buffer[index][15], USBComm_Rx_Buffer[index][16], USBComm_Rx_Buffer[index][17], USBComm_Rx_Buffer[index][18], USBComm_Rx_Buffer[index][19],
				 USBComm_Rx_Buffer[index][20], USBComm_Rx_Buffer[index][21], USBComm_Rx_Buffer[index][22], USBComm_Rx_Buffer[index][23], USBComm_Rx_Buffer[index][24], USBComm_Rx_Buffer[index][25], USBComm_Rx_Buffer[index][26], USBComm_Rx_Buffer[index][27], USBComm_Rx_Buffer[index][28], USBComm_Rx_Buffer[index][29],
				 USBComm_Rx_Buffer[index][30], USBComm_Rx_Buffer[index][31], USBComm_Rx_Buffer[index][32], USBComm_Rx_Buffer[index][33], USBComm_Rx_Buffer[index][34], USBComm_Rx_Buffer[index][35], USBComm_Rx_Buffer[index][36], USBComm_Rx_Buffer[index][37], USBComm_Rx_Buffer[index][38], USBComm_Rx_Buffer[index][39],
				 USBComm_Rx_Buffer[index][40], USBComm_Rx_Buffer[index][41], USBComm_Rx_Buffer[index][42], USBComm_Rx_Buffer[index][43], USBComm_Rx_Buffer[index][44], USBComm_Rx_Buffer[index][45], USBComm_Rx_Buffer[index][46], USBComm_Rx_Buffer[index][47], USBComm_Rx_Buffer[index][48], USBComm_Rx_Buffer[index][49],
				 USBComm_Rx_Buffer[index][50], USBComm_Rx_Buffer[index][51], USBComm_Rx_Buffer[index][52], USBComm_Rx_Buffer[index][53], USBComm_Rx_Buffer[index][54], USBComm_Rx_Buffer[index][55], USBComm_Rx_Buffer[index][56], USBComm_Rx_Buffer[index][57], USBComm_Rx_Buffer[index][58], USBComm_Rx_Buffer[index][59],
				 USBComm_Rx_Buffer[index][60], USBComm_Rx_Buffer[index][61], USBComm_Rx_Buffer[index][62], USBComm_Rx_Buffer[index][63]);
		ONS_Syslog_Send_Msg(SB_SEVERITY_NOTICE, (char *)__FILENAME__, __LINE__, (char *)__func__, msg_txt);
		USBComm_Driver_Rx_Mutex_UNLock(); // unlock Rx mutex
	}
}
#endif

// --------------------------------------------------------------------------
//  Function name		: USBComm_Set_System_In_Termination_State
//
//  Purpose				:
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				:
//
//  Description			:
//
// ---------------------------------------------------------------------------
void USBComm_Set_System_In_Termination_State(int Value)
{
	System_In_Termination_State = Value;
}
// --------------------------------------------------------------------------
//  Function name		: USBComm_Is_System_In_Termination_State
//
//  Purpose				:
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				:
//
//  Description			:
// ---------------------------------------------------------------------------
int USBComm_Is_System_In_Termination_State()
{
	return System_In_Termination_State;
}
// --------------------------------------------------------------------------
//  Function name		: USBCOMM_Is_Termination_Required
//
//  Purpose				: inform main thread that USB_Comm need to terminat
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				:
//
//  Description			: return USB_Comm_Termination_Required - it set to TRUE if critical USB ERROR
//
// ---------------------------------------------------------------------------
int Is_USBCOMM_Termination_Required(void)
{
	return USB_Comm_Termination_Required;
}

int USBComm_getErrMsg(int *errcode, char *str_errmsg)
{
	if (Current_Status.libusb_error_code < 0)
	{
		*errcode = Current_Status.libusb_error_code;
		memcpy(str_errmsg, Current_Status.libusb_strerror, sizeof(Current_Status.libusb_strerror));
		return 0;
	}
	else
		return -1;
}