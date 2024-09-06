// ---------------------------------------------------------------------------
//  Filename: USBComm_Driver_Linux64.c
//  Created by: Jack
//  Date:  18/08/2020
//  Orisol (c)
// ---------------------------------------------------------------------------

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "Definition.h"
#include "ONS_General.h"
#include <string.h>
#include "bl_return_code.h"
#include <pthread.h>
#include <errno.h>
#include <signal.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <RTC_Stitch.h>
#include <Msg_Prot.h>

#ifdef _ROLE_VIRTUAL_RTC
#include <stdbool.h>
#include <stddef.h>
#include "usb.h"
#include "HardwareProfile_dsPIC33EP512MU810_PIM.h"
#include "usb_function_generic.h"
#endif

#include <unistd.h>
#include <signal.h>
#include <libusb-1.0/libusb.h>

#include "USBComm.h"
#include "USBComm_Driver.h"

// -----------------
// Internal Define
// -----------------

#define DEFAULT_BUFLEN 512

// for virtual RTC communication
// host/driver <-------> port gszRtcPort, intermediary <--------> port VIRTUAL_RTC_PORT, virtual RTC

//#define DEFAULT_IP_ADDRESS	"127.0.0.1"
char gszDefaultIpv4[16] = "127.0.0.1";

#ifdef _ROLE_VIRTUAL_RTC
static int giSocket;
static int giClientSocket;
//#define VIRTUAL_RTC_PORT 		"27015"
char gszVirtualRtcPort[6] = "27015";
#endif

#ifdef _ROLE_HOST
static int giSocketForRtc;
#endif

//#define DEFAULT_PORT 		"27016"
// Driver uses the port number to connect to virtual RTC. Virtual RTC (of intermediary) listens to the port to provide service.
char gszRtcPort[6] = "27016";

struct addrinfo *gptResult;
struct addrinfo gtHints;
// struct addrinfo *gpPtr;

extern void AddBulkLog(bool bReceive, int iId, int iCount);
//#endif //

#if defined(_ROLE_HOST)
#define RTC_USB_VID 0x04d8
#define RTC_USB_PID 0x0204
#define RTC_USB_EP_OUT 1
#define RTC_USB_EP_IN 129
#endif //

#define INTRFACE_NUM 0

#define BULK_OUT_TIMEOUT 0 // 0 -No timeout [MS]
#define BULK_IN_TIMEOUT 0  // 0 -No timeout

#define Rx_THREAD_PRIORITY 99
#define Tx_THREAD_PRIORITY 99

//---------------------
// internal variables
// --------------------
typedef struct
{
    //#ifdef _CONNECT_OVER_USB
    libusb_device_handle *Dev_Handle; // USB device handle
                                      //	libusb_context*			Ctx = NULL;										//USB libusb session
    libusb_context *Ctx; // USB libusb session
                         //#endif
                         //  #ifdef ONS_HOST_LINUX
    pthread_mutex_t Rx_Buffer_Mutex; // Mutex to prevent read/Write problem.
    pthread_mutex_t Tx_Buffer_Mutex; // Mutex to prevent read/Write problem.
    pthread_t USBComm_Rx_Thread;     // USB->host receive thread
    pthread_t USBComm_Tx_Thread;     // host->USB transmit thread
    // #endif
} USBComm_Driver_Elment_t;
static USBComm_Driver_Elment_t USBComm_Driver_Elment;

typedef struct
{
    char str_iProduct[48];
    char str_iManufacturer[48];
    int i_bInterfaceNumber;
    int i_bNumEndpoints;
} USBComm_device_descriptor_t;
static USBComm_device_descriptor_t USBComm_rtc_descriptor;

// static int USBComm_Rtc_Err_Flag;
// USB_HANDLE USBComm_GenericInHandle;                                     //USB send handle.  Must be initialized to 0 at startup.
// USB_HANDLE USBComm_GenericOutHandle;
// unsigned char OUTPacket[USBGEN_EP_SIZE] ;                               //USB input messages local buffer for receive messages                                   //USB receive handle.  Must be initialized to 0 at startup.                                        //USB event error flag

#if defined(_ROLE_VIRTUAL_RTC)
// static int USBCheckDeviceStatus ( void );
static int USBComm_Rtc_Err_Flag; // USB event error flag
// static int USBComm_Rx_Int_Flag;                                         //USB Interrupt receive flag
// static int USBComm_Tx_Int_Flag;                                         //USB Interrupt send flag

USB_HANDLE USBComm_GenericInHandle;      // USB send handle.  Must be initialized to 0 at startup.
USB_HANDLE USBComm_GenericOutHandle;     // USB receive handle.  Must be initialized to 0 at startup.
unsigned char OUTPacket[USBGEN_EP_SIZE]; // USB input messages local buffer for receive messages
#endif

// ---------------------------------------------------------------------------
//  Function name		: USBComm_Driver_Set_Ipv4
//  Purpose				: For driver, the function assigns an IP for the driver to connect to virtual RTC.
//                        For virtual RTC, the function assigns an IP for the virtual RTC to bind and listen.
//  Inputs				: char* szNewIpv4 E.g "127.0.0.1"
//  Outputs				:
//  Returns				: 0=OK. Others=error code.
//  Description			: The function has to call before USBComm_Driver_Init().
#include <arpa/inet.h>
int USBComm_Driver_Set_Ipv4(char *szNewIpv4)
{
    if (strlen(szNewIpv4) < sizeof(gszDefaultIpv4))
    {
        struct sockaddr_in addr4;
        if (1 == inet_pton(AF_INET, szNewIpv4, &addr4.sin_addr))
        { // check argv[i+1] is valid ip format or not.
            strcpy(gszDefaultIpv4, szNewIpv4);
            return 0;
        }
        return ERR_INVALID_FORMAT;
    }
    return ERR_INVALID_LENGTH;
}

int USBComm_Driver_Set_Port(char *szNewPort)
{
    if (strlen(szNewPort) < sizeof(gszRtcPort))
    {
        if (strspn(szNewPort, "0123456789") == strlen(szNewPort))
        { // check argv[i+1] is all digit or not
            strcpy(gszRtcPort, szNewPort);
            return 0;
        }
        return ERR_INVALID_FORMAT;
    }
    return ERR_INVALID_LENGTH;
}

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
void USBComm_Driver_Set_Debug(int iRtcId, int Log_message_levels)
{
#if defined(_ROLE_HOST)
    switch (iRtcId)
    {
    case 0: // 0=physical RTC connect over USB
#if LIBUSB_API_VERSION >= 0x01000106
        libusb_set_option(USBComm_Driver_Elment.Ctx, LIBUSB_OPTION_LOG_LEVEL, Log_message_levels);
#else
        libusb_set_debug(USBComm_Driver_Elment.Ctx, Log_message_levels); // suggested level in the documentation - LIBUSB_LOG_LEVEL_INFO		(3)
#endif
        return;

    case 1: // 1=virtual RTC connect over TCP
        return;

    default:
        return;
    }
#endif

#if defined(_ROLE_VIRTUAL_RTC)
    (void)Log_message_levels; //
    switch (iRtcId)
    {
    case 1: // 1=virtual RTC connect over TCP
        return;

    default:
        return;
    }
#endif
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
int USBComm_Driver_Init(int iRtcId)
{
#if defined(_ROLE_HOST)
    switch (iRtcId)
    {
    case 0:                                             // 0=physical RTC connect over USB
        return libusb_init(&USBComm_Driver_Elment.Ctx); // initialize the library for the session we just declared

    case 1: // 1=virtual RTC connect over TCP
        giSocketForRtc = 0;
        return USBCOMM_SUCCESS;

    default:
        return ERR_UNKNOWN_DEVICE;
    }
#endif

#if defined(_ROLE_VIRTUAL_RTC)
    switch (iRtcId)
    {
    case 1: // 1=virtual RTC connect over TCP
        giSocket = 0;
        giClientSocket = 0;
        return USBCOMM_SUCCESS;

    default:
        return ERR_UNKNOWN_DEVICE;
    }
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
int USBComm_Driver_Open_USB_Device(int iRtcId)
{
#if defined(_ROLE_HOST)
    switch (iRtcId)
    {
    case 0: // 0=physical RTC connect over USB
        USBComm_Driver_Elment.Dev_Handle = libusb_open_device_with_vid_pid(USBComm_Driver_Elment.Ctx, RTC_USB_VID, RTC_USB_PID);
        if (USBComm_Driver_Elment.Dev_Handle == NULL)
        {
            // printf("USBComm_Driver_Open_USB_Device fail\n");
            return USBCOMM_ERROR_CAN_NOT_OPEN_DEVICE;
        }
        return USBCOMM_SUCCESS;

    case 1: // 1=virtual RTC connect over TCP
        int res;

        memset(&gtHints, 0, sizeof(gtHints));
        gtHints.ai_family = AF_UNSPEC;
        gtHints.ai_socktype = SOCK_STREAM;
        gtHints.ai_protocol = IPPROTO_TCP;

        // Resolve the server address and port
        res = getaddrinfo(gszDefaultIpv4, gszRtcPort, &gtHints, &gptResult);
        if (res != 0)
        {
            return USBCOMM_ERROR_CAN_NOT_OPEN_DEVICE;
        }
        return USBCOMM_SUCCESS;

    default:
        return ERR_UNKNOWN_DEVICE;
    }
#endif

#if defined(_ROLE_VIRTUAL_RTC)
    int iReturn;

    switch (iRtcId)
    {
    case 1: // 1=virtual RTC connect over TCP
        memset(&gtHints, 0, sizeof(gtHints));
        gtHints.ai_family = AF_UNSPEC;
        gtHints.ai_socktype = SOCK_STREAM;
        gtHints.ai_protocol = IPPROTO_TCP;

        // Resolve the server address and port
        iReturn = getaddrinfo(gszDefaultIpv4, gszVirtualRtcPort, &gtHints, &gptResult);
        if (iReturn != 0)
        {
            return USBCOMM_ERROR_CAN_NOT_OPEN_DEVICE;
        }
        return USBCOMM_SUCCESS;

    default:
        return ERR_UNKNOWN_DEVICE;
    }
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
int USBComm_Driver_Is_Kernel_Driver_Active(int iRtcId)
{
    int res;
#if defined(_ROLE_HOST)
    switch (iRtcId)
    {
    case 0: // 0=physical RTC connect over USB
        res = libusb_kernel_driver_active(USBComm_Driver_Elment.Dev_Handle, USBComm_rtc_descriptor.i_bInterfaceNumber);
        printf("i_bInterfaceNumber:%d,libusb_kernel_driver_active:%d\n", USBComm_rtc_descriptor.i_bInterfaceNumber, res);
        return res;
    case 1: // 1=virtual RTC connect over TCP
        return USBCOMM_SUCCESS;

    default:
        return ERR_UNKNOWN_DEVICE;
    }
#endif

#if defined(_ROLE_VIRTUAL_RTC)
    switch (iRtcId)
    {
    case 1: // 1=virtual RTC connect over TCP
        return USBCOMM_SUCCESS;

    default:
        return ERR_UNKNOWN_DEVICE;
    }
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
int USBComm_Driver_Detach_Kernel_Driver(int iRtcId)
{
#if defined(_ROLE_HOST)
    switch (iRtcId)
    {
    case 0:                                                                                                              // 0=physical RTC connect over USB
        return libusb_detach_kernel_driver(USBComm_Driver_Elment.Dev_Handle, USBComm_rtc_descriptor.i_bInterfaceNumber); // detach kernel driver

    case 1: // 1=virtual RTC connect over TCP
        return USBCOMM_SUCCESS;

    default:
        return ERR_UNKNOWN_DEVICE;
    }
#endif

#if defined(_ROLE_VIRTUAL_RTC)
    switch (iRtcId)
    {
    case 1: // 1=virtual RTC connect over TCP
        return USBCOMM_SUCCESS;

    default:
        return ERR_UNKNOWN_DEVICE;
    }
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
int USBComm_Driver_Release_Interface(int iRtcId)
{
#if defined(_ROLE_HOST)
    switch (iRtcId)
    {
    case 0:                                                                                                           // 0=physical RTC connect over USB
        return libusb_release_interface(USBComm_Driver_Elment.Dev_Handle, USBComm_rtc_descriptor.i_bInterfaceNumber); // release the claimed interface

    case 1: // 1=virtual RTC connect over TCP
        return USBCOMM_SUCCESS;

    default:
        return ERR_UNKNOWN_DEVICE;
    }
#endif

#if defined(_ROLE_VIRTUAL_RTC)
    switch (iRtcId)
    {
    case 1: // 1=virtual RTC connect over TCP
        return USBCOMM_SUCCESS;

    default:
        return ERR_UNKNOWN_DEVICE;
    }
#endif
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
int USBComm_Driver_Claim_Interface(int iRtcId)
{
    int res;
#if defined(_ROLE_HOST)
    switch (iRtcId)
    {
    case 0: // 0=physical RTC connect over USB
        res = libusb_claim_interface(USBComm_Driver_Elment.Dev_Handle, USBComm_rtc_descriptor.i_bInterfaceNumber);
        // printf("libusb_claim_interface:%d\n", res);
        return res; // claim interface 0 (the first) of device (mine had just 1)

    case 1: // 1=virtual RTC connect over TCP
        int iReturn;
        struct addrinfo *tAddr;
        // Attempt to connect to an address until one succeeds
        for (tAddr = gptResult; tAddr != NULL; tAddr = tAddr->ai_next)
        {
            // Create a SOCKET for connecting to server
            giSocketForRtc = socket(tAddr->ai_family, tAddr->ai_socktype, tAddr->ai_protocol);
            if (giSocketForRtc < 0)
            {
                printf("create socket error: %s(errno: %d)\n", strerror(errno), errno);
                return __LINE__;
            }
            // Connect to server.
            iReturn = connect(giSocketForRtc, tAddr->ai_addr, (int)tAddr->ai_addrlen);
            if (iReturn < 0)
            {
                printf("connect error: %s(errno: %d)\n", strerror(errno), errno);
                // closes(giSocketForRtc);
                // giSocketForRtc = 0;
                continue;
            }
            break;
        }

        freeaddrinfo(gptResult);

        if (iReturn < 0)
        {
            printf("Unable to connect to RTC (check if RTC running!)\n");
            return ERR_FAIL_TO_CONNECT;
        }

        return USBCOMM_SUCCESS;

    default:
        return ERR_UNKNOWN_DEVICE;
    }
#endif

#if defined(_ROLE_VIRTUAL_RTC)
    int iReturn;
    int iTryBindCount;

    switch (iRtcId)
    {
    case 1: // 1=virtual RTC connect over TCP
        // Create a SOCKET for connecting to server
        giSocket = socket(gptResult->ai_family, gptResult->ai_socktype, gptResult->ai_protocol);
        if (giSocket < 0)
        {
            // printf("socket failed with error: %ld\n", WSAGetLastError());
            freeaddrinfo(gptResult);
            return __LINE__;
        }
        int reuse = 1;
        if (setsockopt(giSocket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
        {
            // perror(“setsockopet error\n”);
            return -1;
        }
        // Setup the TCP listening socket
        iTryBindCount = 50;
        while ((iReturn = bind(giSocket, gptResult->ai_addr, (int)gptResult->ai_addrlen)) < 0 && iTryBindCount > 0)
        {
            printf("bind error: %s\n", strerror(errno));
            fflush(stdout);
            iTryBindCount--;
            usleep(10000);
        }
        if (iTryBindCount <= 0)
        {
            // printf("bind failed with error: %d\n", WSAGetLastError());
            freeaddrinfo(gptResult);
            close(giSocket);
            giSocket = 0;
            return ERR_FAIL_TO_BIND;
        }

        freeaddrinfo(gptResult);

        iReturn = listen(giSocket, 1); // SOMAXCONN);
        if (iReturn < 0)
        {
            // printf("listen failed with error: %d\n", WSAGetLastError());
            close(giSocket);
            giSocket = 0;
            return ERR_FAIL_TO_LISTEN;
        }

        // Accept a client socket
        giClientSocket = accept(giSocket, NULL, NULL);
        if (giClientSocket < 0)
        {
            // printf("accept failed with error: %d\n", WSAGetLastError());
            close(giSocket);
            giSocket = 0;
            return ERR_FAIL_TO_CONNECT;
        }

        // No longer need server socket ??????????????????????????????
        // close(giListenSocket);
        return USBCOMM_SUCCESS;

    default:
        return ERR_UNKNOWN_DEVICE;
    }
#endif
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
void USBComm_Driver_Close_USB_Device(int iRtcId)
{
#if defined(_ROLE_HOST)
    switch (iRtcId)
    {
    case 0: // 0=physical RTC connect over USB
        libusb_reset_device(USBComm_Driver_Elment.Dev_Handle);
        libusb_close(USBComm_Driver_Elment.Dev_Handle); // close the USB device
        return;

    case 1: // 1=virtual RTC connect over TCP
        if (giSocketForRtc)
        {
            close(giSocketForRtc);
            giSocketForRtc = 0;
        }
        return;

    default:
        return;
    }
#endif

#if defined(_ROLE_VIRTUAL_RTC)
    switch (iRtcId)
    {
    case 1: // 1=virtual RTC connect over TCP
        if (giSocket)
        {
            close(giSocket);
            giSocket = 0;
        }
        if (giClientSocket)
        {
            close(giClientSocket);
            giClientSocket = 0;
        }
        return;

    default:
        return;
    }
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
void USBComm_Driver_Exit(int iRtcId)
{
#if defined(_ROLE_HOST)
    switch (iRtcId)
    {
    case 0:                                     // 0=physical RTC connect over USB
        libusb_exit(USBComm_Driver_Elment.Ctx); // exit libusb
        return;

    case 1: // 1=virtual RTC connect over TCP
        return;

    default:
        return;
    }
#endif

#if defined(_ROLE_VIRTUAL_RTC)
    switch (iRtcId)
    {
    case 1: // 1=virtual RTC connect over TCP
        return;

    default:
        return;
    }
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
// int giMessageBalence;
int USBComm_Driver_Bulk_Send_Message(int iRtcId, ONS_BYTE *outgoing_message, int message_size, int *actual)
{
#if defined(_ROLE_HOST)
    switch (iRtcId)
    {
    case 0: // 0=physical RTC connect over USB
        return libusb_bulk_transfer(USBComm_Driver_Elment.Dev_Handle, (RTC_USB_EP_OUT | LIBUSB_ENDPOINT_OUT),
                                    outgoing_message, message_size, actual, BULK_OUT_TIMEOUT);

    case 1: // 1=virtual RTC connect over TCP
        int iReturn;

        iReturn = send(giSocketForRtc, outgoing_message, message_size, 0);
        if (iReturn < 0)
        {
            printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
            return ERR_FAIL_TO_SEND;
        }
        *actual = iReturn;
        return USBCOMM_SUCCESS;

    default:
        return ERR_UNKNOWN_DEVICE;
    }
#endif

#if defined(_ROLE_VIRTUAL_RTC)
    int iReturn;
    static uint32_t uSendCount = 0;

    switch (iRtcId)
    {
    case 1: // 1=virtual RTC connect over TCP
        if (giClientSocket)
        {
            iReturn = send(giClientSocket, outgoing_message, message_size, 0);
            if (iReturn < 0)
            {
                // printf("send failed with error: %d\n", WSAGetLastError());
                close(giClientSocket);
                giClientSocket = 0;
                return iReturn;
            }
            AddBulkLog(false, *(INT_16 *)outgoing_message, uSendCount++);
            // printf("send(ID=%d, size=%d, count=%u)\n", *(INT_16*)outgoing_message, iReturn, uSendCount++);
        }
        else
        {
            iReturn = ERR_FAIL_TO_SEND;
        }
        *actual = iReturn;
        return USBCOMM_SUCCESS;

    default:
        return ERR_UNKNOWN_DEVICE;
    }
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
int USBComm_Driver_Bulk_Receive_Message(int iRtcId, ONS_BYTE *incomming_message, int *actual)
{
#if defined(_ROLE_HOST)
    Message_Header_t *ptHeader;
    int iRemainingByte;
    struct timeval tv;
    struct timezone tz;
    unsigned long uTimeBeginMs, uTimeNowMs;

    switch (iRtcId)
    {
    case 0: // 0=physical RTC connect over USB
        return libusb_bulk_transfer(USBComm_Driver_Elment.Dev_Handle, (RTC_USB_EP_IN | LIBUSB_ENDPOINT_IN),
                                    incomming_message, USBCOMM_MAX_RX_MESSAGE_SIZE, actual, BULK_IN_TIMEOUT);

    case 1: // 1=virtual RTC connect over TCP
        /*  透過TCP接收封包會做的這麼複雜, 是因為TCP發送時, 不會等待接收方完成接收動作, 而是逕自發送出資料, 資料會存放在接收端的buffer理.
            當接收端接收資料時, 會一次把Buffer裡的資料收下, 不會以發送方的發送動作做為區隔.
            例如發送方第一次發送 12 bytes, 第二次發送 18 bytes, 此時接收端接收一次資料, 則接收到 30 bytes 的資料.
            沒法區分接收的資料裡, 哪些是第一次發送的, 哪些是第二次發送的.
            所以此處接收資料時, 先接收表頭, 表頭裡有一個封包的大小資訊, 再依此接收封包剩餘的內容.
        */
        int iReturn;
        *actual = 0;
        iReturn = recv(giSocketForRtc, incomming_message, sizeof(Message_Header_t), 0);

        if (iReturn == 0)
        {
            printf("Virtual RTC closed the connection. Driver ends immediately.\n");
            _exit(0);
            // return USBCOMM_ERROR_CONNECTION_FAIL;
        }
        if (iReturn != sizeof(Message_Header_t))
        {
            printf("recv msg error: %s(errno: %d)\n", strerror(errno), errno);
            return iReturn;
        }
        ptHeader = (Message_Header_t *)incomming_message;
        if ((unsigned long)(ptHeader->Size) < sizeof(Message_Header_t) || (ptHeader->Size) >= USBCOMM_MAX_RX_MESSAGE_SIZE)
        {
            // flush receive buffer if size of message is wrong.
            char acTemp[8192];
            recv(giSocketForRtc, acTemp, sizeof(acTemp), 0);
            printf("recv msg error: size=%d\n", ptHeader->Size);
            return ERR_INVALID_LENGTH;
        }

        iRemainingByte = (ptHeader->Size) - sizeof(Message_Header_t);

        gettimeofday(&tv, &tz);
        uTimeBeginMs = tv.tv_sec * 1000;
        uTimeBeginMs += tv.tv_usec / 1000;

        while (iRemainingByte)
        {
            iReturn = recv(giSocketForRtc, incomming_message + ptHeader->Size - iRemainingByte, iRemainingByte, 0);
            if (iReturn == 0)
            {
                printf("Virtual RTC closed the connection. Driver ends immediately.\n");
                _exit(0);
                // return USBCOMM_ERROR_CONNECTION_FAIL;
            }
            if (iReturn < 0)
            {
                printf("recv msg error: %s(errno: %d)\n", strerror(errno), errno);
                return iReturn;
            }
            iRemainingByte = iRemainingByte - iReturn;

            gettimeofday(&tv, &tz);
            uTimeNowMs = tv.tv_sec * 1000;
            uTimeNowMs += tv.tv_usec / 1000;
            if ((uTimeNowMs - uTimeBeginMs) > 1000)
            {
                printf("recv msg error: time out.\n");
                return ERR_TIME_OUT;
            }
        }
        *actual = ptHeader->Size;
        return USBCOMM_SUCCESS;

    default:
        return ERR_UNKNOWN_DEVICE;
    } // end of switch( iRtcId)
#endif

#if defined(_ROLE_VIRTUAL_RTC)
    static uint32_t uRecvCount = 0;
    Message_Header_t *ptHeader;
    int iRemainingByte;
    struct timeval tv;
    struct timezone tz;
    unsigned long uTimeBeginMs, uTimeNowMs;
    /*  透過TCP接收封包會做的這麼複雜, 是因為TCP發送時, 不會等待接收方完成接收動作, 而是逕自發送出資料, 資料會存放在接收端的buffer理.
        當接收端接收資料時, 會一次把Buffer裡的資料收下, 不會以發送方的發送動作做為區隔.
        例如發送方第一次發送 12 bytes, 第二次發送 18 bytes, 此時接收端接收一次資料, 則接收到 30 bytes 的資料.
        沒法區分接收的資料裡, 哪些是第一次發送的, 哪些是第二次發送的.
        所以此處接收資料時, 先接收表頭, 表頭裡有一個封包的大小資訊, 再依此接收封包剩餘的內容.
    */
    switch (iRtcId)
    {
    case 1: // 1=virtual RTC connect over TCP
        *actual = 0;
        if (giClientSocket > 0)
        {
            int iReturn;
            iReturn = recv(giClientSocket, incomming_message, sizeof(Message_Header_t), 0);

            if (iReturn == 0)
            {
                if (giClientSocket)
                {
                    close(giClientSocket);
                }
                giClientSocket = accept(giSocket, NULL, NULL);
                if (giClientSocket < 0)
                {
                    // printf("accept failed with error: %d\n", WSAGetLastError());
                    giClientSocket = 0;
                    *actual = 0;
                    return -1;
                }
                *actual = 0;
                return USBCOMM_SUCCESS;
            }
            if (iReturn != sizeof(Message_Header_t))
            {
                printf("recv msg error: %s(errno: %d)\n", strerror(errno), errno);
                return iReturn;
            }
            ptHeader = (Message_Header_t *)incomming_message;
            if ((unsigned long)(ptHeader->Size) < sizeof(Message_Header_t) || (ptHeader->Size) >= USBCOMM_MAX_RX_MESSAGE_SIZE)
            {
                // flush receive buffer if size of message is wrong.
                char acTemp[8192];
                recv(giClientSocket, acTemp, sizeof(acTemp), 0);
                printf("recv msg error: size=%d\n", ptHeader->Size);
                return ERR_INVALID_LENGTH;
            }

            iRemainingByte = (ptHeader->Size) - sizeof(Message_Header_t);

            gettimeofday(&tv, &tz);
            uTimeBeginMs = tv.tv_sec * 1000;
            uTimeBeginMs += tv.tv_usec / 1000;

            while (iRemainingByte)
            {
                iReturn = recv(giClientSocket, incomming_message + sizeof(Message_Header_t) + ptHeader->Size - iRemainingByte, iRemainingByte, 0);
                if (iReturn == 0)
                {
                    printf("Virtual RTC closed the connection. Driver ends immediately.\n");
                    _exit(0);
                    // return USBCOMM_ERROR_CONNECTION_FAIL;
                }
                if (iReturn < 0)
                {
                    printf("recv msg error: %s(errno: %d)\n", strerror(errno), errno);
                    return iReturn;
                }
                iRemainingByte = iRemainingByte - iReturn;

                gettimeofday(&tv, &tz);
                uTimeNowMs = tv.tv_sec * 1000;
                uTimeNowMs += tv.tv_usec / 1000;
                if ((uTimeNowMs - uTimeBeginMs) > 1000)
                {
                    printf("recv msg error: time out.\n");
                    return ERR_TIME_OUT;
                }
            }

            AddBulkLog(true, *(INT_16 *)incomming_message, uRecvCount++);
            printf("recv(Index=%d, ID=%d, size=%d)\n", uRecvCount, *(INT16 *)incomming_message, ptHeader->Size);

            *actual = ptHeader->Size;
            return USBCOMM_SUCCESS;
        }
        return USBCOMM_SUCCESS;

    default:
        return ERR_UNKNOWN_DEVICE;
    } // end of switch( iRtcId)
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
int USBComm_Driver_Tx_Mutex_Lock(void)
{
    while (pthread_mutex_lock(&USBComm_Driver_Elment.Tx_Buffer_Mutex))
    {
        usleep(100);
    }
    return 0;
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
int USBComm_Driver_Rx_Mutex_Lock(void)
{
    while (pthread_mutex_lock(&USBComm_Driver_Elment.Rx_Buffer_Mutex))
    {
        usleep(100);
    }
    return 0;
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
int USBComm_Driver_Tx_Mutex_UNLock(void)
{
    while (pthread_mutex_unlock(&USBComm_Driver_Elment.Tx_Buffer_Mutex))
    {
        usleep(100);
    }
    return 0;
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
int USBComm_Driver_Rx_Mutex_UNLock(void)
{
    while (pthread_mutex_unlock(&USBComm_Driver_Elment.Rx_Buffer_Mutex))
    {
        usleep(100);
    }
    return 0;
}

int USBComm_Driver_Terminate_Tx_Thread (void)
{
    USBComm_Tx_Thread_Terminate();
    if (pthread_join(USBComm_Driver_Elment.USBComm_Tx_Thread, NULL))
        return 0;
    else
        return -1;
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
int USBComm_Driver_Create_Tx_Thread(int iRtcId)
{
#if defined(_ROLE_HOST)
    switch (iRtcId)
    {
    case 0: // 0=physical RTC connect over USB
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

    case 1: // 1=virtual RTC connect over TCP
        int iReturn;

        iReturn = pthread_create(&USBComm_Driver_Elment.USBComm_Tx_Thread, NULL, USBComm_Tx_Thread_Main, NULL);
        if (iReturn != 0)
        {
            return ERR_FAIL_TO_CREATE;
        }
        return iReturn;

    default:
        return ERR_UNKNOWN_DEVICE;
    }
#endif

#if defined(_ROLE_VIRTUAL_RTC)
    int iReturn;

    switch (iRtcId)
    {
    case 1: // 1=virtual RTC connect over TCP
        iReturn = pthread_create(&USBComm_Driver_Elment.USBComm_Tx_Thread, NULL, USBComm_Tx_Thread_Main, NULL);
        if (iReturn != 0)
        {
            return ERR_FAIL_TO_CREATE;
        }
        return iReturn;

    default:
        return ERR_UNKNOWN_DEVICE;
    }
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

int USBComm_Driver_Terminate_Rx_Thread (void)
{
    USBComm_Rx_Thread_Terminate();
    if (pthread_join(USBComm_Driver_Elment.USBComm_Rx_Thread, NULL))
        return 0;
    else
        return -1;
}

int USBComm_Driver_Create_Rx_Thread(int iRtcId)
{
#if defined(_ROLE_HOST)
    switch (iRtcId)
    {
    case 0: // 0=physical RTC connect over USB
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

    case 1: // 1=virtual RTC connect over TCP
        int iReturn;

        iReturn = pthread_create(&USBComm_Driver_Elment.USBComm_Rx_Thread, NULL, USBComm_Rx_Thread_Main, NULL);
        if (iReturn != 0)
        {
            printf("USBComm_Driver_Create_Rx_Thread fail\n");
            return iReturn;
        }
        return iReturn;

    default:
        return ERR_UNKNOWN_DEVICE;
    }
#endif

#if defined(_ROLE_VIRTUAL_RTC)
    int iReturn;

    switch (iRtcId)
    {
    case 1: // 1=virtual RTC connect over TCP
        iReturn = pthread_create(&USBComm_Driver_Elment.USBComm_Rx_Thread, NULL, USBComm_Rx_Thread_Main, NULL);
        if (iReturn != 0)
        {
            printf("USBComm_Driver_Create_Rx_Thread fail\n");
            return iReturn;
        }
        return iReturn;

    default:
        return ERR_UNKNOWN_DEVICE;
    }
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
#include <string.h>
#include <errno.h>
int USBComm_Driver_Tx_Thread_kill(int iRtcId)
{
#if defined(_ROLE_HOST)
    void *tret;

    switch (iRtcId)
    {
    case 0: // 0=physical RTC connect over USB
        // return pthread_kill(USBComm_Driver_Elment.USBComm_Tx_Thread, 1);
        pthread_cancel(USBComm_Driver_Elment.USBComm_Tx_Thread);
        pthread_join(USBComm_Driver_Elment.USBComm_Tx_Thread, &tret);
        return USBCOMM_SUCCESS;

    case 1: // 1=virtual RTC connect over TCP
        // return pthread_kill(USBComm_Driver_Elment.USBComm_Tx_Thread, 1);
        pthread_cancel(USBComm_Driver_Elment.USBComm_Tx_Thread);
        pthread_join(USBComm_Driver_Elment.USBComm_Tx_Thread, &tret);
        return USBCOMM_SUCCESS;

    default:
        return ERR_UNKNOWN_DEVICE;
    }
#endif

#if defined(_ROLE_VIRTUAL_RTC)
    void *tret;

    switch (iRtcId)
    {
    case 1: // 1=virtual RTC connect over TCP
        pthread_cancel(USBComm_Driver_Elment.USBComm_Tx_Thread);
        pthread_join(USBComm_Driver_Elment.USBComm_Tx_Thread, &tret);
        return USBCOMM_SUCCESS;

    default:
        return ERR_UNKNOWN_DEVICE;
    }
    /*
    iReturn = pthread_kill(USBComm_Driver_Elment.USBComm_Tx_Thread, 0); // ESRCH = 執行緒不存在
    if ( iReturn==0 || iReturn==ESRCH ){
        return 0;
        //printf("pthread_kill error: %s\n", strerror(errno));fflush(stdout);
    }
    return iReturn;*/
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
int USBComm_Driver_Rx_Thread_kill(int iRtcId)
{
#if defined(_ROLE_HOST)
    void *tret;

    switch (iRtcId)
    {
    case 0: // 0=physical RTC connect over USB
        // return pthread_kill(USBComm_Driver_Elment.USBComm_Rx_Thread, 1);
        pthread_cancel(USBComm_Driver_Elment.USBComm_Rx_Thread);
        pthread_join(USBComm_Driver_Elment.USBComm_Rx_Thread, &tret);
        return USBCOMM_SUCCESS;

    case 1: // 1=virtual RTC connect over TCP
        // return pthread_kill(USBComm_Driver_Elment.USBComm_Rx_Thread, 1);
        pthread_cancel(USBComm_Driver_Elment.USBComm_Rx_Thread);
        pthread_join(USBComm_Driver_Elment.USBComm_Rx_Thread, &tret);
        return USBCOMM_SUCCESS;

    default:
        return ERR_UNKNOWN_DEVICE;
    }
#endif

#if defined(_ROLE_VIRTUAL_RTC)
    void *tret;

    switch (iRtcId)
    {
    case 1: // 1=virtual RTC connect over TCP
        pthread_cancel(USBComm_Driver_Elment.USBComm_Rx_Thread);
        pthread_join(USBComm_Driver_Elment.USBComm_Rx_Thread, &tret);
        return USBCOMM_SUCCESS;

    default:
        return ERR_UNKNOWN_DEVICE;
    }
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
    usleep(Time_Millisecond * 1000); // Translate time to microseconds
}

#if defined(ONS_RTC) || defined(_ROLE_VIRTUAL_RTC)
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
    (void)event; // avoid compiler unused-warning
    (void)pdata;
    (void)size; // avoid compiler unused-warning

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
/*int USBCheckDeviceStatus ( void )
{
    if ( ( !( USBDeviceState & CONFIGURED_STATE ) ) || ( USBSuspendControl == 1 ) || ( USBComm_Rtc_Err_Flag == 1 ) )
    {
        return USBCOMM_ERROR_MODULE_IN_ERROR_STATE;
    }
    return USBCOMM_SUCCESS;
}
*/
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

// ---------------------------------------------------------------------------
//  Function name		: USBComm_Driver_Find_RTC_Device
//  Purpose				: Find RTC device in usb_list.
//  Inputs				: RTC_USB_VID,RTC_USB_PID
//  Outputs				: string of Product, Manufature.
//  Returns				: 0 on success, or -1
//  Description			: Host -  find a spicific usb device.
// ---------------------------------------------------------------------------
int USBComm_Driver_Find_RTC_Device(int iRtcId, char **str_product, char **str_manufature)
{
#if defined(_ROLE_HOST)
    libusb_device **usbDevices;
    libusb_device_descriptor device_desc;
    libusb_config_descriptor *config_desc;

    libusb_interface_descriptor *interface_desc;
    libusb_device_handle *usb_hdl;
    int res = -1;
    unsigned char str_iProduct[200], str_iManufacturer[200];
    int numDevices;

    switch (iRtcId)
    {
    case 0: // 0=physical RTC connect over USB
        numDevices = libusb_get_device_list(NULL, &usbDevices);
        if (numDevices < 1)
            return -1;
        for (int i = 0; i < numDevices; ++i)
        {
            int r = libusb_get_device_descriptor(usbDevices[i], &device_desc);
            if (r < 0)
                continue;
            r = libusb_get_config_descriptor(usbDevices[i], 0, &config_desc);
            if (r < 0)
                continue;

            libusb_open(usbDevices[i], &usb_hdl);
            libusb_get_string_descriptor_ascii(usb_hdl, device_desc.iProduct, str_iProduct, 200); // this api real get usb_product info from usblib_firmware.
            libusb_get_string_descriptor_ascii(usb_hdl, device_desc.iManufacturer, str_iManufacturer, 200);

            // printf("idVendor:%04x, idProduct:%04x\n", device_desc.idVendor, device_desc.idProduct);
            // printf("iManufature:%s, iProduct:%s\n", str_iManufacturer, str_iProduct);
            // printf("\n");
            libusb_close(usb_hdl);
            if (device_desc.idVendor == RTC_USB_VID && device_desc.idProduct == RTC_USB_PID)
            {
                *str_product = (char *)malloc(sizeof(char) * 48);
                *str_manufature = (char *)malloc(sizeof(char) * 48);
                strcpy(*str_product, (char *)str_iProduct);
                strcpy(*str_manufature, (char *)str_iManufacturer);
                strcpy(USBComm_rtc_descriptor.str_iProduct, (char *)str_iProduct);
                strcpy(USBComm_rtc_descriptor.str_iManufacturer, (char *)str_iManufacturer);
                // USBComm_rtc_descriptor.i_bInterfaceNumber = device_desc.

                // printf("bLength:%d\n",config_desc->bLength);
                // printf("bDescriptorType:%d\n",config_desc->bDescriptorType);
                // printf("wTotalLength:%d\n",config_desc->wTotalLength);
                // printf("bNumInterfaces:%d\n",config_desc->bNumInterfaces);
                for (int i = 0; i < config_desc->bNumInterfaces; i++)
                {
                    USBComm_rtc_descriptor.i_bInterfaceNumber = config_desc->interface[i].altsetting[0].bInterfaceNumber;
                    USBComm_rtc_descriptor.i_bNumEndpoints = config_desc->interface[i].altsetting[0].bNumEndpoints;
                }
                // printf("bInterfaceNumber:%d\n",USBComm_rtc_descriptor.i_bInterfaceNumber);
                // printf("bNumEndpoints:%d\n",USBComm_rtc_descriptor.i_bNumEndpoints);
                res = 0;
            }
        }
        return res;

    case 1: // 1=virtual RTC connect over TCP
        return USBCOMM_SUCCESS;
    default:
        return ERR_UNKNOWN_DEVICE;
    }
#endif

#if defined(_ROLE_VIRTUAL_RTC)
    return USBCOMM_SUCCESS;
#endif

    /*
    printf("numPIC:%d\n", numPIC);
    //success = libusb_reset_device(devh);
    libusb_open(usbDevices[numPIC], &usb_hdl);
    int res = libusb_reset_device(usb_hdl);
    printf("res:%d\n", res);
    if (res == 0)
    {
        printf("success;\n");
    }
    else
    {
        char str_err[64];
        strcpy(str_err, libusb_strerror((libusb_error)res));
        printf("str_err:%s\n", str_err);
    }
*/
}

//   char str_iProduct[48];
//   char str_iManufacturer[48];
//   int i_bInterfaceNumber;