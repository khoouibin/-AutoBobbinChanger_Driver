// ---------------------------------------------------------------------------
//  Filename: USBComm.c
//  Created by: Jack (the best looking one among all Jacks)
//  Date:  21/07/2020
//  Orisol (c)
//  Note: This version is modified from the previous version for Host and virtual RTC on Linux "only".
//  It does NOT work for physical RTC (PIC) and is not able to compile on Windows.
//  On Linux, UHF-host and virtual RTC share the code. Once anyone has to modify the code, please sync code between UHF-host and virtual RTC.
// ---------------------------------------------------------------------------

#ifndef _USBCOMM_H_
#define _USBCOMM_H_

#include "Definition.h"
#include "ONS_General.h"

#ifdef __cplusplus
extern "C" {
#endif

#define USBCOMM_VERSION				"USBCOMM  0.0.3"


#ifdef ONS_PC_PLAT 
#define USBCOMM_MAX_TX_MESSAGE_SIZE  64		//The maximum size for transmit massage in ONS_BYTEs.
#define USBCOMM_TX_BUFFER_SIZE       20		//The maximum size for transmit buffer.
#define USBCOMM_MAX_RX_MESSAGE_SIZE  64		//The maximum size for receive massage in ONS_BYTEs.
#define USBCOMM_RX_BUFFER_SIZE       20		//The maximum size for receive buffer.
#endif


#ifdef ONS_RTC
#define USBCOMM_MAX_TX_MESSAGE_SIZE  64		//The maximum size for transmit massage in ONS_BYTEs.
#define USBCOMM_TX_BUFFER_SIZE       8		//The maximum size for transmit buffer.
#define USBCOMM_MAX_RX_MESSAGE_SIZE  64		//The maximum size for receive massage in ONS_BYTEs.
#define USBCOMM_RX_BUFFER_SIZE       8		//The maximum size for receive buffer.
#endif


//USBComm Module Error Code Definition
typedef enum
{
	 USBCOMM_SUCCESS						=	0,
	 USBCOMM_ERROR_BUFFER_SIZE				=	1,
	 USBCOMM_ERROR_BUFFER_FULL				=	2,
	 USBCOMM_ERROR_MESSAGE_SIZE				=	3,
	 USBCOMM_ERROR_MODULE_IN_ERROR_STATE	=	4,
	 USBCOMM_ERROR_CAN_NOT_OPEN_DEVICE  	=	5,
	 USBCOMM_ERROR_RETRY       				=	6,
	 USBCOMM_ERROR_MODULE_NOT_READY 	    =	7
}
	USBComm_Error_Code_t;

#define USBCOMM_ERROR_CONNECTION_FAIL  0xDEAD


typedef enum
{
	 USBCOMM_INIT_STATE					=	0,
	 USBCOMM_READY_STATE				=	1,
	 USBCOMM_ERROR_STATE				=	2
}
	USBComm_State_code_t;


typedef ONS_BYTE*	USBComm_Buffer_t;

typedef struct
{
	int				RX_Buffer_Full;					// flag to indicate buffer is full.
	unsigned long	RX_Current_Lost_Msgs;			// number of messages lost currently.
	unsigned long	RX_Total_Lost_Msgs;				// total number of messages lost since last session started
	unsigned long	RX_Total_Msgs;					// total number of messages received from USB (including the lost ones).
	unsigned long	Rx_Total_Data_Size;				// total size of received data in ONS_BYTEs
	unsigned long	Rx_Num_Of_Retry;				// total number of time entered  to the retry phase in Rx.
	int				USBComm_Module_State;			// flag to indicate module one of this modes (USBComm_State_code_t): INIT,READY,ERROR
	int				TX_Buffer_Epmty;				// flag to indicate buffer is Empty.
	unsigned long	TX_Current_Buffer_Full_Msgs;	// number of Tx buffer full error currently.
	unsigned long	TX_Total_Buffer_Full_Msgs;		// total number of Tx buffer full error since last session started
	unsigned long	TX_Total_Msgs;					// total number of messages transmitted via USB.
	unsigned long	Tx_Total_Data_Size;				// total size of transmitted data in ONS_BYTEs
	unsigned long	Tx_Total_Buffer_empty;			// total number of accessing empty buffer
	unsigned long	Tx_Num_Of_Retry;				// total number of time entered  to the retry phase in Tx.
	int             libusb_error_code;              // enum libusb_error
	char            libusb_strerror[64];            // strerror list.
}
	USBComm_Status_t;


/*enum libusb_error {
    LIBUSB_SUCCESS             = 0,
    LIBUSB_ERROR_IO            = -1,
    LIBUSB_ERROR_INVALID_PARAM = -2,
    LIBUSB_ERROR_ACCESS        = -3,
    LIBUSB_ERROR_NO_DEVICE     = -4,
    LIBUSB_ERROR_NOT_FOUND     = -5,
    LIBUSB_ERROR_BUSY          = -6,
    LIBUSB_ERROR_TIMEOUT       = -7,
    LIBUSB_ERROR_OVERFLOW      = -8,
    LIBUSB_ERROR_PIPE          = -9,
    LIBUSB_ERROR_INTERRUPTED   = -10,
    LIBUSB_ERROR_NO_MEM        = -11,
    LIBUSB_ERROR_NOT_SUPPORTED = -12,
    LIBUSB_ERROR_OTHER         = -99,
};*/ // define in usb.h

// ----------------
//   Prototypes
//------------------
int  USBComm_Init 				( int iDevice );
int  USBComm_Termination 		( void );
int  USBComm_Send_Message 		( USBComm_Buffer_t Message , int Message_Size );
int  USBComm_Is_Tx_Buffer_Full 	( void );
int  USBComm_Get_Message 		( USBComm_Buffer_t Message , int Buffer_Size , int* New_Message_Received, int* Message_Size );
int  USBComm_Peek_Message		( USBComm_Buffer_t* Message , unsigned int* New_Message_Received, unsigned int* Message_Size );
int  USBComm_Is_Rx_Buffer_Empty ( void );
void USBComm_Get_Status 		( USBComm_Status_t* Status );
int  USBComm_Remove_Rx_Message  (void);
void USBComm_Get_Module_State	( int* Module_State );
void USBComm_Set_Init_Complete	( void );
const char* USBComm_Get_Version			( void );
void USBComm_Print_Rx_Buffer_Data(void);
void USBComm_Set_System_In_Termination_State(int Value);
int  USBComm_Is_System_In_Termination_State(void);
int  Is_USBCOMM_Termination_Required(void);

void* USBComm_Rx_Thread_Main(void*);
void* USBComm_Tx_Thread_Main(void*);

void USBComm_Rx_Thread_Terminate();
void USBComm_Tx_Thread_Terminate();

#ifdef ONS_RTC
void USBComm_Rx_Int_Main ( void );
void USBComm_Tx_Int_Main ( void );
#endif

#ifdef USE_SYSLOG
void USBComm_Print_Current_Status_to_log(void);
#endif

#ifdef __cplusplus
}
#endif

#endif //_USBCOMM_H_
