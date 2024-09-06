// ---------------------------------------------------------------------------
//  Filename: USBComm_Driver.c
//  Created by: Nissim Avichail
//  Date:  30/06/15
//  Orisol (c)
// ---------------------------------------------------------------------------
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <stdint.h>
#include "Definition.h"
#include "ONS_General.h"

#ifdef ONS_WIN
#include <windows.h>
#include <stdio.h>
#endif

#ifdef ONS_USB_WIN
#include <C:\LIBUSB\libusb-win32-bin-1.2.6.0\include\lusb0_usb.h>
#endif // ONS_USB_WIN

#ifdef ONS_IP_WIN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#endif // ONS_IP_WIN

#ifdef ONS_RTC_SIM_IP_WIN
#undef UNICODE

#endif

#ifdef ONS_HOST_LINUX
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <libusb-1.0/libusb.h>
// #include "Debug.h"
#endif

#ifdef ONS_RTC
#include "usb.h"
#include "HardwareProfile_dsPIC33EP512MU810_PIM.h"
#include "usb_function_generic.h"
#endif

#include "USBComm.h"
#include "USBComm_Driver.h"

// -----------------
// Internal Define
// -----------------
#ifdef ONS_IP_WIN
// Need to link with Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"
#ifdef ONS_HOST_IP_WIN
// Need to link withMswsock.lib, and Advapi32.lib
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")
#define DEFAULT_IP_ADDRESS "127.0.0.1" // "localhost"

//#define DEFAULT_IP_ADDRESS     "localhost"

struct addrinfo *ptr;

#endif

#ifdef ONS_RTC_SIM_IP_WIN
SOCKET ListenSocket;
#endif

static WSADATA wsaData;
SOCKET ConnectSocket;
struct addrinfo *result;
struct addrinfo hints;
#endif // ONS_IP_WIN

#ifdef HOST_VS_HOST
// Use this define if you testing HOST to HOST USB communication (Using UNITEK Y-3501 USB Cable)
#define RTC_USB_VID 0x067b // TESTING Vendor=067b ProdID=27a1
#define RTC_USB_PID 0x27a1 // TESTING
#define RTC_USB_EP_OUT 8   // TESTING
#define RTC_USB_EP_IN 137  // TESTING
#else
#define RTC_USB_VID 0x04d8
#define RTC_USB_PID 0x0204
#define RTC_USB_EP_OUT 1
#define RTC_USB_EP_IN 129
#endif

#define INTRFACE_NUM 0

#define BULK_OUT_TIMEOUT 0 // 0 -No timeout [MS]
#define BULK_IN_TIMEOUT 0  // 0 -No timeout

#define Rx_THREAD_PRIORITY 99
#define Tx_THREAD_PRIORITY 99

//---------------------
// internal variables
// --------------------
#ifdef ONS_HOST_LINUX
typedef struct
{
	libusb_device_handle *Dev_Handle; // USB device handle
	libusb_context *Ctx = NULL;		  // USB libusb session
	// #ifdef ONS_HOST_LINUX
	pthread_mutex_t Rx_Buffer_Mutex; // Mutex to prevent read/Write problem.
	pthread_mutex_t Tx_Buffer_Mutex; // Mutex to prevent read/Write problem.
	pthread_t USBComm_Rx_Thread;	 // USB->host receive thread
	pthread_t USBComm_Tx_Thread;	 // host->USB transmit thread
	// #endif
} USBComm_Driver_Elment_t;

static USBComm_Driver_Elment_t USBComm_Driver_Elment;

#endif

#ifdef ONS_USB_WIN

static usb_dev_handle *dev_handle;
#endif

#ifdef ONS_WIN
static HANDLE USBComm_Rx_Thread;
static HANDLE USBComm_Tx_Thread;
static DWORD dw_USBComm_Rx_Thread;
static DWORD dw_USBComm_Tx_Thread;
static HANDLE Rx_Buffer_Mutex; // Mutex to prevent read/Write problem.
static HANDLE Tx_Buffer_Mutex; // Mutex to prevent read/Write problem.

#endif
#ifdef ONS_RTC

static int USBCheckDeviceStatus(void);
static int USBComm_Rtc_Err_Flag; // USB event error flag
static int USBComm_Rx_Int_Flag;	 // USB Interrupt receive flag
static int USBComm_Tx_Int_Flag;	 // USB Interrupt send flag

USB_HANDLE USBComm_GenericInHandle;		 // USB send handle.  Must be initialized to 0 at startup.
USB_HANDLE USBComm_GenericOutHandle;	 // USB receive handle.  Must be initialized to 0 at startup.
unsigned char OUTPacket[USBGEN_EP_SIZE]; // USB input messages local buffer for receive messages
#endif

// ---------------------------------------------------------------------------
//  Function name		: USBComm_Driver_Set_Debug
//
//  Purpose				: Initialization of the USBComm module
//
//  Inputs				: Log_message_levels
//
//  Outputs				:
//						:
//  Returns				:
//
//  Description			: Host -  Set libusb Debug level
//						: LIBUSB_LOG_LEVEL_NONE		(0) : no messages ever printed by the library (default)
//						: LIBUSB_LOG_LEVEL_ERROR	(1) : error messages are printed to stderr
//						: LIBUSB_LOG_LEVEL_WARNING	(2) : warning and error messages are printed to stderr
//						: LIBUSB_LOG_LEVEL_INFO		(3) : informational messages are printed to stdout, warning and error messages are printed to stderr
//						: LIBUSB_LOG_LEVEL_DEBUG	(4) : debug and informational messages are printed to stdout, warnings and errors to stderr)
//
// ---------------------------------------------------------------------------
void USBComm_Driver_Set_Debug(int Log_message_levels)
{
#ifdef ONS_HOST_LINUX

	libusb_set_debug(USBComm_Driver_Elment.Ctx, Log_message_levels); // suggested level in the documentation - LIBUSB_LOG_LEVEL_INFO		(3)

#endif

#ifdef ONS_USB_WIN
	usb_set_debug(Log_message_levels);
#endif

	return;
}

// ---------------------------------------------------------------------------
//  Function name		: USBComm_Driver_Init
//
//  Purpose				: Initialize USB Library.
//
//  Inputs				: USBComm_Rx_Buffer_Adder - pointer to the Rx Buffer in USBComm
//
//  Outputs				:
//						:
//  Returns				: 0 on success, or a Error code on failure (Host - LIBUSB_ERROR)
//
//  Description			: Host -  initialize the libusb
//
// ---------------------------------------------------------------------------
int USBComm_Driver_Init(void)
{
#ifdef ONS_HOST_LINUX

	return libusb_init(&USBComm_Driver_Elment.Ctx); // initialize the library for the session we just declared

#endif

#ifdef ONS_USB_WIN

	usb_init(); /* initialize the library */

	return USBCOMM_SUCCESS;

#endif

#ifdef ONS_IP_WIN
	int res;

	// Init params
	result = NULL;

#ifdef ONS_HOST_IP_WIN
	ptr = NULL;
#endif

#ifdef ONS_RTC_SIM_IP_WIN
	ListenSocket = INVALID_SOCKET;
#endif

	ConnectSocket = INVALID_SOCKET;

	// Initialize Winsock
	res = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (res != 0)
	{
		WSACleanup();
	}

	return res;

#endif

#ifdef ONS_RTC
	USBComm_Rtc_Err_Flag = 0;
	USBComm_Rx_Int_Flag = 0;
	USBComm_Tx_Int_Flag = 0;
	USBComm_GenericInHandle = 0;
	USBComm_GenericOutHandle = 0;
	USBDeviceInit();
	USBDeviceAttach();
	return USBCOMM_SUCCESS;
#endif
}

// ---------------------------------------------------------------------------
//  Function name		: USBComm_Driver_Open_USB_Device
//
//  Purpose				: finding the device handler.
//
//  Inputs				:
//
//  Outputs				:
//						:
//  Returns				: USBCOMM_SUCCESS of USBCOMM_ERROR_CAN_NOT_OPEN_DEVICE
//
//  Description			: Host - Convenience function for finding a device
//						:		 with a particular idVendor/idProduct combination
//                        RTC - This function attaches the device.
// ---------------------------------------------------------------------------
int USBComm_Driver_Open_USB_Device()
{
#ifdef ONS_HOST_LINUX

	USBComm_Driver_Elment.Dev_Handle = libusb_open_device_with_vid_pid(USBComm_Driver_Elment.Ctx, RTC_USB_VID, RTC_USB_PID);

	if (USBComm_Driver_Elment.Dev_Handle == NULL)
	{
		// printf("USBComm_Driver_Open_USB_Device fail\n");
		return USBCOMM_ERROR_CAN_NOT_OPEN_DEVICE;
	}

	return USBCOMM_SUCCESS;

#endif

#ifdef ONS_HOST_IP_WIN
	int res;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	res = getaddrinfo(DEFAULT_IP_ADDRESS, DEFAULT_PORT, &hints, &result);
	if (res != 0)
	{
		WSACleanup();
	}

	return USBCOMM_SUCCESS;

#endif // ONS_IP_WIN

#ifdef ONS_RTC_SIM_IP_WIN
	int res;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	res = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (res != 0)
	{
		WSACleanup();
	}
	return res;

#endif // ONS_RTC_SIM_IP_WIN

#ifdef ONS_USB_WIN

	struct usb_bus *bus;
	struct UsbDevice *dev;

	dev_handle = NULL;

	usb_find_busses();	/* find all busses */
	usb_find_devices(); /* find all connected devices */

	for (bus = usb_get_busses(); bus; bus = bus->next)
	{
		for (dev = bus->devices; dev; dev = dev->next)
		{
			if (dev->descriptor.idVendor == RTC_USB_VID && dev->descriptor.idProduct == RTC_USB_PID)
			{
				dev_handle = usb_open(dev);
			}
		}

		if (dev_handle == NULL)
		{

			printf("error opening device: \n%s\n", usb_strerror());
			return -1; // TBD
		}
		else
		{
			printf("success: device %04X:%04X opened\n", RTC_USB_VID, RTC_USB_PID);
		}

		return USBCOMM_SUCCESS;
	}
	return -1;

#endif

#ifdef ONS_RTC

	return USBCOMM_SUCCESS;
#endif
}

// ---------------------------------------------------------------------------
//  Function name		: USBComm_Driver_Is_Kernel_Driver_Active
//
//  Purpose				: Determine if a kernel driver is active on an interface.
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				: 0 on success, 1 if active, or a Error code on failure (Host - LIBUSB_ERROR)
//
//  Description			: Host - Convenience function for finding a device
//						:		 If a kernel driver is active, you cannot claim the interface,
//						:		 and libusb will be unable to perform I/O.
//
// ---------------------------------------------------------------------------
int USBComm_Driver_Is_Kernel_Driver_Active(void)
{
#ifdef ONS_HOST_LINUX

	return libusb_kernel_driver_active(USBComm_Driver_Elment.Dev_Handle, 0);

#endif

#ifdef ONS_WIN
	return USBCOMM_SUCCESS;
#endif

#ifdef ONS_RTC
	return USBCOMM_SUCCESS;
#endif
}

// ---------------------------------------------------------------------------
//  Function name		: USBComm_Driver_Detach_Kernel_Driver
//
//  Purpose				: Detach a kernel driver from an interface.
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				: 0 on success, or a Error code on failure (Host - LIBUSB_ERROR)
//
//  Description			: Host - Detach a kernel driver from an interface.
//						:		 If successful, you will then be able to claim the interface and perform I/O.
//
// ---------------------------------------------------------------------------
int USBComm_Driver_Detach_Kernel_Driver(void)
{
#ifdef ONS_HOST_LINUX

	return libusb_detach_kernel_driver(USBComm_Driver_Elment.Dev_Handle, 0); // detach kernel driver

#endif

#ifdef ONS_WIN
	return USBCOMM_SUCCESS;
#endif

#ifdef ONS_RTC
	return USBCOMM_SUCCESS;
#endif
}

// ---------------------------------------------------------------------------
//  Function name		: USBComm_Driver_Release_Interface
//
//  Purpose				: Release an interface previously claimed with libusb_claim_interface().
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				: 0 on success, or a Error code on failure (Host - LIBUSB_ERROR)
//
//  Description			: Host - Release an interface previously claimed with libusb_claim_interface().
//						:		 You should release all claimed interfaces before closing a device handle.
//						:		 This is a blocking function.
//
// ---------------------------------------------------------------------------
int USBComm_Driver_Release_Interface(void)
{
#ifdef ONS_HOST_LINUX

	return libusb_release_interface(USBComm_Driver_Elment.Dev_Handle, 0); // release the claimed interface

#endif

#ifdef ONS_USB_WIN

	usb_release_interface(dev_handle, 0);

#endif

	return USBCOMM_SUCCESS;
}

// ---------------------------------------------------------------------------
//  Function name		: USBComm_Driver_Claim_Interface
//
//  Purpose				: Claim an interface on a given device handle.
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				: 0 on success, or a Error code on failure (Host - LIBUSB_ERROR)
//
//  Description			: Host - Claim an interface on a given device handle.
//						:		 You must claim the interface you wish to use before you can perform I/O on any of its endpoints.
//						:		 This is a non-blocking function.
//
// ---------------------------------------------------------------------------
int USBComm_Driver_Claim_Interface(void)
{
#ifdef ONS_HOST_LINUX

	return libusb_claim_interface(USBComm_Driver_Elment.Dev_Handle, 0); // claim interface 0 (the first) of device (mine had just 1)

#endif

#ifdef ONS_RTC_SIM_IP_WIN
	int res;

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET)
	{
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return __LINE__;
	}

	// Setup the TCP listening socket
	res = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (res == SOCKET_ERROR)
	{
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return __LINE__;
	}

	freeaddrinfo(result);

	res = listen(ListenSocket, SOMAXCONN);
	if (res == SOCKET_ERROR)
	{
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return __LINE__;
	}

	// Accept a client socket
	ConnectSocket = accept(ListenSocket, NULL, NULL);
	if (ConnectSocket == INVALID_SOCKET)
	{
		printf("accept failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return __LINE__;
	}

	// No longer need server socket
	closesocket(ListenSocket);

	return USBCOMM_SUCCESS;

#endif

#ifdef ONS_HOST_IP_WIN
	int res;
	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
	{
		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET)
		{
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return __LINE__;
		}

		// Connect to server.
		res = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (res == SOCKET_ERROR)
		{
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET)
	{
		printf("Unable to connect to RTC (check if RTC running!)\n");
		WSACleanup();
		return __LINE__;
	}

	return USBCOMM_SUCCESS;
#endif // ONS_IP_WIN

#ifdef ONS_USB_WIN

	if (usb_set_configuration(dev_handle, 1) < 0)
	{
		printf("error setting config #%d: %s\n", 1, usb_strerror());
		usb_close(dev_handle);
		return __LINE__;
	}
	else
	{
		printf("success: set configuration #%d\n", 1);
	}

	if (usb_claim_interface(dev_handle, 0) < 0)
	{
		printf("error claiming interface #%d:\n%s\n", INTRFACE_NUM, usb_strerror());
		usb_close(dev_handle);
		return __LINE__;
	}
	else
	{
		printf("success: claim_interface #%d\n", INTRFACE_NUM);
	}
	return USBCOMM_SUCCESS;

#endif
#ifdef ONS_RTC
	return USBCOMM_SUCCESS;
#endif
	return USBCOMM_SUCCESS;
}

// ---------------------------------------------------------------------------
//  Function name		: USBComm_Driver_Close_USB_Device
//
//  Purpose				: Close a device handle.
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				:
//
//  Description			: Host - Close a device handle.
//						:		 Should be called on all open handles before your application exits.
//						:		 Internally, this function destroys the reference that was added by libusb_open() on the given device.
//						:		 This is a non-blocking function; no requests are sent over the bus.
//
// ---------------------------------------------------------------------------
void USBComm_Driver_Close_USB_Device(void)
{
#ifdef ONS_HOST_LINUX

	libusb_close(USBComm_Driver_Elment.Dev_Handle); // close the USB device

#endif

#ifdef ONS_USB_WIN

	if (dev_handle)
	{
		usb_close(dev_handle);
	}

#endif
#ifdef ONS_IP_WIN

	closesocket(ConnectSocket);
	WSACleanup();
#endif
}

// ---------------------------------------------------------------------------
//  Function name		: USBComm_Driver_Exit
//
//  Purpose				: uninitialized USB library.
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				:
//
//  Description			: Host - Deinitialize libusb.
//						:		 Should be called after closing all open devices and before your application terminates.
//                        RTC - Detach USB device
// ---------------------------------------------------------------------------
void USBComm_Driver_Exit(void)
{
#ifdef ONS_HOST_LINUX

	libusb_exit(USBComm_Driver_Elment.Ctx); // exit libusb
	return;
#endif

#ifdef ONS_USB_WIN

	if (dev_handle)
	{
		usb_close(dev_handle);
	}

#endif

#ifdef ONS_RTC
	USBSoftDetach();
#endif
}

// ---------------------------------------------------------------------------
//  Function name		: USBComm_Driver_Bulk_Send_Message
//
//  Purpose				: send message to USB in bulk mode.
//
//  Inputs				:
//
//  Outputs				: outgoing_message 	- pointer to the message to send
//						: message_size		- the size of the message to send
//						: actual 		   	- the actual size of that sent successfully
//
//  Returns				: 0 on success, or a Error code on failure (Host - LIBUSB_ERROR)
//
//  Description			: send message to USB EP (OUT direction) in bulk mode.
//
// ---------------------------------------------------------------------------
int USBComm_Driver_Bulk_Send_Message(ONS_BYTE *outgoing_message, int message_size, int *actual)
{
#ifdef ONS_HOST_LINUX

	return libusb_bulk_transfer(USBComm_Driver_Elment.Dev_Handle, (RTC_USB_EP_OUT | LIBUSB_ENDPOINT_OUT),
								outgoing_message, message_size, actual, BULK_OUT_TIMEOUT);
#endif

#ifdef ONS_USB_WIN
	*actual = usb_bulk_write(dev_handle, RTC_USB_EP_OUT, outgoing_message, message_size, BULK_OUT_TIMEOUT);
	if (*actual > 0)
	{
		return USBCOMM_SUCCESS;
	}
	return *actual;
#endif

#ifdef ONS_IP_WIN
	int res;

	res = send(ConnectSocket, outgoing_message, message_size, 0);
	if (res == SOCKET_ERROR)
	{
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return res;
	}
	*actual = res;
	return USBCOMM_SUCCESS;

#endif

#ifdef ONS_RTC
	if (!USBCheckDeviceStatus())
	{
		if (!USBHandleBusy(USBComm_GenericInHandle))
		{
			if (!USBComm_Tx_Int_Flag)
			{
				USBComm_GenericInHandle = USBGenWrite(USBGEN_EP_NUM, outgoing_message, message_size);
				*actual = USBHandleGetLength(USBComm_GenericInHandle);
				return USBCOMM_SUCCESS;
			}
		}
		return USBCOMM_ERROR_RETRY;
	}
	return USBCOMM_ERROR_MODULE_IN_ERROR_STATE;
#endif
}

// ---------------------------------------------------------------------------
//  Function name		: USBComm_Driver_Bulk_Receive_Message
//
//  Purpose				: receive message from USB in bulk mode.
//
//  Inputs				:
//
//  Outputs				: incomming_message 	- pointer to the  received message
//						: actual 		   		- the actual size of that sent successfully
//
//  Returns				: 0 on success, or a Error code on failure (Host - LIBUSB_ERROR)
//
//  Description			: send message to USB EP (OUT direction) in bulk mode.
//
// ---------------------------------------------------------------------------
int USBComm_Driver_Bulk_Receive_Message(ONS_BYTE *incomming_message, int *actual)
{
#ifdef ONS_HOST_LINUX

	return libusb_bulk_transfer(USBComm_Driver_Elment.Dev_Handle, (RTC_USB_EP_IN | LIBUSB_ENDPOINT_IN),
								incomming_message, USBCOMM_MAX_RX_MESSAGE_SIZE, actual, BULK_IN_TIMEOUT);

#endif

#ifdef ONS_USB_WIN
	*actual = usb_bulk_read(dev_handle, RTC_USB_EP_IN, incomming_message, USBCOMM_MAX_RX_MESSAGE_SIZE, 0);
	if (*actual > 0)
	{
		return USBCOMM_SUCCESS;
	}
	return *actual;
#endif

#ifdef ONS_IP_WIN
	int res;

	res = recv(ConnectSocket, incomming_message, USBCOMM_MAX_RX_MESSAGE_SIZE, 0);
	if (res > 0)
	{
		*actual = res;
		return USBCOMM_SUCCESS;
	}

	if (res == 0)
	{
		printf("Connection closed\n");
		return USBCOMM_ERROR_CONNECTION_FAIL;
	}
	else
	{
		res = WSAGetLastError();
		printf("recv failed with error: %d\n", WSAGetLastError());
		return res;
	}
#endif

#ifdef ONS_RTC
	if (!USBCheckDeviceStatus())
	{
		if (!USBHandleBusy(USBComm_GenericOutHandle))
		{
			if (!USBComm_Rx_Int_Flag)
			{
				*actual = USBHandleGetLength(USBComm_GenericOutHandle);
				memcpy(incomming_message, OUTPacket, *actual);
				USBComm_GenericOutHandle = USBGenRead(USBGEN_EP_NUM, (BYTE *)OUTPacket, USBGEN_EP_SIZE);
				return USBCOMM_SUCCESS;
			}
		}
		return USBCOMM_ERROR_RETRY;
	}
	return USBCOMM_ERROR_MODULE_IN_ERROR_STATE;
#endif
}

// ---------------------------------------------------------------------------
//  Function name		: USBComm_Driver_Tx_Mutex_Lock
//
//  Purpose				: lock Tx mutex.
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				:
//
//  Description			: lock Tx mutex/Interrupt Flag.
//
// ---------------------------------------------------------------------------
#ifdef ONS_PC_PLAT
void USBComm_Driver_Tx_Mutex_Lock(void)
#else
int USBComm_Driver_Tx_Mutex_Lock(void)
#endif
{
#ifdef ONS_HOST_LINUX

	pthread_mutex_lock(&USBComm_Driver_Elment.Tx_Buffer_Mutex); // lock Tx mutex

#endif

#ifdef ONS_WIN
	WaitForSingleObject(Tx_Buffer_Mutex, INFINITE);
#endif

#ifdef ONS_RTC
	// mutex already locked
	if (USBComm_Tx_Int_Flag)
		return ONS_FALSE;

	USBComm_Tx_Int_Flag = 1;
	return ONS_TRUE;
#endif
}

// ---------------------------------------------------------------------------
//  Function name		: USBComm_Driver_Rx_Mutex_Lock
//
//  Purpose				: lock Rx mutex.
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				:
//
//  Description			: lock Rx mutex/Interrupt Flag.
//
// ---------------------------------------------------------------------------
#ifdef ONS_PC_PLAT
void USBComm_Driver_Rx_Mutex_Lock(void)
#else
int USBComm_Driver_Rx_Mutex_Lock(void)
#endif
{
#ifdef ONS_HOST_LINUX

	pthread_mutex_lock(&USBComm_Driver_Elment.Rx_Buffer_Mutex); // lock Rx mutex

#endif

#ifdef ONS_WIN
	WaitForSingleObject(Rx_Buffer_Mutex, INFINITE);
#endif

#ifdef ONS_RTC
	if (USBComm_Rx_Int_Flag)
		return ONS_FALSE;
	USBComm_Rx_Int_Flag = 1;
	return ONS_TRUE;
#endif
}

// ---------------------------------------------------------------------------
//  Function name		: USBComm_Driver_Tx_Mutex_UNLock
//
//  Purpose				: unlock Tx mutex.
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				:
//
//  Description			: unlock Tx mutex/Interrupt Flag.
//
// ---------------------------------------------------------------------------
void USBComm_Driver_Tx_Mutex_UNLock(void)
{
#ifdef ONS_HOST_LINUX

	pthread_mutex_unlock(&USBComm_Driver_Elment.Tx_Buffer_Mutex); // unlock Tx mutex

#endif

#ifdef ONS_WIN
	ReleaseMutex(Tx_Buffer_Mutex);
#endif // ONS_WIN

#ifdef ONS_RTC
	USBComm_Tx_Int_Flag = 0;
#endif
}

// ---------------------------------------------------------------------------
//  Function name		: USBComm_Driver_Rx_Mutex_UNLock
//
//  Purpose				: unlock Rx mutex.
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				:
//
//  Description			: unlock Rx mutex/Interrupt Flag.
//
// ---------------------------------------------------------------------------
void USBComm_Driver_Rx_Mutex_UNLock(void)
{
#ifdef ONS_HOST_LINUX

	pthread_mutex_unlock(&USBComm_Driver_Elment.Rx_Buffer_Mutex); // unlock Rx mutex

#endif

#ifdef ONS_WIN
	ReleaseMutex(Rx_Buffer_Mutex);
#endif // ONS_WIN

#ifdef ONS_RTC
	USBComm_Rx_Int_Flag = 0;
#endif
}

// ---------------------------------------------------------------------------
//  Function name		: USBComm_Driver_Create_Tx_Thread
//
//  Purpose				: create Tx thread.
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				: 0 for success or pthread error code
//
//  Description			: create Tx thread.
//
// ---------------------------------------------------------------------------
int USBComm_Driver_Create_Tx_Thread(void)
{
#ifdef ONS_HOST_LINUX

	struct sched_param param;
	int policy;
	unsigned int res;

	res = pthread_create(&USBComm_Driver_Elment.USBComm_Tx_Thread, NULL, USBComm_Tx_Thread_Main, NULL);
	if (res != 0)
	{
		return res;
	}

	policy = SCHED_RR;
	param.sched_priority = Tx_THREAD_PRIORITY;
	res = pthread_setschedparam(USBComm_Driver_Elment.USBComm_Tx_Thread, policy, &param);
	if (res != 0)
	{
		return res;
	}
	param.sched_priority = 0;

	res = pthread_getschedparam(USBComm_Driver_Elment.USBComm_Tx_Thread, &policy, &param);

	return res;

#endif

#ifdef ONS_WIN
	DWORD dwError;

	USBComm_Tx_Thread = CreateThread(NULL,					 // default security attributes
									 0,						 // use default stack size
									 USBComm_Tx_Thread_Main, // thread function
									 NULL,					 // argument to thread function
									 0,						 // use default creation flags
									 &dw_USBComm_Tx_Thread); // returns the thread identifie
	printf("Tx thread ID = %d\n", dw_USBComm_Tx_Thread);

	if (USBComm_Tx_Thread == NULL)
	{
		dwError = GetLastError();
		printf("USBComm_Tx_Thread creation failed, error: %d.\n", dwError);
		return dwError;
	}
	/*
			if (!SetThreadPriority(USBComm_Tx_Thread, THREAD_PRIORITY_ABOVE_NORMAL))
			{
				dwError = GetLastError();
				printf("Failed to set TX Thread to hige priority err = %d\n", dwError);
				return -1;
			}
	*/
	// create TX MUTEX
	Tx_Buffer_Mutex = CreateMutex(NULL,	 // default security attributes
								  FALSE, // initially not owned
								  NULL); // unnamed mutex

	if (Tx_Buffer_Mutex == NULL)
	{
		dwError = GetLastError();
		printf("CreateMutex Tx_Buffer_Mutex error: %d\n", GetLastError());
		return -1;
	}
	return USBCOMM_SUCCESS;
#endif
#ifdef ONS_RTC
	return USBCOMM_SUCCESS;
#endif
}

// ---------------------------------------------------------------------------
//  Function name		: USBComm_Driver_Create_Rx_Thread
//
//  Purpose				: create Rx thread.
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				: 0 for success or  error code
//
//  Description			: create Rx thread.
//
// ---------------------------------------------------------------------------
int USBComm_Driver_Create_Rx_Thread(void)
{
#ifdef ONS_HOST_LINUX

	struct sched_param param;
	unsigned int res;
	int policy;

	res = pthread_create(&USBComm_Driver_Elment.USBComm_Rx_Thread, NULL, USBComm_Rx_Thread_Main, NULL);
	if (res != 0)
	{
		printf("USBComm_Driver_Create_Rx_Thread fail\n");
		return res;
	}
	// Debug_Print_Debug_Message_With_Param ("res = ", res);
	policy = SCHED_RR; // SCHED_FIFO;
	param.sched_priority = Rx_THREAD_PRIORITY;
	res = pthread_setschedparam(USBComm_Driver_Elment.USBComm_Rx_Thread, policy, &param);
	if (res != 0)
	{
		printf("pthread_setschedparam fail\n");
		printf("%d\n", res);
		return res;
	}
	res = pthread_getschedparam(USBComm_Driver_Elment.USBComm_Rx_Thread, &policy, &param);
	if (res != 0)
	{
		printf("pthread_getschedparam fail\n");
		return res;
	}

	return res;
#endif

#ifdef ONS_WIN

	DWORD dwError;

	USBComm_Rx_Thread = CreateThread(NULL,					 // default security attributes
									 0,						 // use default stack size
									 USBComm_Rx_Thread_Main, // thread function
									 NULL,					 // argument to thread function
									 0,						 // use default creation flags
									 &dw_USBComm_Rx_Thread); // returns the thread identifie
	if (USBComm_Rx_Thread == NULL)
	{
		dwError = GetLastError();
		printf("USBComm_Rx_Thread creation failed, error: %d.\n", dwError);
		return dwError;
	}

	printf("Rx thread ID = %d\n", dw_USBComm_Rx_Thread);
	/*
		if (!SetThreadPriority(USBComm_Rx_Thread, THREAD_PRIORITY_ABOVE_NORMAL))
		{
			dwError = GetLastError();
			printf("Failed to set RX Thread to hige priority err = %d\n", dwError);
			return -1;
		}
	*/
	// create TX MUTEX
	Rx_Buffer_Mutex = CreateMutex(NULL,	 // default security attributes
								  FALSE, // initially not owned
								  NULL); // unnamed mutex

	if (Rx_Buffer_Mutex == NULL)
	{
		dwError = GetLastError();
		printf("CreateMutex Rx_Buffer_Mutex error: %d\n", GetLastError());
		return -1;
	}
	return USBCOMM_SUCCESS;

#endif

#ifdef ONS_RTC
	return USBCOMM_SUCCESS;
#endif
}

// ---------------------------------------------------------------------------
//  Function name		: USBComm_Driver_Tx_Thread_kill
//
//  Purpose				: create Tx thread.
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				: 0 for success or  error code
//
//  Description			: create Tx thread.
//
// ---------------------------------------------------------------------------
int USBComm_Driver_Tx_Thread_kill(void)
{
#ifdef ONS_HOST_LINUX

	return pthread_kill(USBComm_Driver_Elment.USBComm_Tx_Thread, 1);
#endif

#ifdef ONS_WIN

	if (CloseHandle(USBComm_Tx_Thread) != 0)
	{
		if (CloseHandle(Tx_Buffer_Mutex) != 0)
		{
			return USBCOMM_SUCCESS;
		}
		else
		{
			printf("faild on close Tx mutex \n");
			return -1; // TBD ADD ERROR DEFINE
		}
	}
	else
	{
		printf("faild on close Tx Thread operation (Tx mutex not close)\n");
		return -1; // TBD ADD ERROR DEFINE
	}

#endif

#ifdef ONS_RTC
	return USBCOMM_SUCCESS;
#endif
}
// ---------------------------------------------------------------------------
//  Function name		: USBComm_Driver_Rx_Thread_kill
//
//  Purpose				: create Rx thread.
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				: 0 for success or  error code
//
//  Description			: create Rx thread.
//
// ---------------------------------------------------------------------------
int USBComm_Driver_Rx_Thread_kill(void)
{
#ifdef ONS_HOST_LINUX

	return pthread_kill(USBComm_Driver_Elment.USBComm_Rx_Thread, 1);
	//	return 0;

#endif

#ifdef ONS_WIN

	if (CloseHandle(USBComm_Rx_Thread) != 0)
	{
		if (CloseHandle(Rx_Buffer_Mutex) != 0)
		{
			return USBCOMM_SUCCESS;
		}
		else
		{
			printf("faild on close Rx mutex \n");
			return -1; // TBD ADD ERROR DEFINE
		}
	}
	else
	{
		printf("faild on close Tx Thread operation (Rx mutex not close)\n");
		return -1; // TBD ADD ERROR DEFINE
	}

#endif

#ifdef ONS_RTC
	return USBCOMM_SUCCESS;
#endif
}

// ---------------------------------------------------------------------------
//  Function name		: USBComm_Driver_Sleep_Millisecond
//
//  Purpose				: sleep for time in milliseconds.
//
//  Inputs				: sleep time in Millisecond
//
//  Outputs				:
//
//  Returns				:
//
//  Description			: sleep for time in milliseconds.
//
// ---------------------------------------------------------------------------
void USBComm_Driver_Sleep_Millisecond(unsigned int Time_Millisecond)
{
#ifdef ONS_HOST_LINUX

	usleep(Time_Millisecond * 1000); // Translate time to microseconds

#endif
#ifdef ONS_WIN

	Sleep(Time_Millisecond);

#endif // ONS_WIN
}

#ifdef ONS_RTC
// ---------------------------------------------------------------------------
//  Function name		: USBCBInitEP
//
//  Purpose				: This function is called when the device becomes initialized.
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				:
//
//  Description			: This callback function should initialize the endpoints
//                        for the device's usage according to the current configuration.
//
// ---------------------------------------------------------------------------
void USBCBInitEP(void)
{
	// Enable the application endpoints
	USBEnableEndpoint(USBGEN_EP_NUM, USB_OUT_ENABLED | USB_IN_ENABLED | USB_HANDSHAKE_ENABLED | USB_DISALLOW_SETUP);
	// Arm the application OUT endpoint, so it can receive a packet from the host
	USBComm_GenericOutHandle = USBGenRead(USBGEN_EP_NUM, (BYTE *)OUTPacket, USBGEN_EP_SIZE);
}

// ---------------------------------------------------------------------------
//  Function name		: USER_USB_CALLBACK_EVENT_HANDLER
//
//  Purpose				: This function is called from the USB stack to
//                        notify a user application that a USB event occured.
//
//  Inputs				: int event - the type of event
//                        void *pdata - pointer to the event data
//                        WORD size - size of the event data
//
//  Outputs				:
//
//  Returns				: BOOL True/False
//
//  Description			:  This callback is in interrupt context
//                         when the USB_INTERRUPT option is selected.
//
// ---------------------------------------------------------------------------
BOOL USER_USB_CALLBACK_EVENT_HANDLER(int event, void *pdata, WORD size)
{
	switch (event)
	{
	case EVENT_TRANSFER:
		break;
	case EVENT_SOF:
		break;
	case EVENT_SUSPEND:
		break;
	case EVENT_RESUME:
		break;
	case EVENT_CONFIGURED:
		USBCBInitEP();
		break;
	case EVENT_SET_DESCRIPTOR:
		break;
	case EVENT_EP0_REQUEST:
		break;
	case EVENT_BUS_ERROR:
	case EVENT_TRANSFER_TERMINATED:
		USBComm_Rtc_Err_Flag = 1;
		break;
	default:
		break;
	}
	return TRUE;
}

// ---------------------------------------------------------------------------
//  Function name		: USBCheckDeviceStatus
//
//  Purpose				: This function is called before each interrupt send / receive
//                        routine to check the device state.
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				: Success/Failure
//
//  Description			: This function check the device state.
//
// ---------------------------------------------------------------------------
int USBCheckDeviceStatus(void)
{
	if ((!(USBDeviceState & CONFIGURED_STATE)) || (USBSuspendControl == 1) || (USBComm_Rtc_Err_Flag == 1))
	{
		return USBCOMM_ERROR_MODULE_IN_ERROR_STATE;
	}
	return USBCOMM_SUCCESS;
}

// ---------------------------------------------------------------------------
//  Function name		: USBCheckFirstInit
//
//  Purpose				: This function is called to check first initialization of USB device.
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				: Success/Failure
//
//  Description			: This function check the device state.
//
// ---------------------------------------------------------------------------
int USBCheckFirstInit(void)
{
	if ((!(USBDeviceState & CONFIGURED_STATE)) || (USBSuspendControl == 1))
	{
		return USBCOMM_ERROR_MODULE_NOT_READY;
	}
	return USBCOMM_SUCCESS;
}

#endif
