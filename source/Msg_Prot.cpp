// ---------------------------------------------------------------------------
//  Filename: Msg_Port.c
//  Created by: Nissim Avichail
//  Date:  06/07/15
//  Orisol (c)
// ---------------------------------------------------------------------------

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "Definition.h"
#include "ONS_General.h"
#include "RTC_Stitch.h"
#include "Timers.h"

// #ifdef ONS_PC_PLAT
// #include "Debug.h"
// #endif

#ifdef ONS_WIN
#include <windows.h>
#include <iostream>
#endif

#ifdef ONS_HOST_LINUX
#include <sys/time.h>

#ifdef USE_SYSLOG
#include "ONS_Syslog.h"				//ONS_SYSLOG
#endif //#ifdef USE_SYSLOG

#endif

#ifdef ONS_WIN
#define __func__   __FUNCTION__
#endif
#ifdef ONS_RTC
#include "Timers.h"
#include "Ons_General.h"
#include "RTC_Stitch.h"
#endif

#include "Msg_Prot.h"
#include "USBComm.h"

 // -----------------
// Internal Define
// -----------------

// ---------------------
// internal Prototypes
// ---------------------
static unsigned long Msg_Prot_Checksum_calculator ( ONS_BYTE* Data , unsigned int Size );
static unsigned long Msg_Prot_Get_Msg_SN ( void );


//---------------------
// internal variables
// --------------------
static 	unsigned long			gMsg_SN;	 			// Message S/N this number counter.
static 	Message_Header_t		Last_Msg_Hdr;

#ifdef TESTING_MODE
unsigned long				Rcv_Time;
#endif

#define MAX_SYSLOG_DESC_SIZE 256
#define LOG_INFO_LEVEL 5
#define LOG_CATEGORY_RTC_COMM 0

#define __FILENAME__ "log.txt"

// ------------------------------
//  API Function Implementation
// ------------------------------
// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Init
//
//  Purpose				: Module initialization
//
//  Inputs				:
//						:
//
//  Outputs				:
//
//  Returns				: Success or Error code
//
//  Description			: Module initialization
//
// ---------------------------------------------------------------------------
void Msg_Prot_Init (void)
{
	gMsg_SN = 0;

	Last_Msg_Hdr.Id = NO_MESSAGE_ID;

#ifdef TESTING_MODE
	Rcv_Time = 0;
#endif

}


// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Send_Message
//
//  Purpose				: send message to USB
//
//  Inputs				: Msg_Buff - pointer to transmit message Buffer .
//						: Msg_Size - size of given message buffer
//
//  Outputs				:
//
//  Returns				: Success or Error code
//
//  Description			: Send message to USB
//
// ---------------------------------------------------------------------------
int Msg_Prot_Send_Message (ONS_BYTE* Mgs_Data_Buff, unsigned int Msg_Id, unsigned int Msg_Data_Size)
{

	Generic_Message_t tmp_message;

	int res;

	// update all message filed
	memcpy(tmp_message.Data,Mgs_Data_Buff,Msg_Data_Size);

	tmp_message.Message_Header.Id = Msg_Id;

	tmp_message.Message_Header.Serial_Number = Msg_Prot_Get_Msg_SN();

	tmp_message.Message_Header.Size = Msg_Data_Size + sizeof(tmp_message.Message_Header);   // the size

#ifdef ONS_HOST

	//tmp_message.Message_Header.Time_Stamp = Debug_Get_Time_in_Milliseconds();

#endif

#ifdef ONS_RTC

    tmp_message.Message_Header.Time_Stamp = SysTimer_GetTimeInMiliSeconds();

#endif

	res = USBComm_Send_Message ((USBComm_Buffer_t)&tmp_message , tmp_message.Message_Header.Size );

	return res;
}



// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Receive_Next_Message_From_USB
//
//  Purpose				: get new message from USB RX buffer
//
//  Inputs				: Data_Buff - Buffer for received data.
//
//
//  Outputs				: Msg_Id    - message identification or NO_MESSAGE_ID if there is no message exist in buffer .
//						: New_Msg   -  the sender have to check this to determine if new message received
//						:			   1 - if message exist return 1
//						: 			   0 - No message received.
//
//  Returns				: Success or Error code
//
//  Description			: Get the new message from USB receive buffer
//						: update the buffer data, message id and new message fields,
//						: return success or relevant error code.
//
// ---------------------------------------------------------------------------
int Msg_Prot_Receive_Next_Message (Rcv_Msg_Data_t Data_Buff, unsigned int* Msg_Id, unsigned int* New_Msg)
{

	unsigned int 		msg_size;

	Generic_Message_t	generic_msg;
	Generic_Message_t*	msg_p;
 	int			res;

#ifdef TESTING_MODE

	struct timeval  	tv;

#endif
	msg_p = &generic_msg;
	res = USBComm_Peek_Message( (ONS_BYTE**)&msg_p, New_Msg, &msg_size );

	if (res != (int)USBCOMM_SUCCESS )
	{

		return (res);

	}

	if (!(*New_Msg))
	{
		*Msg_Id = NO_MESSAGE_ID;

		return (MSG_PROT_SUCCESS);
	}

#ifdef TESTING_MODE

	gettimeofday(&tv,NULL);

	Rcv_Time = (unsigned long)((1000000*(tv.tv_sec%1000)) + tv.tv_usec);

#endif

	if (msg_size < sizeof(Message_Header_t))
	{
		*Msg_Id = NO_MESSAGE_ID;
		USBComm_Remove_Rx_Message();    // add this. Jack. RTC
		return (MSG_PROT_ERROR_SIZE_MESSAGE);
	}

	//Extract data from USBComm received data buffer


	memcpy(&Last_Msg_Hdr,&(msg_p->Message_Header), sizeof(Last_Msg_Hdr));	// save last message header

	memcpy(Data_Buff, (msg_p->Data), MSG_PROT_GEN_MSG_MAX_DATA_SIZE);		// save message data to given Data Buffer

	res = USBComm_Remove_Rx_Message();										// we already copy the message so now we can remove it from Buffer

	*Msg_Id = Last_Msg_Hdr.Id;

	return res;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Get_Last_Msg_Hdr
//
//  Purpose				: send message to USB
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				: Success or Error code
//
//  Description			: Send message to USB
//
// ---------------------------------------------------------------------------
void Msg_Prot_Get_Last_Msg_Hdr (int* piMsg_Id,unsigned long* puMsg_SN, int* piMsg_Size, unsigned long* puMsg_Time )
{

	*piMsg_Id   = Last_Msg_Hdr.Id;

	*puMsg_SN   = Last_Msg_Hdr.Serial_Number;

	*piMsg_Size = Last_Msg_Hdr.Size;

	*puMsg_Time = Last_Msg_Hdr.Time_Stamp;

}


//-----------------------------
// Pack/unpack function
//-----------------------------

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Next_Stitch_data
//
//  Purpose				: pack & transmit Next stitch data to RTC
//
//  Inputs				: Stitch_Data - all the stitch data.
//
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			:  pack & transmit stitching data message to RTC
//
// ---------------------------------------------------------------------------

int Msg_Prot_Pack_n_Tx_Next_Stitch_data(RTC_Stitch_t Stitch_Data, int Flush_Buffer)
{

	Msg_Prot_Stitch_Msg_t  stitch_msg;

	int res;

	stitch_msg.Msg_Hdr.Id = MSG_PROT_NEXT_STITCH_DATA;

	stitch_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	stitch_msg.Stitch_Info.Stitch = Stitch_Data;

	stitch_msg.Stitch_Info.Flush_Buffer = Flush_Buffer;

	stitch_msg.Msg_Hdr.Size = sizeof(stitch_msg.Msg_Hdr)+ sizeof( stitch_msg.Stitch_Info);

#ifdef ONS_HOST
	unsigned long Tx_Time;
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];

#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
    gettimeofday(&tv,NULL);
	Tx_Time = ((1000000*(tv.tv_sec%1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time =((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	stitch_msg.Msg_Hdr.Time_Stamp = Tx_Time;
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_NEXT_STITCH_DATA %d", stitch_msg.Stitch_Info.Stitch.SID);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 1, __FILENAME__, __LINE__, __func__, msg_txt);

	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
#endif // ONS_HOST

	res = USBComm_Send_Message( (USBComm_Buffer_t)&stitch_msg , stitch_msg.Msg_Hdr.Size );
	return res;
}


// ---------------------------------------------------------------------------
//  Function name		:  Msg_Prot_Unpack_Next_Stitch_data
//
//  Purpose				: unpack test message
//
//  Inputs				: Msg_Buff - pointer to the received message (without the generic header) .
//
//  Outputs				: Stitch_Data - the next stitch data
//
//  Returns				: Success
//
//  Description			: Get the next stitch data
//
// ---------------------------------------------------------------------------
int  Msg_Prot_Unpack_Next_Stitch_data(ONS_BYTE* Msg_Buff, RTC_Stitch_t* Stitch_Data, int* Flush_Buffer )
{
	Stitch_Info_t* msg_p;

	msg_p = (Stitch_Info_t*)Msg_Buff;
 	memcpy(Stitch_Data, &msg_p->Stitch, sizeof(RTC_Stitch_t));
 	*Flush_Buffer = msg_p->Flush_Buffer;

 	return MSG_PROT_SUCCESS;
}


// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_RTC_Status
//
//  Purpose				: pack & transmit RTC status to HOST
//
//  Inputs				: Stitch_ID 	 - Current stitch ID
//						  Current_RPM    - RTC actual current RPM
//						  Current_X_Pos	 - RTC actual current position X
//						  Current_Y_Pos  - RTC actual current position Y
//						  Target_RPM	 - RPM Target value for RTC to reach
//						  Target_X_Pos	 - Position X Target value for RTC to reach
//						  Target_Y_Pos	 - Position Y Target value for RTC to reach
//						  RTC_State 	 - RTC_READY / RTC_RUN
//						  Action_Started -  indication for Start Jump/ Start SH lock up/ Thread break
//						  HW_State		 - HW state (Needle up up down, Cat Done or Not )
//						  Error_Code 	 - RTC error codes (TBD)
//						  Reason_code 		 - reason code from RTC
//
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			:
//
// ---------------------------------------------------------------------------
int Msg_Prot_Pack_n_Tx_RTC_Status(int Stitch_ID,int Current_RPM, int Current_X_Pos, int Current_Y_Pos,
								  int Target_RPM, int Target_X_Pos, int Target_Y_Pos,
								  int XY_Moving, int Sew_Head_Rotating, int RTC_State,
								  RTC_Action_Start_Report_t Action_Started, int Buff_Stitches_Num, int HW_State, ONS_RTC_Status_Error_type_t Error_Code, int Reason_Code)
{
	Msg_Prot_RTC_Status_Msg_t  RTC_Status_msg;

	int res;


	RTC_Status_msg.Msg_Hdr.Id 					= MSG_PROT_RTC_STATUS;

	RTC_Status_msg.Msg_Hdr.Serial_Number 		= Msg_Prot_Get_Msg_SN();

	RTC_Status_msg.RTC_Status.Stitch_ID 		= Stitch_ID;

	RTC_Status_msg.RTC_Status.Current_RPM 		= Current_RPM;

	RTC_Status_msg.RTC_Status.Current_X_Pos 	= Current_X_Pos;

	RTC_Status_msg.RTC_Status.Current_Y_Pos 	= Current_Y_Pos;

	RTC_Status_msg.RTC_Status.Target_RPM 		= Target_RPM;

	RTC_Status_msg.RTC_Status.Target_X_Pos 		= Target_X_Pos;

	RTC_Status_msg.RTC_Status.Target_Y_Pos 		= Target_Y_Pos;

	RTC_Status_msg.RTC_Status.XY_Moving 		= XY_Moving;

	RTC_Status_msg.RTC_Status.Sew_Head_Rotating	= Sew_Head_Rotating;

	RTC_Status_msg.RTC_Status.State 			= RTC_State;

	RTC_Status_msg.RTC_Status.Action_Start		= (INT_16)Action_Started;

	RTC_Status_msg.RTC_Status.Buff_Stitches_Num = Buff_Stitches_Num;

	RTC_Status_msg.RTC_Status.HW_State			= HW_State;
		
	RTC_Status_msg.RTC_Status.Error_Code 		= (INT_16)Error_Code;

	RTC_Status_msg.RTC_Status.Reason_Code		= Reason_Code;
	
	RTC_Status_msg.Msg_Hdr.Size 				= sizeof(RTC_Status_msg.Msg_Hdr)+ sizeof( RTC_Status_msg.RTC_Status);

 	

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	RTC_Status_msg.Msg_Hdr.Time_Stamp = Tx_Time;
#endif // ONS_HOST

#ifdef ONS_RTC
    RTC_Status_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif    

	res = USBComm_Send_Message( (USBComm_Buffer_t)&RTC_Status_msg , RTC_Status_msg.Msg_Hdr.Size );

	return res;
}


// ---------------------------------------------------------------------------
//  Function name		:  Msg_Prot_Unpack_RTC_Status
//
//  Purpose				: unpack test message
//
//  Inputs				: Msg_Buff - pointer to the received message (without the generic header) .
//
//  Outputs				:RTC_State - the RTC  state
//						 Stitch_ID
//
//  Returns				: Success
//
//  Description			:get the next stitch data
//
// ---------------------------------------------------------------------------
int Msg_Prot_Unpack_RTC_Status(ONS_BYTE* Msg_Buff,int* Stitch_ID,
									int* Current_RPM, int* Current_X_Pos, int* Current_Y_Pos,
									int* Target_RPM, int* Target_X_Pos, int* Target_Y_Pos,
									int* XY_Moving, int* Sew_Head_Rotating, int* RTC_State,
									RTC_Action_Start_Report_t* Action_Started, int* Buff_Stitches_Num, int* HW_State, ONS_RTC_Status_Error_type_t* Error_Code, int* Reason_Code)
{

	Msg_Prot_RTC_Status_t* msg_p;

	msg_p = (Msg_Prot_RTC_Status_t*)Msg_Buff;

	*Stitch_ID			= msg_p->Stitch_ID;

	*RTC_State 			= msg_p->State;

	*Current_RPM 		= msg_p->Current_RPM ;

	*Current_X_Pos 		= msg_p->Current_X_Pos;

	*Current_Y_Pos 		= msg_p->Current_Y_Pos;

	*Target_RPM 		= msg_p->Target_RPM ;

	*Target_X_Pos 		= msg_p->Target_X_Pos;

	*Target_Y_Pos 		= msg_p->Target_Y_Pos;

	*XY_Moving			= msg_p->XY_Moving;

	*Sew_Head_Rotating	= msg_p->Sew_Head_Rotating;

	*RTC_State 			= msg_p->State;

	*Action_Started		= (RTC_Action_Start_Report_t)msg_p->Action_Start;

	*Buff_Stitches_Num	= msg_p->Buff_Stitches_Num;

	*HW_State			= msg_p->HW_State;

	*Error_Code 		= (ONS_RTC_Status_Error_type_t)msg_p->Error_Code;

	*Reason_Code		= msg_p->Reason_Code;


#ifdef ONS_HOST
	//char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	//snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Receive MSG_PROT_RTC_STATUS");
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 2, __FILENAME__, __LINE__, __func__, (char*)"Receive MSG_PROT_RTC_STATUS");
//	ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_INFO, __FILENAME__, __LINE__, __func__, msg_txt);
//	Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_RTC_STATUS, *Stitch_ID, *Target_X_Pos , *Target_Y_Pos , *XY_Moving, *Sew_Head_Rotating,  *RTC_State);
#endif

	return MSG_PROT_SUCCESS;

}


// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_RTC_RUN
//
//  Purpose				: pack & transmit RUN command to RTC
//
//  Inputs				: Mode 		- STITCHING_MODE / DRYRUN_MODE / STEP_MODE
//						  Delay 	- valid only for DryRun delay between stitching
//						  Direction - valid only for DryRun DRYRUN_BW / DRYRUN_FW
//						  APF_Move	- valid only for DryRun (0 - APF in fix position, 1 - change APF according to stitch data)
//						  
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			: send to RTC the RUN notification with the mode relevant info so RTC can start RUN operation
//
// ---------------------------------------------------------------------------
int Msg_Prot_Pack_n_Tx_RTC_RUN(int Mode, unsigned int Delay, int Direction, int APF_Move)
{
	Msg_Prot_RTC_Run_Msg_t  RTC_RUN_msg;

	int res;

	RTC_RUN_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	RTC_RUN_msg.Msg_Hdr.Id 		= MSG_PROT_RTC_RUN;

	RTC_RUN_msg.Msg_Hdr.Size 	= sizeof(RTC_RUN_msg.Msg_Hdr)+ sizeof( RTC_RUN_msg.Info);

	RTC_RUN_msg.Info.Mode 		= Mode;

	RTC_RUN_msg.Info.Delay 		= Delay;

	RTC_RUN_msg.Info.Direction 	= Direction;

	RTC_RUN_msg.Info.APF_Move	= APF_Move;

#ifdef ONS_HOST
		unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
		struct timeval  	tv;
		gettimeofday(&tv, NULL);
		Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
		SYSTEMTIME time;
		GetSystemTime(&time);
		Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
		RTC_RUN_msg.Msg_Hdr.Time_Stamp = Tx_Time;

		//char	msg_txt[MAX_SYSLOG_DESC_SIZE];
		//snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_RTC_RUN");
		//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 3, __FILENAME__, __LINE__, __func__, (char*)"Send MSG_PROT_RTC_RUN");
//		snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_RTC_RUN : Mode = %d , Delay = %d, Direction = %d APD_Move=%d\n", Mode, Delay, Direction, APF_Move);
//		ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
//		Print_To_Screen(msg_txt);
#endif // ONS_HOST

		res = USBComm_Send_Message( (USBComm_Buffer_t)&RTC_RUN_msg , RTC_RUN_msg.Msg_Hdr.Size );

		return res;
}



// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_RTC_RUN
//
//  Purpose				: unpack test message
//
//  Inputs				: Msg_Buff - pointer to the received message (without the generic header) .
//
//  Outputs				: Mode 		- STITCHING_MODE / DRYRUN_MODE / STEP
//						  Delay 	- valid only for DryRun delay between stitching
//						  Direction - valid only for DryRun DRYRUN_BW / DRYRUN_FW
//						  APF_Move	- valid only for DryRun (0 - APF in fix position, 1 - change APF according to stitch data)
//
//  Returns				: Success
//
//  Description			: get the RUN command info
//
// ---------------------------------------------------------------------------
int  Msg_Prot_Unpack_RTC_RUN(ONS_BYTE* Msg_Buff,int *Mode,unsigned int *Delay, int* Direction, int* APF_Move)
{

	Msg_Prot_RTC_Run_t* msg_p;

	msg_p = (Msg_Prot_RTC_Run_t*)Msg_Buff;

	*Mode = msg_p->Mode;

	*Delay = msg_p->Delay;

	*Direction = msg_p->Direction;

	*APF_Move = msg_p->APF_Move;

	return MSG_PROT_SUCCESS;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_RTC_DR_Delay_Update
//
//  Purpose				: pack & transmit DR_Delay_Update command to RTC
//
//  Inputs				: Delay 	- valid only for DryRun delay between stitching
//
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			: send to RTC the DR_Delay_Update notification with the mode relevant info so RTC can Update DR Delay  
//
// ---------------------------------------------------------------------------
int Msg_Prot_Pack_n_Tx_RTC_DR_Delay_Update(unsigned int Delay)
{
	Msg_Prot_RTC_Dry_Run_Delay_Update_Msg_t  RTC_DR_delay_update_msg;

	int res;

	RTC_DR_delay_update_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	RTC_DR_delay_update_msg.Msg_Hdr.Id = MSG_PROT_UPDATE_DRY_RUN_DELAY_REQ;

	RTC_DR_delay_update_msg.Msg_Hdr.Size = sizeof(RTC_DR_delay_update_msg.Msg_Hdr) + sizeof(RTC_DR_delay_update_msg.Dry_Run_Delay);

	RTC_DR_delay_update_msg.Dry_Run_Delay = Delay;

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	RTC_DR_delay_update_msg.Msg_Hdr.Time_Stamp = Tx_Time;

	char	msg_txt[MAX_SYSLOG_DESC_SIZE];

	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_UPDATE_DRY_RUN_DELAY_REQ : Delay = %d",Delay);
//	ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 4, __FILENAME__, __LINE__, __func__, msg_txt);

	//Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_RTC_RUN, Mode, Delay, Direction, 0, 0, 0);
#endif // ONS_HOST

	res = USBComm_Send_Message((USBComm_Buffer_t)&RTC_DR_delay_update_msg, RTC_DR_delay_update_msg.Msg_Hdr.Size);

	return res;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_RTC_DR_Delay_Update
//
//  Purpose				: unpack test message
//
//  Inputs				: Msg_Buff - pointer to the received message (without the generic header) .
//
//  Outputs				: Delay 	- valid only for DryRun delay between stitching
//
//  Returns				: Success
//
//  Description			: get the Dry run speed command info
//
// ---------------------------------------------------------------------------
int Msg_Prot_Unpack_RTC_DR_Delay_Update(ONS_BYTE* Msg_Buff, unsigned int *Delay)
{

	UINT_16* msg_p;

	msg_p = (UINT_16*)Msg_Buff;

	*Delay = *msg_p;

	return MSG_PROT_SUCCESS;
}



// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_RTC_Error
//
//  Purpose				: pack & transmit RTC Error notification to HOST
//
//  Inputs				: Error_Code - RTC error codes (TBD)
//						  Error_data - Error string info
//
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			: receive error code and string of data pack and send them
//
// ---------------------------------------------------------------------------
int Msg_Prot_Pack_n_Tx_RTC_Error(int  Error_Code, char* Error_Data)
{
	Msg_Prot_RTC_Err_Msg_t  RTC_Error_msg;

	int res;


	RTC_Error_msg.Msg_Hdr.Id = MSG_PROT_RTC_ERROR_MSG;

	RTC_Error_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	RTC_Error_msg.Info.Error_Code = Error_Code;

	memcpy(RTC_Error_msg.Info.Error_Data, Error_Data, sizeof(RTC_Error_msg.Info.Error_Data));

	RTC_Error_msg.Msg_Hdr.Size = sizeof(RTC_Error_msg.Msg_Hdr)+ sizeof( RTC_Error_msg.Info);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	RTC_Error_msg.Msg_Hdr.Time_Stamp = Tx_Time;
#endif // ONS_HOST


#ifdef ONS_RTC
	RTC_Error_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message( (USBComm_Buffer_t)&RTC_Error_msg , RTC_Error_msg.Msg_Hdr.Size );

	return res;
}

// ---------------------------------------------------------------------------
//  Function name		:  Msg_Prot_Unpack_RTC_Error
//
//  Purpose				: unpack RTC error notification message
//
//  Inputs				: Msg_Buff - pointer to the received message (without the generic header) .
//
//  Outputs				:Error_Code - RTC error codes (TBD)
//						 Error_data - Error string info
//
//  Returns				: Success
//
//  Description			: unpack the error code and error data from the RTC message
//
// ---------------------------------------------------------------------------
int  Msg_Prot_Unpack_RTC_Error(ONS_BYTE* Msg_Buff, int* Error_Code, char* Error_Data)

{

	Msg_Prot_RTC_Err_t* msg_p;

	msg_p = (Msg_Prot_RTC_Err_t*)Msg_Buff;

	*Error_Code = (int)(msg_p->Error_Code);

	memcpy(Error_Data,msg_p->Error_Data, sizeof(msg_p->Error_Data));

#ifdef ONS_HOST
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Receive MSG_PROT_RTC_ERROR_MSG code = %d",*Error_Code);
//	ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 5, __FILENAME__, __LINE__, __func__, msg_txt);
//	Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_RTC_ERROR_MSG, *Error_Code, 0 , 0 , 0, 0, 0);
#endif

	return MSG_PROT_SUCCESS;

}
// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_RTC_Move_X_Y
//
//  Purpose				: pack & transmit Move command to RTC
//
//  Inputs				: X_Pos, Y_pos - the destination position for the movement
//						  Scale_x, Scale_Y - the movement scale for each axe.
//
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			: send to RTC the destination location for the movement and the scale for the axes
//
// ---------------------------------------------------------------------------
int Msg_Prot_Pack_n_Tx_RTC_Move_X_Y(int  X_Pos, int Y_pos, int Scale_X, int Scale_Y)
{
	Msg_Prot_RTC_Move_X_Y_Msg_t  RTC_Move_msg;

	int res;

		RTC_Move_msg.Msg_Hdr.Serial_Number 	= Msg_Prot_Get_Msg_SN();

		RTC_Move_msg.Msg_Hdr.Id 			= MSG_PROT_RTC_MOVE_X_Y;

		RTC_Move_msg.Msg_Hdr.Size 			= sizeof(RTC_Move_msg.Msg_Hdr)+ sizeof( RTC_Move_msg.Info);

		RTC_Move_msg.Info.X_pos				= X_Pos;

		RTC_Move_msg.Info.Y_pos				= Y_pos;

		RTC_Move_msg.Info.Scale_X			= Scale_X;

		RTC_Move_msg.Info.Scale_Y			= Scale_Y;

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	RTC_Move_msg.Msg_Hdr.Time_Stamp 	= Tx_Time;
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_RTC_MOVE_X_Y : X_Pos =%d, Y_pos = %d, Scale_X = %d, Scale_Y = %d", X_Pos, Y_pos, Scale_X, Scale_Y);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 6, __FILENAME__, __LINE__, __func__, msg_txt);

//	ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
//	Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_RTC_MOVE_X_Y, X_Pos, Y_pos, Scale_X, Scale_Y, 0, 0); 
#endif // ONS_HOST

		res = USBComm_Send_Message( (USBComm_Buffer_t)&RTC_Move_msg , RTC_Move_msg.Msg_Hdr.Size );

		return res;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_RTC_Move_X_Y
//
//  Purpose				: unpack Move message
//
//  Inputs				: Msg_Buff - pointer to the received message (without the generic header) .
//
//  Outputs				: X_Pos, Y_pos - the destination position for the movement
//						  Scale_x, Scale_Y - the movement scale for each axe.
//
//  Returns				: Success
//
//  Description			: RTC gets the destination for the move commend and the scale for each axe
//
// ---------------------------------------------------------------------------
int  Msg_Prot_Unpack_RTC_Move_X_Y(ONS_BYTE* Msg_Buff, int*  X_Pos, int* Y_Pos, int* Scale_X, int* Scale_Y)
{

	Msg_Prot_RTC_Move_X_Y_t* msg_p;

	msg_p = (Msg_Prot_RTC_Move_X_Y_t*)Msg_Buff;

	*X_Pos = msg_p->X_pos;

	*Y_Pos = msg_p->Y_pos;

	*Scale_X = msg_p->Scale_X;

	*Scale_Y = msg_p->Scale_Y;

	return MSG_PROT_SUCCESS;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Dump_REQ
//
//  Purpose				: pack & transmit  dump request from RTC
//
//  Inputs				: 
//
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			: send to RTC dump data  request in order to initiate sending of logs in case for ERROR and befor ABORT
//
// ---------------------------------------------------------------------------
int Msg_Prot_Pack_n_Tx_RTC_Dump_Req( void )
{
	Msg_Prot_Dump_Req_Msg_t  Dump_Req_msg;

	int res;


	Dump_Req_msg.Msg_Hdr.Id 				= MSG_PROT_DUMP_REQ;

	Dump_Req_msg.Msg_Hdr.Serial_Number 		= Msg_Prot_Get_Msg_SN();

	Dump_Req_msg.Msg_Hdr.Size 				= sizeof(Dump_Req_msg.Msg_Hdr);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	Dump_Req_msg.Msg_Hdr.Time_Stamp = Tx_Time;
	//char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	//snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_DUMP_REQ");
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 7, __FILENAME__, __LINE__, __func__, (char*)"Send MSG_PROT_DUMP_REQ");
//	ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt); 
//	Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_DUMP_REQ, 0, 0, 0, 0, 0, 0);
#endif // ONS_HOST

#ifdef ONS_RTC
    Dump_Req_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message( (USBComm_Buffer_t)&Dump_Req_msg , Dump_Req_msg.Msg_Hdr.Size );

	return res;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_RTC_Dump
//
//  Purpose				: pack & transmit data need to dump into file
//
//  Inputs				: Dump_Type - the Type of dump message
//						  Info Size - number of messages to dump
//						  Dump_Data - data to dump message data
//
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			: send to host dump data in order to write to log file
//
// ---------------------------------------------------------------------------
int Msg_Prot_Pack_n_Tx_RTC_Dump(int  Dump_Type, int Info_Size, Msg_Prot_RTC_Dump_Data_t* Dump_Data)
{
	Msg_Prot_RTC_Dump_Msg_t  RTC_Dump_msg;

	int res;

	RTC_Dump_msg.Msg_Hdr.Serial_Number 	= Msg_Prot_Get_Msg_SN();

	RTC_Dump_msg.Msg_Hdr.Id 			= MSG_PROT_RTC_DUMP_TO_LOG;

	RTC_Dump_msg.Msg_Hdr.Size 			= sizeof(RTC_Dump_msg.Msg_Hdr)+ sizeof(Dump_Type) + sizeof(Msg_Prot_RTC_Dump_t);

	RTC_Dump_msg.Info.Info_Size				= Info_Size;

	RTC_Dump_msg.Info.Dump_Type				= Dump_Type;

	memcpy (RTC_Dump_msg.Info.Data, Dump_Data, (sizeof(Msg_Prot_RTC_Dump_Data_t)*Info_Size));

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
		RTC_Dump_msg.Msg_Hdr.Time_Stamp 	= Tx_Time;
#endif // ONS_HOST


		res = USBComm_Send_Message( (USBComm_Buffer_t)&RTC_Dump_msg , RTC_Dump_msg.Msg_Hdr.Size );

		return res;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_RTC_Dump
//
//  Purpose				: unpack Dump message
//
//  Inputs				: Msg_Buff - pointer to the received message (without the generic header) .
//						  Buff_Size - size of buffer (num of int in the given array)
//
//  Outputs				: Dump_Type - the Type of dump message
//						  Dump_Data - data to dump message data
//						  Info_Size - number of messages data to dump (dump message data size 3*INT_16)
//
//  Returns				: Success
//
//  Description			: Host gets the Dump data need to writ into log file
//
// ---------------------------------------------------------------------------
int Msg_Prot_Unpack_RTC_Dump(ONS_BYTE* Msg_Buff, int  *Dump_Type, int* Info_Size, Msg_Prot_RTC_Dump_Data_t* Dump_Data)
{
	Msg_Prot_RTC_Dump_t* msg_p;


	msg_p = (Msg_Prot_RTC_Dump_t*)Msg_Buff;

	*Dump_Type = msg_p->Dump_Type;

	*Info_Size = msg_p->Info_Size;

	memcpy (Dump_Data, msg_p->Data, sizeof(Msg_Prot_RTC_Dump_Data_t)*msg_p->Info_Size);
	
#ifdef ONS_HOST
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Receive MSG_PROT_RTC_DUMP_TO_LOG (size = %d) ", *Info_Size);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 8, __FILENAME__, __LINE__, __func__, msg_txt);

	//Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_RTC_DUMP_TO_LOG, msg_p->Dump_Type, msg_p->Info_Size , msg_p->Data[0].Data, msg_p->Data[0].Time, msg_p->Data[0].Type,*Info_Size);
#endif

	return MSG_PROT_SUCCESS;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_RTC_Home_Mode
//
//  Purpose				: pack & transmit Host HOME mode
//
//  Inputs				:Home_Mode - the host current home mode
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			:
//
// ---------------------------------------------------------------------------
int Msg_Prot_Pack_n_Tx_Home_Mode(Msg_Prot_Home_Mode_t Home_Mode, Msg_Prot_Home_Z_Direction_t Home_Z_Direction)
{

	Msg_Prot_Home_Mode_Msg_t  Home_Mode_Data_Msg;
	int res;

	Home_Mode_Data_Msg.Msg_Hdr.Serial_Number 	= Msg_Prot_Get_Msg_SN();

	Home_Mode_Data_Msg.Msg_Hdr.Id 			= MSG_PROT_HOME_MODE;

	Home_Mode_Data_Msg.Msg_Hdr.Size 			= sizeof(Msg_Prot_Home_Mode_Msg_t);

	Home_Mode_Data_Msg.Home_Mode_Data.Home_Mode				= Home_Mode;
	Home_Mode_Data_Msg.Home_Mode_Data.Z_Home_Direction		= Home_Z_Direction;
#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	Home_Mode_Data_Msg.Msg_Hdr.Time_Stamp 	= Tx_Time;
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_HOME_MODE  MODE = %d Z_Direction = %d", Home_Mode_Data_Msg.Home_Mode_Data.Home_Mode, Home_Mode_Data_Msg.Home_Mode_Data.Z_Home_Direction);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 9, __FILENAME__, __LINE__, __func__, msg_txt);
	//	Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_HOME_MODE, Home_Mode, 0 , 0 , 0 , 0 ,  0 );
#endif // ONS_HOST

		res = USBComm_Send_Message( (USBComm_Buffer_t)&Home_Mode_Data_Msg, Home_Mode_Data_Msg.Msg_Hdr.Size );

		return res;
}
// ---------------------------------------------------------------------------
int Msg_Prot_Pack_HomeMode_3020(Msg_Prot_Home_Mode_t Home_Mode, Msg_Prot_Home_Z_Direction_t Home_Z_Direction,int VAL_1,int VAL_2)
{

	Msg_Prot_Home_Mode_Msg3020_t  Home_Mode_Data_Msg3020;
	int res;
	Home_Mode_Data_Msg3020.Msg_Hdr.Serial_Number 			= Msg_Prot_Get_Msg_SN();
	Home_Mode_Data_Msg3020.Msg_Hdr.Id 						= MSG_PROT_HOME_MODE;
	Home_Mode_Data_Msg3020.Msg_Hdr.Size 					= sizeof(Msg_Prot_Home_Mode_Msg3020_t);
	Home_Mode_Data_Msg3020.Home_Mode_Data3020.Home_Mode			= Home_Mode;
	Home_Mode_Data_Msg3020.Home_Mode_Data3020.Z_Home_Direction	= Home_Z_Direction;
	Home_Mode_Data_Msg3020.Home_Mode_Data3020.HomeVal_1			= VAL_1;
	Home_Mode_Data_Msg3020.Home_Mode_Data3020.HomeVal_2			= VAL_2;
#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN______
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	Home_Mode_Data_Msg3020.Msg_Hdr.Time_Stamp 	= Tx_Time;
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	//snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_HOME_MODE  MODE = %d Z_Direction = %d", Home_Mode_Data_Msg3020.Home_Mode_Data3020.Home_Mode, Home_Mode_Data_Msg.Home_Mode_Data.Z_Home_Direction);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 9, __FILENAME__, __LINE__, __func__, msg_txt);
	//	Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_HOME_MODE, Home_Mode, 0 , 0 , 0 , 0 ,  0 );
#endif // ONS_HOST

		res = USBComm_Send_Message( (USBComm_Buffer_t)&Home_Mode_Data_Msg3020, Home_Mode_Data_Msg3020.Msg_Hdr.Size );

		return res;
}
// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_RTC_Home_status
//
//  Purpose				: unpack
//
//  Inputs				: Msg_Buff - pointer to the received message (without the generic header) .
//
//  Outputs				: Home_Mode - HOST current Home mode
//
//  Returns				: Success
//
//  Description			:
//
// ---------------------------------------------------------------------------
int Msg_Prot_Unpack_Home_Mode(ONS_BYTE* Msg_Buff, Msg_Prot_Home_Mode_t* Home_Mode, Msg_Prot_Home_Z_Direction_t* Home_Z_Direction)
{
	Msg_Prot_Home_Mode_Data_t* msg_p;

	msg_p = (Msg_Prot_Home_Mode_Data_t*)Msg_Buff;

	*Home_Mode = (Msg_Prot_Home_Mode_t)msg_p->Home_Mode;
	*Home_Z_Direction = (Msg_Prot_Home_Z_Direction_t)msg_p->Z_Home_Direction;

	return MSG_PROT_SUCCESS;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_RTC_Home_Status
//
//  Purpose				: pack & transmit
//
//  Inputs				:
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			:
//
// ---------------------------------------------------------------------------
int Msg_Prot_Pack_n_Tx_RTC_Home_Status(RTC_State_t	RTC_Home_Mode, int X_Pos, int Y_Pos, int XY_Moving, int Sew_Head_Rotating, RTC_Home_Result_t Home_Res, int Error_Code)
{
	Msg_Prot_RTC_Home_Status_Msg_t  RTC_home_status_msg;

	int res;

		RTC_home_status_msg.Msg_Hdr.Serial_Number 	= Msg_Prot_Get_Msg_SN();

		RTC_home_status_msg.Msg_Hdr.Id 				= MSG_PORT_RTC_HOME_STATUS;

		RTC_home_status_msg.Msg_Hdr.Size 			= sizeof(RTC_home_status_msg.Msg_Hdr)+ sizeof( RTC_home_status_msg.RTC_Status);

		RTC_home_status_msg.RTC_Status.X_pos				= X_Pos;

		RTC_home_status_msg.RTC_Status.Y_pos				= Y_Pos;

		RTC_home_status_msg.RTC_Status.RTC_Home_Mode		= RTC_Home_Mode;

		RTC_home_status_msg.RTC_Status.Sew_Head_Rotating	= Sew_Head_Rotating;

		RTC_home_status_msg.RTC_Status.Home_Res				= Home_Res;

		RTC_home_status_msg.RTC_Status.XY_Moving			= XY_Moving;

		RTC_home_status_msg.RTC_Status.Error_Code			= Error_Code;
#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
		RTC_home_status_msg.Msg_Hdr.Time_Stamp 	= Tx_Time;
#endif // ONS_HOST

		res = USBComm_Send_Message( (USBComm_Buffer_t)&RTC_home_status_msg , RTC_home_status_msg.Msg_Hdr.Size );
	//	printf("rtc msg host :  MSG_PORT_RTC_HOME_STATUS : RTC_Home_Mode=%d, Test_Res = %d RES = %d \n",RTC_home_status_msg.RTC_Status.RTC_Home_Mode,RTC_home_status_msg.RTC_Status.Test_Res,res);
		return res;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_RTC_Home_status
//
//  Purpose				: unpack
//
//  Inputs				: Msg_Buff - pointer to the received message (without the generic header) .
//
//  Outputs				:
//
//  Returns				: Success
//
//  Description			:
//
// ---------------------------------------------------------------------------
int  Msg_Prot_Unpack_RTC_Home_Status(ONS_BYTE* Msg_Buff, RTC_State_t* RTC_Home_Mode, int* X_Pos, int* Y_Pos, int* XY_Moving, int*	Sew_Head_Rotating, RTC_Home_Result_t* Home_Res, int* Error_Code)
{
	Msg_Prot_RTC_Home_Status_t* msg_p;

	msg_p = (Msg_Prot_RTC_Home_Status_t*)Msg_Buff;

	*RTC_Home_Mode 		= (RTC_State_t)msg_p->RTC_Home_Mode;
	*X_Pos				= msg_p->X_pos;
	*Y_Pos 				= msg_p->Y_pos;
	*XY_Moving 			= msg_p->XY_Moving;
	*Sew_Head_Rotating 	= msg_p->Sew_Head_Rotating;
	*Home_Res 			= (RTC_Home_Result_t)msg_p->Home_Res;
	*Error_Code			= msg_p->Error_Code;

//	printf(" RTC_Home_Mode = %d, Home_Res = %d, Error_Code = %d ",msg_p->RTC_Home_Mode,msg_p->Home_Res,msg_p->Error_Code);
#ifdef ONS_HOST
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Receive MSG_PORT_RTC_HOME_STATUS : RTC_Home_Mode =%d,  X_Pos=%d,  Y_Pos=%d,  XY_Moving=%d, Sew_Head_Rotating=%d,  Home_Res=%d,  Error_Code=%d", *RTC_Home_Mode, *X_Pos, *Y_Pos, *XY_Moving, *Sew_Head_Rotating, *Home_Res, *Error_Code);
//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 10, __FILENAME__, __LINE__, __func__, msg_txt);
	//Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PORT_RTC_HOME_STATUS, *RTC_Home_Mode, *X_Pos, *Y_Pos , *XY_Moving , *Sew_Head_Rotating, *Home_Res );
#endif
	return MSG_PROT_SUCCESS;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_IO_Act_Cmd
//
//  Purpose				: unpack
//
//  Inputs				: Num_Of_CMD
//						: IO_Act_Data
//
//  Outputs				:
//
//  Returns				: Success
//
//  Description			:
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Pack_n_Tx_IO_Act_Cmd(int Num_Of_IO_CMD, Msg_Prot_IO_Act_t* IO_Act_Data)
{
	Msg_Prot_IO_Act_Cmd_t  IO_Act_CMD;
//	int index;
	int res = 0;

//	printf("Num_Of_IO_CMD = %d id = %d state = %d\n",Num_Of_IO_CMD, IO_Act_Data[0].IO_Act_ID, IO_Act_Data[0].IO_Act_State);

	if (Num_Of_IO_CMD >= MSG_PROT_MAX_IO_ACT_NUM)
	{
		res = Num_Of_IO_CMD;

		Num_Of_IO_CMD = MSG_PROT_MAX_IO_ACT_NUM-1;
	}
	IO_Act_CMD.Msg_Hdr.Serial_Number 	= Msg_Prot_Get_Msg_SN();

	IO_Act_CMD.Msg_Hdr.Id 				= MSG_PROT_ACT_IO_CMD;

	IO_Act_CMD.Msg_Hdr.Size 			= sizeof(IO_Act_CMD.Msg_Hdr)+ sizeof( IO_Act_CMD.IO_Act_Info);

	IO_Act_CMD.IO_Act_Info.Num_OF_IO_Act_ID = Num_Of_IO_CMD;

//	for (index = 0; index < Num_Of_CMD; index++)
//	{
//		IO_Act_CMD.IO_Act_Info.IO_Act_data[index] = IO_Act_Data[index];
//	}

	memcpy(IO_Act_CMD.IO_Act_Info.IO_Act_data,IO_Act_Data, sizeof(INT_16)*2*Num_Of_IO_CMD);
//	printf("final - Num_Of_IO_CMD = %d id = %d state = %d \n",IO_Act_CMD.IO_Act_Info.Num_OF_IO_Act_ID,IO_Act_CMD.IO_Act_Info.IO_Act_data[0].IO_Act_ID, IO_Act_CMD.IO_Act_Info.IO_Act_data[0].IO_Act_State);

#ifdef ONS_HOST
	char				msg_txt[MAX_SYSLOG_DESC_SIZE];

	if (res != 0 )
	{
		snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "MSG ABORTED Num_Of_CMD >= MSG_PROT_MAX_IO_ACT_NUM  res =%d\n",res );
		//ONS_Log.Update_Log_Data(LOG_ERROR_LEVEL, LOG_CATEGORY_RTC_COMM, 11, __FILENAME__, __LINE__, __func__, msg_txt);
		//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_WARNING, __FILENAME__, __LINE__, __func__, msg_txt);
		//char	msg_txt[MAX_SYSLOG_DESC_SIZE];
		//Log_Write_Error(__FILE__, __func__, __LINE__, res ,"Num_Of_CMD >= MSG_PROT_MAX_IO_ACT_NUM) ");
		return 0;
	}

	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	IO_Act_CMD.Msg_Hdr.Time_Stamp 	= Tx_Time;
	//snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_ACT_IO_CMD");
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 15, __FILENAME__, __LINE__, __func__, (char*)"Send MSG_PROT_ACT_IO_CMD");
//	ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
//	Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_ACT_IO_CMD, Num_Of_IO_CMD, IO_Act_Data[0].IO_Act_ID, IO_Act_Data[0].IO_Act_State, 0 , 0, 0 );
#endif // ONS_HOST

	res = USBComm_Send_Message( (USBComm_Buffer_t)&IO_Act_CMD , IO_Act_CMD.Msg_Hdr.Size );

	return res;

}
// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_IO_Act_Cmd
//
//  Purpose				: unpack
//
//  Inputs				:
//
//  Outputs				: Num_Of_CMD
//						: IO_Act_Data
//
//  Returns				: Success
//
//  Description			:
//
// ---------------------------------------------------------------------------
int  Msg_Prot_Unpack_IO_Act_Cmd(ONS_BYTE* Msg_Buff,int* Num_Of_CMD, Msg_Prot_IO_Act_t* IO_Act_Data)
{

	Msg_Prot_IO_Act_Data_t* msg_p;
//	int index;

	msg_p = (Msg_Prot_IO_Act_Data_t*)Msg_Buff;

	*Num_Of_CMD = msg_p->Num_OF_IO_Act_ID;

//	if (Data_Buff_size < *Num_Of_CMD)
//		*Num_Of_CMD = Data_Buff_size;
//	for (index = 0; index < *Num_Of_CMD; index++)
//	{
//		 IO_Act_Data[index] = msg_p->IO_Act_data[index];
//	}

	memcpy(IO_Act_Data, msg_p->IO_Act_data, (sizeof(INT_16)*2*msg_p->Num_OF_IO_Act_ID));

	return MSG_PROT_SUCCESS;

}
// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Req_IO_Act_State
//
//  Purpose				: unpack
//
//  Inputs				: Num_Of_Req
//						: IO_Act_Data
//
//  Outputs				:
//
//  Returns				: Success
//
//  Description			:
//
// ---------------------------------------------------------------------------
int Msg_Prot_Pack_n_Tx_Req_IO_Act_State(int Num_Of_Req, Msg_Prot_IO_Act_t* IO_Act_Data)
{
	Msg_Prot_IO_Act_Cmd_t  IO_Act_CMD;
//	int index;
	int res = 0;


	if (Num_Of_Req >= MSG_PROT_MAX_IO_ACT_NUM)
	{
		res = Num_Of_Req;

		Num_Of_Req = MSG_PROT_MAX_IO_ACT_NUM-1;
	}

	IO_Act_CMD.Msg_Hdr.Serial_Number 	= Msg_Prot_Get_Msg_SN();

	IO_Act_CMD.Msg_Hdr.Id 				= MSG_PROT_REQ_ACT_IO_STATE;

	IO_Act_CMD.Msg_Hdr.Size 			= sizeof(IO_Act_CMD.Msg_Hdr)+ sizeof( IO_Act_CMD.IO_Act_Info);

	IO_Act_CMD.IO_Act_Info.Num_OF_IO_Act_ID = Num_Of_Req;

//	for (index = 0; index < Num_Of_Req; index++)
//	{
//		IO_Act_CMD.IO_Act_Info.IO_Act_data[index] = IO_Act_Data[index];
//	}

	memcpy(IO_Act_CMD.IO_Act_Info.IO_Act_data,IO_Act_Data, sizeof(INT_16)*2*Num_Of_Req);

#ifdef ONS_HOST
	char				msg_txt[MAX_SYSLOG_DESC_SIZE];

	if (res != 0 )
	{
		snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "MSG ABORTED Num_Of_Req >= MSG_PROT_MAX_IO_ACT_NUM res =%d\n", res);
//		ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_ERR, __FILENAME__, __LINE__, __func__, msg_txt);
		//ONS_Log.Update_Log_Data(LOG_ERROR_LEVEL, LOG_CATEGORY_RTC_COMM, 16, __FILENAME__, __LINE__, __func__, msg_txt);
		return 0;
//		char	msg_txt[MAX_SYSLOG_DESC_SIZE];
//		snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_NEXT_STITCH_DATA");
//		ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
//		Log_Write_Error(__FILE__, __func__, __LINE__, res ,"Num_Of_Req >= MSG_PROT_MAX_IO_ACT_NUM) ");
	}

	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	IO_Act_CMD.Msg_Hdr.Time_Stamp 	= Tx_Time;
//	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
//	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_REQ_ACT_IO_STATE");
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 17, __FILENAME__, __LINE__, __func__, (char*)"Send MSG_PROT_REQ_ACT_IO_STATE");
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
	//Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_REQ_ACT_IO_STATE, Num_Of_Req, IO_Act_Data[0].IO_Act_ID, 0, 0 , 0, 0 );
#endif // ONS_HOST

	res = USBComm_Send_Message( (USBComm_Buffer_t)&IO_Act_CMD , IO_Act_CMD.Msg_Hdr.Size );

	return res;

}
// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Req_IO_Act_State
//
//  Purpose				: unpack
//
//  Inputs				:
//
//  Outputs				: Num_Of_Req
//						: IO_Act_Data
//
//  Returns				: Success
//
//  Description			:
//
// ---------------------------------------------------------------------------
int Msg_Prot_Unpack_Req_IO_Act_State(ONS_BYTE* Msg_Buff,int* Num_Of_Req, Msg_Prot_IO_Act_t* IO_Act_Data)
{
	Msg_Prot_IO_Act_Data_t* msg_p;
//	int index;

	msg_p = (Msg_Prot_IO_Act_Data_t*)Msg_Buff;

	*Num_Of_Req = msg_p->Num_OF_IO_Act_ID;

//	if (Data_Buff_size < *Num_Of_Req)
//		*Num_Of_Req = Data_Buff_size;

//	for (index = 0; index < *Num_Of_Req; index++)
//	{
//		 IO_Act_Data[index] = msg_p->IO_Act_data[index];
//	}

	memcpy(IO_Act_Data, msg_p->IO_Act_data,  (sizeof(INT_16)*2*msg_p->Num_OF_IO_Act_ID));

	return MSG_PROT_SUCCESS;
}
// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_IO_Act_State
//
//  Purpose				: unpack
//
//  Inputs				: Num_Of_Status
//						: IO_Act_Data
//
//  Outputs				:
//
//  Returns				: Success
//
//  Description			:
//
// ---------------------------------------------------------------------------
int Msg_Prot_Pack_n_Tx_IO_Act_State(int Num_Of_Status, Msg_Prot_IO_Act_t* IO_Act_Data)
{
	Msg_Prot_IO_Act_Cmd_t  IO_Act_CMD;
//	int index;
	int res = 0;


	if (Num_Of_Status >= MSG_PROT_MAX_IO_ACT_NUM)
	{
		res = Num_Of_Status;

		Num_Of_Status = MSG_PROT_MAX_IO_ACT_NUM-1;
	}

	IO_Act_CMD.Msg_Hdr.Serial_Number 	= Msg_Prot_Get_Msg_SN();

	IO_Act_CMD.Msg_Hdr.Id 				= MSG_PROT_RTC_ACT_IO_STATE;

	IO_Act_CMD.Msg_Hdr.Size 			= sizeof(IO_Act_CMD.Msg_Hdr)+ sizeof( IO_Act_CMD.IO_Act_Info);

	IO_Act_CMD.IO_Act_Info.Num_OF_IO_Act_ID = Num_Of_Status;

//	for (index = 0; index < Num_Of_Status; index++)
//	{
//		IO_Act_CMD.IO_Act_Info.IO_Act_data[index] = IO_Act_Data[index];
//	}

	memcpy(IO_Act_CMD.IO_Act_Info.IO_Act_data,IO_Act_Data, sizeof(INT_16)*2*Num_Of_Status);

#ifdef ONS_HOST
	char				msg_txt[MAX_SYSLOG_DESC_SIZE];

	if (res != 0 )
	{
		snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "MSG ABORTED Num_Of_State >= MSG_PROT_MAX_IO_ACT_NUM res =%d\n", res);
		//ONS_Log.Update_Log_Data(LOG_ERROR_LEVEL, LOG_CATEGORY_RTC_COMM, 18, __FILENAME__, __LINE__, __func__, msg_txt);
//		ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_ERR, __FILENAME__, __LINE__, __func__, msg_txt);
		//Log_Write_Error(__FILE__, __func__, __LINE__, res ,"Num_Of_State >= MSG_PROT_MAX_IO_ACT_NUM) ");
	}

	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	IO_Act_CMD.Msg_Hdr.Time_Stamp 	= Tx_Time;	

#endif // ONS_HOST


	res = USBComm_Send_Message( (USBComm_Buffer_t)&IO_Act_CMD , IO_Act_CMD.Msg_Hdr.Size );

	return res;
}
// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_IO_Act_State
//
//  Purpose				: unpack
//
//  Inputs				:
//
//  Outputs				: Num_Of_CMD
//						: IO_Act_Data
//
//  Returns				: Success
//
//  Description			:
//
// ---------------------------------------------------------------------------
int  Msg_Prot_Unpack_IO_Act_State(ONS_BYTE* Msg_Buff,int* num_of_status, Msg_Prot_IO_Act_t* IO_Act_Data)
{
		Msg_Prot_IO_Act_Data_t* msg_p;
//		int index;

		msg_p = (Msg_Prot_IO_Act_Data_t*)Msg_Buff;

		*num_of_status = msg_p->Num_OF_IO_Act_ID;

//		if (Data_Buff_size < *num_of_status)
//			*num_of_status = Data_Buff_size;

//		for (index = 0; index < *num_of_status; index++)
//		{
//			IO_Act_Data[index] = msg_p->IO_Act_data[index];
//		}
		memcpy(IO_Act_Data, msg_p->IO_Act_data, (sizeof(INT_16)*2*msg_p->Num_OF_IO_Act_ID));
#ifdef ONS_HOST_LINUX
		char	msg_txt[MAX_SYSLOG_DESC_SIZE];
		snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Receive MSG_PROT_RTC_ACT_IO_STATE (num_of_status = %d)", *num_of_status);
		//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 19, __FILENAME__, __LINE__, __func__, msg_txt);
	//	ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
	//	Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_RTC_ACT_IO_STATE, *num_of_status, IO_Act_Data[0].IO_Act_ID, 0, 0 , 0, 0 );
#endif

		return MSG_PROT_SUCCESS;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Debug_Data_Msg
//
//  Purpose				: pack & transmit RTC status to HOST
//
//  Inputs				: Debug Data		- Debug string info
//
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			:
//
// ---------------------------------------------------------------------------
int Msg_Prot_Pack_n_Tx_Debug_Data_Msg(char* Debug_Data)
{

	Msg_Prot_RTC_Dbg_Msg_t  RTC_Debug_msg;

	int res;




		RTC_Debug_msg.Msg_Hdr.Id 					= MSG_PROT_RTC_DEBUG_MSG;

		RTC_Debug_msg.Msg_Hdr.Serial_Number 		= Msg_Prot_Get_Msg_SN();

		memcpy(RTC_Debug_msg.Debug_Data,Debug_Data, sizeof(RTC_Debug_msg.Debug_Data));

		RTC_Debug_msg.Msg_Hdr.Size 				= sizeof(RTC_Debug_msg.Msg_Hdr)+ sizeof( RTC_Debug_msg.Debug_Data);


#ifdef ONS_HOST
		unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
		struct timeval  	tv;
		gettimeofday(&tv, NULL);
		Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
		SYSTEMTIME time;
		GetSystemTime(&time);
		Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
		RTC_Debug_msg.Msg_Hdr.Time_Stamp = Tx_Time;
#endif // ONS_HOST

#ifdef ONS_RTC
    RTC_Debug_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

		res = USBComm_Send_Message( (USBComm_Buffer_t)&RTC_Debug_msg , RTC_Debug_msg.Msg_Hdr.Size );

		return res;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Debug_Data_Msg
//
//  Purpose				: Unpack Debug data
//
//  Inputs				: Msg_Buff
//
//  Outputs				: Debug Data		- Debug string info
//
//  Returns				: success or error code
//
//  Description			:
//
// ---------------------------------------------------------------------------
int Msg_Prot_Unpack_Debug_Data_Msg(ONS_BYTE* Msg_Buff, char* Debug_Data)
{
	char* msg_p;
	msg_p = (char*)Msg_Buff;
	
	memcpy(Debug_Data, msg_p,( sizeof(char)* MSG_PROT_RTC_DEBUG_DATA_SIZE));

#ifdef ONS_HOST
	Debug_Data[MSG_PROT_RTC_DEBUG_DATA_SIZE-1]= '\0';  //Trancat for safety
//	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
//	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Receive MSG_PROT_RTC_DEBUG_MSG");
//	ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 20, __FILENAME__, __LINE__, __func__, (char*)"Receive MSG_PROT_RTC_DEBUG_MSG");

//	Log_Write_String_Msg(__FILE__, __func__, __LINE__,MSG_PROT_RTC_DEBUG_DATA_SIZE , Debug_Data);
#endif

	return MSG_PROT_SUCCESS;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Protocol_Validation_Req
//
//  Purpose				: pack & transmit Protocol validation request
//
//  Inputs				: 
//
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			: 
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Pack_n_Tx_Protocol_Validation_Req ( void )
{

	Msg_Prot_Protocol_Validation_Req_Msg_t  protocol_validation_msg;

	int res;

		protocol_validation_msg.Msg_Hdr.Id 					= MSG_PROT_PROTOCOL_VALIDATION_REQ;

		protocol_validation_msg.Msg_Hdr.Serial_Number 		= Msg_Prot_Get_Msg_SN();

		protocol_validation_msg.Msg_Hdr.Size 				= sizeof(protocol_validation_msg.Msg_Hdr);

#ifdef ONS_HOST
		unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
		struct timeval  	tv;
		gettimeofday(&tv, NULL);
		Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
		SYSTEMTIME time;
		GetSystemTime(&time);
		Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
		protocol_validation_msg.Msg_Hdr.Time_Stamp = Tx_Time;
	//	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	//	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_PROTOCOL_VALIDATION_REQ");
		//ONS_Log.Update_Log_Data(LOG_CRITICAL_LEVEL, LOG_CATEGORY_RTC_COMM, 14, __FILENAME__, __LINdE__, __func__, (char*)"Send MSG_PROT_PROTOCOL_VALIDATION_REQ");
//		ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
	//	Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_PROTOCOL_VALIDATION_REQ, 0, 0,0,0,0,0);
#endif // ONS_HOST

#ifdef ONS_RTC
    protocol_validation_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

		res = USBComm_Send_Message( (USBComm_Buffer_t)&protocol_validation_msg , protocol_validation_msg.Msg_Hdr.Size );

		return res;
}

//int		Msg_Prot_Unpack_RTC_Init_Start_Cmd ( void );

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Protocol_Validation_Status
//
//  Purpose				: pack & transmit RTC Protocol Validation status to HOST
//
//  Inputs				: RTC_Major_Ver
//						  RTC_Minor_Ver
//
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			:
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Pack_n_Tx_Protocol_Validation_Status ( int RTC_Base_Ver , int RTC_Major_Ver, int RTC_Minor_Ver)
{

	Msg_Prot_Protocol_Validation_Status_Msg_t  RTC_Protocol_Validation_status_msg;

	int res;

		RTC_Protocol_Validation_status_msg.Msg_Hdr.Id 					= MSG_PROT_PROTOCOL_VALIDATION_STATUS;

		RTC_Protocol_Validation_status_msg.Msg_Hdr.Serial_Number 		= Msg_Prot_Get_Msg_SN();

        RTC_Protocol_Validation_status_msg.Version_Info.Msg_Prot_Base  = MSG_PROT_BASE_VERSION;
        
		RTC_Protocol_Validation_status_msg.Version_Info.Msg_Prot_Major = MSG_PROT_MAJOR_VERSION;

		RTC_Protocol_Validation_status_msg.Version_Info.Msg_Prot_Minor = MSG_PROT_MINOR_VERSION;

		RTC_Protocol_Validation_status_msg.Version_Info.RTC_Base_Ver	= RTC_Base_Ver;

		RTC_Protocol_Validation_status_msg.Version_Info.RTC_Major_Ver 	= RTC_Major_Ver;

		RTC_Protocol_Validation_status_msg.Version_Info.RTC_Minor_Ver	= RTC_Minor_Ver;

		RTC_Protocol_Validation_status_msg.Msg_Hdr.Size 				= sizeof(RTC_Protocol_Validation_status_msg.Msg_Hdr)+ sizeof( RTC_Protocol_Validation_status_msg.Version_Info);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
		RTC_Protocol_Validation_status_msg.Msg_Hdr.Time_Stamp = Tx_Time;
#endif // ONS_HOST


#ifdef ONS_RTC
    RTC_Protocol_Validation_status_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

		res = USBComm_Send_Message( (USBComm_Buffer_t)&RTC_Protocol_Validation_status_msg , RTC_Protocol_Validation_status_msg.Msg_Hdr.Size );

		return res;
}


// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Protocol_Validation_Status
//
//  Purpose				: unpack
//
//  Inputs				: Msg_Buff
//
//  Outputs				: RTC_Major_Ver
//						: RTC_Minor_Ver
//						: RTC_Msg_Prot_Major_Ver
//						: RTC_Msg_Prot_Minor_Ver
//
//  Returns				: Success
//
//  Description			: Get RTC and Message protocol versions
//
// ---------------------------------------------------------------------------
int		Msg_Prot_Unpack_Protocol_Validation_Status ( ONS_BYTE* Msg_Buff, int* RTC_Base_Ver, int* RTC_Major_Ver, int* RTC_Minor_Ver, int* RTC_Msg_Prot_Base_Ver, int* RTC_Msg_Prot_Major_Ver, int* RTC_Msg_Prot_Minor_Ver )
{
		Msg_Prot_RTC_Version_t* msg_p;

		msg_p = (Msg_Prot_RTC_Version_t*)Msg_Buff;

		*RTC_Base_Ver				= msg_p->RTC_Base_Ver;
		*RTC_Major_Ver 				= msg_p->RTC_Major_Ver;
		*RTC_Minor_Ver 				= msg_p->RTC_Minor_Ver;
		*RTC_Msg_Prot_Base_Ver		= msg_p->Msg_Prot_Base;
		*RTC_Msg_Prot_Major_Ver		= msg_p->Msg_Prot_Major;
		*RTC_Msg_Prot_Minor_Ver		= msg_p->Msg_Prot_Minor;

#ifdef ONS_HOST
		char	msg_txt[MAX_SYSLOG_DESC_SIZE];
		snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Receive MSG_PROT_PROTOCOL_VALIDATION_STATUS : rtc %d.%d.%d Comm %d.%d.%d", *RTC_Base_Ver, *RTC_Major_Ver, *RTC_Minor_Ver, *RTC_Msg_Prot_Base_Ver, *RTC_Msg_Prot_Major_Ver, *RTC_Msg_Prot_Minor_Ver);
		//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 21, __FILENAME__, __LINE__, __func__, msg_txt);
		//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
		//Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_PROTOCOL_VALIDATION_STATUS, *RTC_Base_Ver,*RTC_Major_Ver, *RTC_Minor_Ver, *RTC_Msg_Prot_Base_Ver,*RTC_Msg_Prot_Major_Ver,*RTC_Msg_Prot_Minor_Ver);
#endif
		return MSG_PROT_SUCCESS;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Lock_SH_State
//
//  Purpose				: pack & transmit Host req for Lock Sew head up
//
//  Inputs				: State : 0 - SH Lock at up pos , 1 - SH UNLock, 2 - SH Lock at the stame Pos
//
//
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			:
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Pack_n_Tx_Lock_SH_State(SH_UP_Pos_t State)
{

	Msg_Prot_Lock_SH_State_Msg_t   Lock_SH_State_msg;

	int res;

	Lock_SH_State_msg.Msg_Hdr.Id 					= MSG_PROT_SH_LOCK_STATE;

	Lock_SH_State_msg.Msg_Hdr.Serial_Number 		= Msg_Prot_Get_Msg_SN();

	Lock_SH_State_msg.Lock_SH_State 				= State;

	Lock_SH_State_msg.Msg_Hdr.Size 					= sizeof(Msg_Prot_Lock_SH_State_Msg_t);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	Lock_SH_State_msg.Msg_Hdr.Time_Stamp = Tx_Time;
		char	msg_txt[MAX_SYSLOG_DESC_SIZE];
		snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send Lock_SH_State_msg state = %d", State);
		//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 22, __FILENAME__, __LINE__, __func__, msg_txt);
		//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
#endif // ONS_HOST


#ifdef ONS_RTC
		Lock_SH_State_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

		res = USBComm_Send_Message( (USBComm_Buffer_t)&Lock_SH_State_msg, Lock_SH_State_msg.Msg_Hdr.Size );

		return res;
}

int	Msg_Prot_Pack_n_Tx_Lock_SH_State3020(SH_UP_Pos_t State,int Value)
{

	//Msg_Prot_Lock_SH_State_Msg_t   Lock_SH_State_msg;
	Msg_Prot_Lock_SH_State_Msg3020_t LockSH3020;
	int res;

	LockSH3020.Msg_Hdr.Id 					= MSG_PROT_SH_LOCK_STATE;

	LockSH3020.Msg_Hdr.Serial_Number 		= Msg_Prot_Get_Msg_SN();

	LockSH3020.Lock_SH_State 				= State;
	LockSH3020.Lock_SH_value 				= Value;

	LockSH3020.Msg_Hdr.Size 				= sizeof(Msg_Prot_Lock_SH_State_Msg3020_t);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	LockSH3020.Msg_Hdr.Time_Stamp = Tx_Time;
		char	msg_txt[MAX_SYSLOG_DESC_SIZE];
		snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send Lock_SH_State_msg state = %d", State);
		//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 22, __FILENAME__, __LINE__, __func__, msg_txt);
		//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
#endif // ONS_HOST


#ifdef ONS_RTC
		LockSH3020.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

		res = USBComm_Send_Message( (USBComm_Buffer_t)&LockSH3020, LockSH3020.Msg_Hdr.Size );

		return res;
}


// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Lock_SH_State
//
//  Purpose				: unpack
//
//  Inputs				: Msg_Buff
//
//  Outputs				: State
//
//  Returns				: Success
//
//  Description			: unpack the sew head lock at up position required state (lock/unlock)
//
// ---------------------------------------------------------------------------
int		Msg_Prot_Unpack_Lock_SH_State(ONS_BYTE* Msg_Buff, SH_UP_Pos_t* State)
{
		int* msg_p;

		msg_p	= (int*)Msg_Buff;

		*State 	= (SH_UP_Pos_t)*msg_p;

		return MSG_PROT_SUCCESS;
}


// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Update_Profile_1
//
//  Purpose				: pack & transmit RTC relevant Profile Data
//
//  Inputs				: Profile_1_Data - include all the data + update mask
//
//
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			: get the data structure that contains mask for all the update fields and the data.
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Pack_n_Tx_Update_Profile_1 ( Msg_Prot_Profile_1_Data_t  Profile_1_Data)
{

	Msg_Prot_Update_Profile_1_Msg_t   profile_update_data_msg;

	int res;


		profile_update_data_msg.Msg_Hdr.Id 					= MSG_PROT_UPDATE_PROFILE_1_MSG;

		profile_update_data_msg.Msg_Hdr.Serial_Number 		= Msg_Prot_Get_Msg_SN();

		profile_update_data_msg.RTC_Profile_1_Data			= Profile_1_Data;

		profile_update_data_msg.Msg_Hdr.Size 				= sizeof(profile_update_data_msg.Msg_Hdr)+ sizeof( profile_update_data_msg.RTC_Profile_1_Data);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else// ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
		profile_update_data_msg.Msg_Hdr.Time_Stamp = Tx_Time;
		//char	msg_txt[MAX_SYSLOG_DESC_SIZE];
		//snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_UPDATE_PROFILE_1_MSG");
		//: Update_Data_Mask = %d, ZS_Sew_Direction =%d, ZS_Direction_Change_Delay = %d, ZS_Lock_Delay = %d, ZS_Default_Update_Data_Position = %d, ZS_Default_Stop_Position = %d,Tension_Enabled = %d, Tension_Max_ED_Percentage=%d,Tension_PWM_Divider_Factor=%d,Tension_PWM_Period_In_uSec=%d,Tension_Total_Of_Levels=%d",
		//										profile_update_data_msg.RTC_Profile_1_Data.Update_Data_Mask, profile_update_data_msg.RTC_Profile_1_Data.ZS_Sew_Direction,
		//										profile_update_data_msg.RTC_Profile_1_Data.ZS_Direction_Change_Delay,profile_update_data_msg.RTC_Profile_1_Data.ZS_Lock_Delay,
		//										profile_update_data_msg.RTC_Profile_1_Data.ZS_Default_Update_Data_Position, profile_update_data_msg.RTC_Profile_1_Data.ZS_Default_Stop_Position,
		//										profile_update_data_msg.RTC_Profile_1_Data.Tension_Enabled, profile_update_data_msg.RTC_Profile_1_Data.Tension_Max_ED_Percentage, 
		//										profile_update_data_msg.RTC_Profile_1_Data.Tension_PWM_Divider_Factor, profile_update_data_msg.RTC_Profile_1_Data.Tension_PWM_Period_In_uSec, 
		//										profile_update_data_msg.RTC_Profile_1_Data.Tension_Total_Of_Levels);
//		ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
		//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 23, __FILENAME__, __LINE__, __func__, (char*)"Send MSG_PROT_UPDATE_PROFILE_1_MSG");
//		Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_UPDATE_PROFILE_1_MSG,
//						profile_update_data_msg.RTC_Profile_1_Data.Update_Data_Mask, profile_update_data_msg.RTC_Profile_1_Data.ZS_Sew_Direction,
//						profile_update_data_msg.RTC_Profile_1_Data.ZS_Direction_Change_Delay,profile_update_data_msg.RTC_Profile_1_Data.ZS_Lock_Delay,
//						profile_update_data_msg.RTC_Profile_1_Data.ZS_Default_Update_Data_Position,profile_update_data_msg.RTC_Profile_1_Data.ZS_Default_Stop_Position);
#endif // ONS_HOST

#ifdef ONS_RTC
    profile_update_data_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

		res = USBComm_Send_Message( (USBComm_Buffer_t)&profile_update_data_msg , profile_update_data_msg.Msg_Hdr.Size );

		return res;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Update_Profile_1
//
//  Purpose				: unpack
//
//  Inputs				: Msg_Buff
//
//  Outputs				: Profile_Data
//
//  Returns				: Success
//
//  Description			: Get Profile data and mask
//
// ---------------------------------------------------------------------------
int		Msg_Prot_Unpack_Update_Profile_1(ONS_BYTE* Msg_Buff,Msg_Prot_Profile_1_Data_t* Profile_1_Data)
{
	Msg_Prot_Profile_1_Data_t* msg_p;

	msg_p = (Msg_Prot_Profile_1_Data_t*)Msg_Buff;

 	memcpy(Profile_1_Data, msg_p, sizeof(Msg_Prot_Profile_1_Data_t));

	return MSG_PROT_SUCCESS;
}



// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Update_Profile_2
//
//  Purpose				: pack & transmit RTC relevant Profile Data
//
//  Inputs				: Profile_Data - include all the data + update mask
//
//
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			: get the data structure that contains mask for all the update fields and the data.
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Pack_n_Tx_Update_Profile_2 ( Msg_Prot_Profile_2_Data_t  Profile_2_Data)
{

	Msg_Prot_Update_Profile_2_Msg_t   profile_update_data_msg;

	int res;

		profile_update_data_msg.Msg_Hdr.Id 					= MSG_PROT_UPDATE_PROFILE_2_MSG;

		profile_update_data_msg.Msg_Hdr.Serial_Number 		= Msg_Prot_Get_Msg_SN();

		profile_update_data_msg.RTC_Profile_2_Data			= Profile_2_Data;

		profile_update_data_msg.Msg_Hdr.Size 				= sizeof(profile_update_data_msg.Msg_Hdr)+ sizeof( profile_update_data_msg.RTC_Profile_2_Data);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
		profile_update_data_msg.Msg_Hdr.Time_Stamp = Tx_Time;
		//char	msg_txt[MAX_SYSLOG_DESC_SIZE];
		//snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_UPDATE_PROFILE_2_MSG");
			//: Update_Data_Mask=%d,XYS_Direction_Change_Delay=%d,XYS_Init_X_Position=%d,XYS_Init_Y_Position=%d,XYS_X_Positive_Dir=%d,XYS_Y_Positive_Dir=%d,TB_Enabled=%d,TB_Num_Of_Stitches_In_Start_Mode=%d,TB_Reset_Cmd, TB_Reset_IO=%d,TB_Run_Threshold=%d,TB_Sensor_Id=%d,TB_Sensor_ON=%d, TB_Start_Threshold =%d",
			//			profile_update_data_msg.RTC_Profile_2_Data.Update_Data_Mask, profile_update_data_msg.RTC_Profile_2_Data.XYS_Direction_Change_Delay,
			//			profile_update_data_msg.RTC_Profile_2_Data.XYS_Init_X_Position,profile_update_data_msg.RTC_Profile_2_Data.XYS_Init_Y_Position,
			//			profile_update_data_msg.RTC_Profile_2_Data.XYS_X_Positive_Dir,profile_update_data_msg.RTC_Profile_2_Data.XYS_Y_Positive_Dir,
			//			profile_update_data_msg.RTC_Profile_2_Data.TB_Enabled, profile_update_data_msg.RTC_Profile_2_Data.TB_Num_Of_Stitches_In_Start_Mode,
			//			profile_update_data_msg.RTC_Profile_2_Data.TB_Reset_Cmd, profile_update_data_msg.RTC_Profile_2_Data.TB_Reset_IO,
			//			profile_update_data_msg.RTC_Profile_2_Data.TB_Run_Threshold, profile_update_data_msg.RTC_Profile_2_Data.TB_Sensor_Id,
			//			profile_update_data_msg.RTC_Profile_2_Data.TB_Sensor_ON, profile_update_data_msg.RTC_Profile_2_Data.TB_Start_Threshold);
//		ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
		//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 24, __FILENAME__, __LINE__, __func__, (char*)"Send MSG_PROT_UPDATE_PROFILE_2_MSG");

//		Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_UPDATE_PROFILE_2_MSG,
						

#endif // ONS_HOST

#ifdef ONS_RTC
    profile_update_data_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

		res = USBComm_Send_Message( (USBComm_Buffer_t)&profile_update_data_msg , profile_update_data_msg.Msg_Hdr.Size );

		return res;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Update_Profile_2
//
//  Purpose				: unpack
//
//  Inputs				: Msg_Buff
//
//  Outputs				: Profile_Data
//
//  Returns				: Success
//
//  Description			: Get Profile data and mask
//
// ---------------------------------------------------------------------------
int		Msg_Prot_Unpack_Update_Profile_2(ONS_BYTE* Msg_Buff,Msg_Prot_Profile_2_Data_t* Profile_2_Data)
{
	Msg_Prot_Profile_2_Data_t* msg_p;

	msg_p = (Msg_Prot_Profile_2_Data_t*)Msg_Buff;

 	memcpy(Profile_2_Data, msg_p, sizeof(Msg_Prot_Profile_2_Data_t));

	return MSG_PROT_SUCCESS;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Update_Profile_3
//
//  Purpose				: pack & transmit RTC relevant Profile Data
//
//  Inputs				: Profile_Data - include all the data + update mask
//
//
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			: get the data structure that contains mask for all the update fields and the data.
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Pack_n_Tx_Update_Profile_3(Msg_Prot_Profile_3_Data_t  Profile_3_Data)
{

	Msg_Prot_Update_Profile_3_Msg_t   profile_update_data_msg;

	int res;

	profile_update_data_msg.Msg_Hdr.Id = MSG_PROT_UPDATE_PROFILE_3_MSG;

	profile_update_data_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	profile_update_data_msg.RTC_Profile_3_Data = Profile_3_Data;

	profile_update_data_msg.Msg_Hdr.Size = sizeof(profile_update_data_msg.Msg_Hdr) + sizeof(profile_update_data_msg.RTC_Profile_3_Data);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	profile_update_data_msg.Msg_Hdr.Time_Stamp = Tx_Time;

//	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
//	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_UPDATE_PROFILE_3_MSG");
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 25, __FILENAME__, __LINE__, __func__, (char*)"Send MSG_PROT_UPDATE_PROFILE_3_MSG");
//	ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt); 
//	Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_UPDATE_PROFILE_3_MSG,
//		profile_update_data_msg.RTC_Profile_3_Data.Update_Data_Mask, profile_update_data_msg.RTC_Profile_3_Data.Max_XY_Accel,
//		profile_update_data_msg.RTC_Profile_3_Data.Max_XY_Speed, profile_update_data_msg.RTC_Profile_3_Data.Max_DT_Value,
//		profile_update_data_msg.RTC_Profile_3_Data.ISR_Time, profile_update_data_msg.RTC_Profile_3_Data.XY_Step,
//		profile_update_data_msg.RTC_Profile_3_Data. );

#endif // ONS_HOST

#ifdef ONS_RTC
	profile_update_data_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&profile_update_data_msg, profile_update_data_msg.Msg_Hdr.Size);

	return res;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Update_Profile_3
//
//  Purpose				: unpack
//
//  Inputs				: Msg_Buff
//
//  Outputs				: Profile_Data
//
//  Returns				: Success
//
//  Description			: Get Profile data and mask
//
// ---------------------------------------------------------------------------
int		Msg_Prot_Unpack_Update_Profile_3(ONS_BYTE* Msg_Buff, Msg_Prot_Profile_3_Data_t* Profile_3_Data)
{
	Msg_Prot_Profile_3_Data_t* msg_p;

	msg_p = (Msg_Prot_Profile_3_Data_t*)Msg_Buff;

	memcpy(Profile_3_Data, msg_p, sizeof(Msg_Prot_Profile_3_Data_t));

	return MSG_PROT_SUCCESS;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Update_Profile_4
//
//  Purpose				: pack & transmit RTC relevant Profile Data
//
//  Inputs				: Profile_Data - include all the data + update mask
//
//
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			: get the data structure that contains mask for all the update fields and the data.
//
// ---------------------------------------------------------------------------
int		Msg_Prot_Pack_n_Tx_Update_Profile_4(Msg_Prot_Profile_4_Data_t  Profile_Data)
{

	Msg_Prot_Update_Profile_4_Msg_t   profile_update_data_msg;

	int res;

	profile_update_data_msg.Msg_Hdr.Id = MSG_PROT_UPDATE_PROFILE_4_MSG;

	profile_update_data_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	profile_update_data_msg.RTC_Profile_4_Data = Profile_Data;

	profile_update_data_msg.Msg_Hdr.Size = sizeof(profile_update_data_msg.Msg_Hdr) + sizeof(profile_update_data_msg.RTC_Profile_4_Data);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	profile_update_data_msg.Msg_Hdr.Time_Stamp = Tx_Time;

	//char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	//snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_UPDATE_PROFILE_4_MSG");
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 26, __FILENAME__, __LINE__, __func__, (char*)"Send MSG_PROT_UPDATE_PROFILE_4_MSG");
//	ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt); 
//	Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_UPDATE_PROFILE_3_MSG,
//		profile_update_data_msg.RTC_Profile_4_Data.Update_Data_Mask, profile_update_data_msg.RTC_Profile_4_Data.XY_Home1_Speed,
//		profile_update_data_msg.RTC_Profile_4_Data.XY_Home1_X_Move, profile_update_data_msg.RTC_Profile_4_Data.XY_Home1_Y_Move,
//		profile_update_data_msg.RTC_Profile_4_Data.XY_Home2_Speed, profile_update_data_msg.RTC_Profile_4_Data.XY_Home2_X_Shift);

#endif // ONS_HOST

#ifdef ONS_RTC
	profile_update_data_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&profile_update_data_msg, profile_update_data_msg.Msg_Hdr.Size);

	return res;
}
//-------------------------------------------------------dec31,2019.
// 3020.
int		Msg_Prot_Pack_n_Tx_Update_Profile_4_3020(Msg_Prot_Profile_4_3020_Data_t  Profile_Data)
{

	Msg_Prot_Update_Profile_4_3020_Msg_t   profile_update_data_msg;

	int res;

	profile_update_data_msg.Msg_Hdr.Id = MSG_PROT_UPDATE_PROFILE_4_MSG;

	profile_update_data_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	profile_update_data_msg.RTC_Profile_4_3020_Data = Profile_Data;

	profile_update_data_msg.Msg_Hdr.Size = sizeof(profile_update_data_msg.Msg_Hdr) + sizeof(profile_update_data_msg.RTC_Profile_4_3020_Data);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	profile_update_data_msg.Msg_Hdr.Time_Stamp = Tx_Time;

#endif // ONS_HOST

#ifdef ONS_RTC
	profile_update_data_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&profile_update_data_msg, profile_update_data_msg.Msg_Hdr.Size);

	return res;
}
//--end_of_profile4_dec31,2019.



//-------------------------------------------

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Update_Profile_4
//
//  Purpose				: unpack
//
//  Inputs				: Msg_Buff
//
//  Outputs				: Profile_Data
//
//  Returns				: Success
//
//  Description			: Get Profile data and mask
//
// ---------------------------------------------------------------------------
int		Msg_Prot_Unpack_Update_Profile_4(ONS_BYTE* Msg_Buff, Msg_Prot_Profile_4_Data_t* Profile_4_Data)
{
	Msg_Prot_Profile_4_Data_t* msg_p;

	msg_p = (Msg_Prot_Profile_4_Data_t*)Msg_Buff;

	memcpy(Profile_4_Data, msg_p, sizeof(Msg_Prot_Profile_4_Data_t));

	return MSG_PROT_SUCCESS;
}


// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Update_Profile_5
//
//  Purpose				: pack & transmit RTC relevant Profile Data
//
//  Inputs				: Profile_Data - include all the data + update mask
//
//
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			: get the data structure that contains mask for all the update fields and the data.
//
// ---------------------------------------------------------------------------
int		Msg_Prot_Pack_n_Tx_Update_Profile_5(Msg_Prot_Profile_5_Data_t  Profile_Data)
{

	Msg_Prot_Update_Profile_5_Msg_t   profile_update_data_msg;

	int res;

	profile_update_data_msg.Msg_Hdr.Id = MSG_PROT_UPDATE_PROFILE_5_MSG;

	profile_update_data_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	profile_update_data_msg.RTC_Profile_5_Data = Profile_Data;

	profile_update_data_msg.Msg_Hdr.Size = sizeof(profile_update_data_msg.Msg_Hdr) + sizeof(profile_update_data_msg.RTC_Profile_5_Data);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	profile_update_data_msg.Msg_Hdr.Time_Stamp = Tx_Time;

	//char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	//snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_UPDATE_PROFILE_5_MSG");
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 27, __FILENAME__, __LINE__, __func__, (char*)"Send MSG_PROT_UPDATE_PROFILE_5_MSG");
//	ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt); 
//	Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_UPDATE_PROFILE_3_MSG,
//		profile_update_data_msg.RTC_Profile_5_Data.Run_State_Stoping_Mode_Timeout, profile_update_data_msg.RTC_Profile_5_Data.TB_Num_Of_Stitches_In_Start_Mode,
//		profile_update_data_msg.RTC_Profile_5_Data.SH_Up_Speed, profile_update_data_msg.RTC_Profile_5_Data.SH_Up_Stop_Angle,
//		profile_update_data_msg.RTC_Profile_5_Data.DR_STP_Dt_Vx, profile_update_data_msg.RTC_Profile_5_Data.DR_STP_Dt_Vy);

#endif // ONS_HOST

#ifdef ONS_RTC
	profile_update_data_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&profile_update_data_msg, profile_update_data_msg.Msg_Hdr.Size);

	return res;
}


// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Update_Profile_5
//
//  Purpose				: unpack
//
//  Inputs				: Msg_Buff
//
//  Outputs				: Profile_Data
//
//  Returns				: Success
//
//  Description			: Get Profile data and mask
//
// ---------------------------------------------------------------------------
int		Msg_Prot_Unpack_Update_Profile_5(ONS_BYTE* Msg_Buff, Msg_Prot_Profile_5_Data_t* Profile_5_Data)
{
	Msg_Prot_Profile_5_Data_t* msg_p;

	msg_p = (Msg_Prot_Profile_5_Data_t*)Msg_Buff;

	memcpy(Profile_5_Data, msg_p, sizeof(Msg_Prot_Profile_5_Data_t));

	return MSG_PROT_SUCCESS;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Update_Profile_6
//
//  Purpose				: pack & transmit RTC relevant Profile Data
//
//  Inputs				: Profile_Data - include all the data + update mask
//
//
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			: get the data structure that contains mask for all the update fields and the data.
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Pack_n_Tx_Update_Profile_6(Msg_Prot_Profile_6_Data_t  Profile_Data)
{

	Msg_Prot_Update_Profile_6_Msg_t		profile_update_data_msg;
#ifdef ONS_HOST
	//char								msg_txt[MAX_SYSLOG_DESC_SIZE];
#endif
	int									res;

	profile_update_data_msg.Msg_Hdr.Id = MSG_PROT_UPDATE_PROFILE_6_MSG;

	profile_update_data_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	profile_update_data_msg.RTC_Profile_6_Data = Profile_Data;

	profile_update_data_msg.Msg_Hdr.Size = sizeof(profile_update_data_msg.Msg_Hdr) + sizeof(profile_update_data_msg.RTC_Profile_6_Data);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	profile_update_data_msg.Msg_Hdr.Time_Stamp = Tx_Time;

//	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_UPDATE_PROFILE_6_MSG");
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 28, __FILENAME__, __LINE__, __func__, (char*)"Send MSG_PROT_UPDATE_PROFILE_6_MSG");
//	ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
#endif // ONS_HOST

#ifdef ONS_RTC
	profile_update_data_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&profile_update_data_msg, profile_update_data_msg.Msg_Hdr.Size);

	return res;
}


// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Update_Profile_6
//
//  Purpose				: unpack
//
//  Inputs				: Msg_Buff
//
//  Outputs				: Profile_Data
//
//  Returns				: Success
//
//  Description			: Get Profile data and mask
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Unpack_Update_Profile_6(ONS_BYTE* Msg_Buff, Msg_Prot_Profile_6_Data_t* Profile_6_Data)
{
	Msg_Prot_Profile_6_Data_t* msg_p;

	msg_p = (Msg_Prot_Profile_6_Data_t*)Msg_Buff;

	memcpy(Profile_6_Data, msg_p, sizeof(Msg_Prot_Profile_6_Data_t));

	return MSG_PROT_SUCCESS;
}


// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Update_Profile_7
//
//  Purpose				: pack & transmit RTC relevant Profile Data
//
//  Inputs				: Profile_Data - include all the data + update mask
//
//
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			: get the data structure that contains mask for all the update fields and the data.
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Pack_n_Tx_Update_Profile_7(Msg_Prot_Profile_7_Data_t  Profile_Data)
{

	Msg_Prot_Update_Profile_7_Msg_t		profile_update_data_msg;
#ifdef ONS_HOST
//	char								msg_txt[MAX_SYSLOG_DESC_SIZE];
#endif
	int									res;

	profile_update_data_msg.Msg_Hdr.Id = MSG_PROT_UPDATE_PROFILE_7_MSG;

	profile_update_data_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	profile_update_data_msg.RTC_Profile_7_Data = Profile_Data;

	profile_update_data_msg.Msg_Hdr.Size = sizeof(profile_update_data_msg.Msg_Hdr) + sizeof(profile_update_data_msg.RTC_Profile_7_Data);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	profile_update_data_msg.Msg_Hdr.Time_Stamp = Tx_Time;

	//snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_UPDATE_PROFILE_7_MSG");
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 29, __FILENAME__, __LINE__, __func__, (char*)"Send MSG_PROT_UPDATE_PROFILE_7_MSG");
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
#endif // ONS_HOST

#ifdef ONS_RTC
	profile_update_data_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&profile_update_data_msg, profile_update_data_msg.Msg_Hdr.Size);

	return res;
}

//-------------------------------------------------------dec31,2019.
// 3020.
int	Msg_Prot_Pack_n_Tx_Update_Profile_7_3020(Msg_Prot_Profile_7_3020_Data_t  Profile_Data)
{

	Msg_Prot_Update_Profile_7_3020_Msg_t		profile_update_data_msg;
#ifdef ONS_HOST
//	char								msg_txt[MAX_SYSLOG_DESC_SIZE];
#endif
	int									res;

	profile_update_data_msg.Msg_Hdr.Id = MSG_PROT_UPDATE_PROFILE_7_MSG;

	profile_update_data_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	profile_update_data_msg.RTC_Profile_7_3020_Data = Profile_Data;

	profile_update_data_msg.Msg_Hdr.Size = sizeof(profile_update_data_msg.Msg_Hdr) + sizeof(profile_update_data_msg.RTC_Profile_7_3020_Data);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	profile_update_data_msg.Msg_Hdr.Time_Stamp = Tx_Time;

	#endif // ONS_HOST

#ifdef ONS_RTC
	profile_update_data_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&profile_update_data_msg, profile_update_data_msg.Msg_Hdr.Size);

	return res;
}
//--end_of_profile7_dec31,2019.

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Update_Profile_7
//
//  Purpose				: unpack
//
//  Inputs				: Msg_Buff
//
//  Outputs				: Profile_Data
//
//  Returns				: Success
//
//  Description			: Get Profile data and mask
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Unpack_Update_Profile_7(ONS_BYTE* Msg_Buff, Msg_Prot_Profile_7_Data_t* Profile_7_Data)
{
	Msg_Prot_Profile_7_Data_t* msg_p;

	msg_p = (Msg_Prot_Profile_7_Data_t*)Msg_Buff;

	memcpy(Profile_7_Data, msg_p, sizeof(Msg_Prot_Profile_7_Data_t));

	return MSG_PROT_SUCCESS;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Update_Profile_8
//
//  Purpose				: pack & transmit RTC relevant Profile Data
//
//  Inputs				: Profile_Data - include all the data + update mask
//
//
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			: get the data structure that contains mask for all the update fields and the data.
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Pack_n_Tx_Update_Profile_8(Msg_Prot_Profile_8_Data_t  Profile_Data)
{

	Msg_Prot_Update_Profile_8_Msg_t		profile_update_data_msg;
#ifdef ONS_HOST
	//char								msg_txt[MAX_SYSLOG_DESC_SIZE];
#endif
	int									res;

	profile_update_data_msg.Msg_Hdr.Id = MSG_PROT_UPDATE_PROFILE_8_MSG;

	profile_update_data_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	profile_update_data_msg.RTC_Profile_8_Data = Profile_Data;
	memcpy(&profile_update_data_msg.RTC_Profile_8_Data, &Profile_Data, sizeof(Msg_Prot_Profile_8_Data_t));

	profile_update_data_msg.Msg_Hdr.Size = sizeof(profile_update_data_msg.Msg_Hdr) + sizeof(profile_update_data_msg.RTC_Profile_8_Data);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	profile_update_data_msg.Msg_Hdr.Time_Stamp = Tx_Time;

	//snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_UPDATE_PROFILE_8_MSG");
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 30, __FILENAME__, __LINE__, __func__, (char*)"Send MSG_PROT_UPDATE_PROFILE_8_MSG");
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
#endif // ONS_HOST

#ifdef ONS_RTC
	profile_update_data_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&profile_update_data_msg, profile_update_data_msg.Msg_Hdr.Size);

	return res;
}


// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Update_Profile_8
//
//  Purpose				: unpack
//
//  Inputs				: Msg_Buff
//
//  Outputs				: Profile_Data
//
//  Returns				: Success
//
//  Description			: Get Profile data and mask
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Unpack_Update_Profile_8(ONS_BYTE* Msg_Buff, Msg_Prot_Profile_8_Data_t* Profile_8_Data)
{
	Msg_Prot_Profile_8_Data_t* msg_p;

	msg_p = (Msg_Prot_Profile_8_Data_t*)Msg_Buff;

	memcpy(Profile_8_Data, msg_p, sizeof(Msg_Prot_Profile_8_Data_t));

	return MSG_PROT_SUCCESS;
}


// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Update_Profile_9
//
//  Purpose				: pack & transmit RTC relevant Profile Data
//
//  Inputs				: Profile_Data - include all the data + update mask
//
//
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			: get the data structure that contains mask for all the update fields and the data.
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Pack_n_Tx_Update_Profile_9(Msg_Prot_Profile_9_Data_t  Profile_Data)
{

	Msg_Prot_Update_Profile_9_Msg_t		profile_update_data_msg;
#ifdef ONS_HOST
	//char								msg_txt[MAX_SYSLOG_DESC_SIZE];
#endif
	int									res;

	profile_update_data_msg.Msg_Hdr.Id = MSG_PROT_UPDATE_PROFILE_9_MSG;

	profile_update_data_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	profile_update_data_msg.RTC_Profile_9_Data = Profile_Data;

	profile_update_data_msg.Msg_Hdr.Size = sizeof(profile_update_data_msg.Msg_Hdr) + sizeof(profile_update_data_msg.RTC_Profile_9_Data);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	profile_update_data_msg.Msg_Hdr.Time_Stamp = Tx_Time;

	//snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_UPDATE_PROFILE_9_MSG");
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 31, __FILENAME__, __LINE__, __func__, (char*)"Send MSG_PROT_UPDATE_PROFILE_9_MSG");
//	ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
#endif // ONS_HOST

#ifdef ONS_RTC
	profile_update_data_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&profile_update_data_msg, profile_update_data_msg.Msg_Hdr.Size);

	return res;
}

//-------------------------------------------------------dec31,2019.
// 3020.
int	Msg_Prot_Pack_n_Tx_Update_Profile_9_3020(Msg_Prot_Profile_9_3020_Data_t  Profile_Data)
{

	Msg_Prot_Update_Profile_9_3020_Msg_t		profile_update_data_msg;
#ifdef ONS_HOST
//	char								msg_txt[MAX_SYSLOG_DESC_SIZE];
#endif
	int									res;

	profile_update_data_msg.Msg_Hdr.Id = MSG_PROT_UPDATE_PROFILE_9_MSG;

	profile_update_data_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	profile_update_data_msg.RTC_Profile_9_3020_Data = Profile_Data;

	profile_update_data_msg.Msg_Hdr.Size = sizeof(profile_update_data_msg.Msg_Hdr) + sizeof(profile_update_data_msg.RTC_Profile_9_3020_Data);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	profile_update_data_msg.Msg_Hdr.Time_Stamp = Tx_Time;

	#endif // ONS_HOST

#ifdef ONS_RTC
	profile_update_data_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&profile_update_data_msg, profile_update_data_msg.Msg_Hdr.Size);

	return res;
}
//--end_of_profile9_dec31,2019.

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Update_Profile_9
//
//  Purpose				: unpack
//
//  Inputs				: Msg_Buff
//
//  Outputs				: Profile_Data
//
//  Returns				: Success
//
//  Description			: Get Profile data and mask
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Unpack_Update_Profile_9(ONS_BYTE* Msg_Buff, Msg_Prot_Profile_9_Data_t* Profile_9_Data)
{
	Msg_Prot_Profile_9_Data_t* msg_p;

	msg_p = (Msg_Prot_Profile_9_Data_t*)Msg_Buff;

	memcpy(Profile_9_Data, msg_p, sizeof(Msg_Prot_Profile_9_Data_t));

	return MSG_PROT_SUCCESS;
}


// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Update_Profile_10
//
//  Purpose				: pack & transmit RTC relevant Profile Data
//
//  Inputs				: Profile_Data - include all the data + update mask
//
//
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			: get the data structure that contains mask for all the update fields and the data.
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Pack_n_Tx_Update_Profile_10(Msg_Prot_Profile_10_Data_t  Profile_Data)
{

	Msg_Prot_Update_Profile_10_Msg_t		profile_update_data_msg;
#ifdef ONS_HOST
//	char								msg_txt[MAX_SYSLOG_DESC_SIZE];
#endif
	int									res;

	profile_update_data_msg.Msg_Hdr.Id = MSG_PROT_UPDATE_PROFILE_10_MSG;

	profile_update_data_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	profile_update_data_msg.RTC_Profile_10_Data = Profile_Data;

	profile_update_data_msg.Msg_Hdr.Size = sizeof(profile_update_data_msg.Msg_Hdr) + sizeof(profile_update_data_msg.RTC_Profile_10_Data);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	profile_update_data_msg.Msg_Hdr.Time_Stamp = Tx_Time;

//	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_UPDATE_PROFILE_10_MSG");
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 32, __FILENAME__, __LINE__, __func__, (char*)"Send MSG_PROT_UPDATE_PROFILE_10_MSG");
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
#endif // ONS_HOST

#ifdef ONS_RTC
	profile_update_data_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif
	res = USBComm_Send_Message((USBComm_Buffer_t)&profile_update_data_msg, profile_update_data_msg.Msg_Hdr.Size);

	return res;
}

//-------------------------------------------------------dec31,2019.
// 3020.
int	Msg_Prot_Pack_n_Tx_Update_Profile_10_3020(Msg_Prot_Profile_10_3020_Data_t  Profile_Data)
{

	Msg_Prot_Update_Profile_10_3020_Msg_t		profile_update_data_msg;
#ifdef ONS_HOST
//	char								msg_txt[MAX_SYSLOG_DESC_SIZE];
#endif
	int									res;

	profile_update_data_msg.Msg_Hdr.Id = MSG_PROT_UPDATE_PROFILE_10_MSG;

	profile_update_data_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	profile_update_data_msg.RTC_Profile_10_3020_Data = Profile_Data;

	profile_update_data_msg.Msg_Hdr.Size = sizeof(profile_update_data_msg.Msg_Hdr) + sizeof(profile_update_data_msg.RTC_Profile_10_3020_Data);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	profile_update_data_msg.Msg_Hdr.Time_Stamp = Tx_Time;

	#endif // ONS_HOST

#ifdef ONS_RTC
	profile_update_data_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&profile_update_data_msg, profile_update_data_msg.Msg_Hdr.Size);

	return res;
}
//--end_of_profile9_dec31,2019.


// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Update_Profile_10
//
//  Purpose				: unpack
//
//  Inputs				: Msg_Buff
//
//  Outputs				: Profile_Data
//
//  Returns				: Success
//
//  Description			: Get Profile data and mask
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Unpack_Update_Profile_10(ONS_BYTE* Msg_Buff, Msg_Prot_Profile_10_Data_t* Profile_10_Data)
{
	Msg_Prot_Profile_10_Data_t* msg_p;

	msg_p = (Msg_Prot_Profile_10_Data_t*)Msg_Buff;

	memcpy(Profile_10_Data, msg_p, sizeof(Msg_Prot_Profile_10_Data_t));

	return MSG_PROT_SUCCESS;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Update_Profile_11
//
//  Purpose				: pack & transmit RTC relevant Profile Data
//
//  Inputs				: Profile_Data - include all the data + update mask
//
//
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			: get the data structure that contains mask for all the update fields and the data.
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Pack_n_Tx_Update_Profile_11(Msg_Prot_Profile_11_Data_t  Profile_Data)
{

	Msg_Prot_Update_Profile_11_Msg_t		profile_update_data_msg;
#ifdef ONS_HOST
//	char								msg_txt[MAX_SYSLOG_DESC_SIZE];
#endif
	int									res;

	profile_update_data_msg.Msg_Hdr.Id = MSG_PROT_UPDATE_PROFILE_11_MSG;

	profile_update_data_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	profile_update_data_msg.RTC_Profile_11_Data = Profile_Data;

	profile_update_data_msg.Msg_Hdr.Size = sizeof(profile_update_data_msg.Msg_Hdr) + sizeof(profile_update_data_msg.RTC_Profile_11_Data);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	profile_update_data_msg.Msg_Hdr.Time_Stamp = Tx_Time;

//	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_UPDATE_PROFILE_11_MSG");
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 33, __FILENAME__, __LINE__, __func__, (char*)"Send MSG_PROT_UPDATE_PROFILE_11_MSG");
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
#endif // ONS_HOST

#ifdef ONS_RTC
	profile_update_data_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&profile_update_data_msg, profile_update_data_msg.Msg_Hdr.Size);

	return res;
}


// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Update_Profile_11
//
//  Purpose				: unpack
//
//  Inputs				: Msg_Buff
//
//  Outputs				: Profile_Data
//
//  Returns				: Success
//
//  Description			: Get Profile data and mask
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Unpack_Update_Profile_11(ONS_BYTE* Msg_Buff, Msg_Prot_Profile_11_Data_t* Profile_11_Data)
{
	Msg_Prot_Profile_11_Data_t* msg_p;

	msg_p = (Msg_Prot_Profile_11_Data_t*)Msg_Buff;

	memcpy(Profile_11_Data, msg_p, sizeof(Msg_Prot_Profile_11_Data_t));

	return MSG_PROT_SUCCESS;
}


// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Update_Profile_12
//
//  Purpose				: pack & transmit RTC relevant Profile Data
//
//  Inputs				: Profile_Data - include all the data + update mask
//
//
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			: get the data structure that contains mask for all the update fields and the data.
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Pack_n_Tx_Update_Profile_12(Msg_Prot_Profile_12_Data_t  Profile_Data)
{

	Msg_Prot_Update_Profile_12_Msg_t		profile_update_data_msg;
#ifdef ONS_HOST
	//char								msg_txt[MAX_SYSLOG_DESC_SIZE];
#endif
	int									res;

	profile_update_data_msg.Msg_Hdr.Id = MSG_PROT_UPDATE_PROFILE_12_MSG;

	profile_update_data_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	profile_update_data_msg.RTC_Profile_12_Data = Profile_Data;

	profile_update_data_msg.Msg_Hdr.Size = sizeof(profile_update_data_msg.Msg_Hdr) + sizeof(profile_update_data_msg.RTC_Profile_12_Data);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	profile_update_data_msg.Msg_Hdr.Time_Stamp = Tx_Time;

	//snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_UPDATE_PROFILE_12_MSG");
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 34, __FILENAME__, __LINE__, __func__, (char*)"Send MSG_PROT_UPDATE_PROFILE_12_MSG");
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
#endif // ONS_HOST

#ifdef ONS_RTC
	profile_update_data_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&profile_update_data_msg, profile_update_data_msg.Msg_Hdr.Size);

	return res;
}


// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Update_Profile_12
//
//  Purpose				: unpack
//
//  Inputs				: Msg_Buff
//
//  Outputs				: Profile_Data
//
//  Returns				: Success
//
//  Description			: Get Profile data and mask
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Unpack_Update_Profile_12(ONS_BYTE* Msg_Buff, Msg_Prot_Profile_12_Data_t* Profile_12_Data)
{
	Msg_Prot_Profile_12_Data_t* msg_p;

	msg_p = (Msg_Prot_Profile_12_Data_t*)Msg_Buff;

	memcpy(Profile_12_Data, msg_p, sizeof(Msg_Prot_Profile_12_Data_t));

	return MSG_PROT_SUCCESS;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Update_Profile_13
//
//  Purpose				: pack & transmit RTC relevant Profile Data
//
//  Inputs				: Profile_Data - include all the data + update mask
//
//
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			: get the data structure that contains mask for all the update fields and the data.
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Pack_n_Tx_Update_Profile_13(Msg_Prot_Profile_13_Data_t  Profile_Data)
{

	Msg_Prot_Update_Profile_13_Msg_t		profile_update_data_msg;
#ifdef ONS_HOST
//	char								msg_txt[MAX_SYSLOG_DESC_SIZE];
#endif
	int									res;

	profile_update_data_msg.Msg_Hdr.Id = MSG_PROT_UPDATE_PROFILE_13_MSG;

	profile_update_data_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	profile_update_data_msg.RTC_Profile_13_Data = Profile_Data;

	profile_update_data_msg.Msg_Hdr.Size = sizeof(profile_update_data_msg.Msg_Hdr) + sizeof(profile_update_data_msg.RTC_Profile_13_Data);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	profile_update_data_msg.Msg_Hdr.Time_Stamp = Tx_Time;

	//snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_UPDATE_PROFILE_13_MSG");
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 35, __FILENAME__, __LINE__, __func__, (char*)"Send MSG_PROT_UPDATE_PROFILE_13_MSG");
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
#endif // ONS_HOST

#ifdef ONS_RTC
	profile_update_data_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&profile_update_data_msg, profile_update_data_msg.Msg_Hdr.Size);

	return res;
}


// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Update_Profile_13
//
//  Purpose				: unpack
//
//  Inputs				: Msg_Buff
//
//  Outputs				: Profile_Data
//
//  Returns				: Success
//
//  Description			: Get Profile data and mask
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Unpack_Update_Profile_13(ONS_BYTE* Msg_Buff, Msg_Prot_Profile_13_Data_t* Profile_13_Data)
{
	Msg_Prot_Profile_13_Data_t* msg_p;

	msg_p = (Msg_Prot_Profile_13_Data_t*)Msg_Buff;

	memcpy(Profile_13_Data, msg_p, sizeof(Msg_Prot_Profile_13_Data_t));

	return MSG_PROT_SUCCESS;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Update_Profile_14
//
//  Purpose				: pack & transmit RTC relevant Profile Data
//
//  Inputs				: Profile_Data - include all the data + update mask
//
//
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			: get the data structure that contains mask for all the update fields and the data.
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Pack_n_Tx_Update_Profile_14(Msg_Prot_Profile_14_Data_t  Profile_Data)
{

	Msg_Prot_Update_Profile_14_Msg_t		profile_update_data_msg;
#ifdef ONS_HOST
//	char								msg_txt[MAX_SYSLOG_DESC_SIZE];
#endif
	int									res;

	profile_update_data_msg.Msg_Hdr.Id = MSG_PROT_UPDATE_PROFILE_14_MSG;

	profile_update_data_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	profile_update_data_msg.RTC_Profile_14_Data = Profile_Data;
	memcpy(&profile_update_data_msg.RTC_Profile_14_Data, &Profile_Data, sizeof(Msg_Prot_Profile_14_Data_t));

	profile_update_data_msg.Msg_Hdr.Size = sizeof(profile_update_data_msg.Msg_Hdr) + sizeof(profile_update_data_msg.RTC_Profile_14_Data);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	profile_update_data_msg.Msg_Hdr.Time_Stamp = Tx_Time;

//	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_UPDATE_PROFILE_14_MSG");
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 36, __FILENAME__, __LINE__, __func__, (char*)"Send MSG_PROT_UPDATE_PROFILE_14_MSG");
//	ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
#endif // ONS_HOST

#ifdef ONS_RTC
	profile_update_data_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&profile_update_data_msg, profile_update_data_msg.Msg_Hdr.Size);

	return res;
}


// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Update_Profile_14
//
//  Purpose				: unpack
//
//  Inputs				: Msg_Buff
//
//  Outputs				: Profile_Data
//
//  Returns				: Success
//
//  Description			: Get Profile data and mask
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Unpack_Update_Profile_14(ONS_BYTE* Msg_Buff, Msg_Prot_Profile_14_Data_t* Profile_14_Data)
{
	Msg_Prot_Profile_14_Data_t* msg_p;

	msg_p = (Msg_Prot_Profile_14_Data_t*)Msg_Buff;

	memcpy(Profile_14_Data, msg_p, sizeof(Msg_Prot_Profile_14_Data_t));

	return MSG_PROT_SUCCESS;
}


// Lock/Unlock XY Pos
// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Lock_XY_Pos
//
//  Purpose				: pack & transmit Host req for Lock Sew head up
//
//  Inputs				: State : 0 - XY Lock, 1 - XY UNLock
//
//
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			:
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Pack_n_Tx_Lock_XY_Pos(XY_Lock_State_t State)
{

	Msg_Prot_Lock_XY_Pos_Msg_t   Lock_XY_Pos_msg;

	int res;

	Lock_XY_Pos_msg.Msg_Hdr.Id = MSG_PROT_LOCK_XY;

	Lock_XY_Pos_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	Lock_XY_Pos_msg.XY_Pos_State = State;

	Lock_XY_Pos_msg.Msg_Hdr.Size = sizeof(Lock_XY_Pos_msg.Msg_Hdr) + sizeof(Lock_XY_Pos_msg.XY_Pos_State);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	Lock_XY_Pos_msg.Msg_Hdr.Time_Stamp = Tx_Time;
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_LOCK_XY state = %d", State);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 96, __FILENAME__, __LINE__, __func__, msg_txt);

//	ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
	//	Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_RTC_INIT_STATUS, State, 0,0,0,0,0);
#endif // ONS_HOST


#ifdef ONS_RTC
	Lock_XY_Pos_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&Lock_XY_Pos_msg, Lock_XY_Pos_msg.Msg_Hdr.Size);

	return res;
}


// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Lock_XY_Pos
//
//  Purpose				: unpack
//
//  Inputs				: Msg_Buff
//
//  Outputs				: State
//
//  Returns				: Success
//
//  Description			: unpack the XY lock position required state (lock/unlock)
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Unpack_Lock_XY_Pos(ONS_BYTE* Msg_Buff, XY_Lock_State_t* State)
{
#ifdef _ROLE_VIRTUAL_RTC
    INT_16* msg_p;

    msg_p = (INT_16*)Msg_Buff;

    *State = (XY_Lock_State_t)*msg_p;
#else
	int* msg_p;

	msg_p = (int*)Msg_Buff;

	*State = (XY_Lock_State_t)*msg_p;
#endif
	return MSG_PROT_SUCCESS;
}


// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Profile_Update_Status
//
//  Purpose				: pack & transmit RTC Update Profile Status
//
//  Inputs				: Profile_Type_Num , Status
//
//
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			: Send host status for each profile that was update.
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Pack_n_Tx_Profile_Update_Status(int Profile_Type_Num, ONS_Profile_Update_Status Status)
{

	Msg_Prot_Profile_Update_Status_Msg_t   profile_update_status_msg;

	int res;

		profile_update_status_msg.Msg_Hdr.Id 					= MSG_PROT_UPDATE_PROFILE_STATUS_MSG;

		profile_update_status_msg.Msg_Hdr.Serial_Number 		= Msg_Prot_Get_Msg_SN();

		profile_update_status_msg.Info.Profile_Update_Status	= Status;

		profile_update_status_msg.Info.Profile_Type_Num			= Profile_Type_Num;

		profile_update_status_msg.Msg_Hdr.Size 					= sizeof(profile_update_status_msg.Msg_Hdr)+ sizeof( profile_update_status_msg.Info);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
		profile_update_status_msg.Msg_Hdr.Time_Stamp = Tx_Time;
		char	msg_txt[MAX_SYSLOG_DESC_SIZE];
		snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_UPDATE_PROFILE_STATUS_MSG : Profile_Type_Num = %d,Profile_Update_Status =%d ", profile_update_status_msg.Info.Profile_Type_Num, profile_update_status_msg.Info.Profile_Update_Status);
		//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 37, __FILENAME__, __LINE__, __func__, msg_txt);
		//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
	//	Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_UPDATE_PROFILE_STATUS_MSG,	profile_update_status_msg.Info.Profile_Type_Num, profile_update_status_msg.Info.Profile_Update_Status,0 ,0 ,0 ,0 );
#endif // ONS_HOST

#ifdef ONS_RTC
    profile_update_status_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

		res = USBComm_Send_Message( (USBComm_Buffer_t)&profile_update_status_msg , profile_update_status_msg.Msg_Hdr.Size );

		return res;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Profile_Update_Status
//
//  Purpose				: unpack status from RTC regarding the profile update
//
//  Inputs				: Msg_Buff
//
//  Outputs				: Profile_Type_Num, Profile_Update_Status.
//
//  Returns				: Success
//
//  Description			: Get Profile data and mask
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Unpack_Profile_Update_Status(ONS_BYTE* Msg_Buff, int* Profile_Type_Num, ONS_Profile_Update_Status* Profile_Update_Status)
{
	Msg_Prot_Profile_Update_Status_t* msg_p;

	msg_p = (Msg_Prot_Profile_Update_Status_t*)Msg_Buff;

	*Profile_Type_Num = msg_p->Profile_Type_Num;

	*Profile_Update_Status = (ONS_Profile_Update_Status)msg_p->Profile_Update_Status;

#ifdef ONS_HOST

	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Receive MSG_PROT_UPDATE_PROFILE_STATUS_MSG : Profile_Type_Num=%d, Profile_Update_Status=%d", *Profile_Type_Num, *Profile_Update_Status);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 38, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
	//Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_UPDATE_PROFILE_STATUS_MSG,	*Profile_Type_Num, *Profile_Update_Status,0 ,0 ,0 ,0 );

#endif

	return MSG_PROT_SUCCESS;
}



// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Preform_INIT_Req
//
//  Purpose				: pack INIT request and send it to RTC
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				: Success or Error code
//
//  Description			: request RTC to perform INIT operation
//
// ---------------------------------------------------------------------------
int		Msg_Prot_Pack_n_Tx_Preform_INIT_Req (void)
{

	Msg_Prot_Perform_Init_Req_Msg_t  perform_init_req_msg;

	int res;

		perform_init_req_msg.Msg_Hdr.Id 				= MSG_PROT_PERFORM_INIT_REQ;

		perform_init_req_msg.Msg_Hdr.Serial_Number 		= Msg_Prot_Get_Msg_SN();

		perform_init_req_msg.Msg_Hdr.Size 				= sizeof(perform_init_req_msg.Msg_Hdr);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
		perform_init_req_msg.Msg_Hdr.Time_Stamp = Tx_Time;
	//	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	//	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_PERFORM_INIT_REQ");
		//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 39, __FILENAME__, __LINE__, __func__, (char*)"Send MSG_PROT_PERFORM_INIT_REQ");
		//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
		//Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_PERFORM_INIT_REQ, 0, 0,0,0,0,0);
#endif // ONS_HOST

#ifdef ONS_RTC
	perform_init_req_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

		res = USBComm_Send_Message( (USBComm_Buffer_t)&perform_init_req_msg , perform_init_req_msg.Msg_Hdr.Size );

		return res;
}



// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_RTC_Init_Status
//
//  Purpose				: pack INIT Status and send it to HOST
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				: Success or Error code
//
//  Description			: send RTC INIT status to HOST
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Pack_n_Tx_RTC_Init_Status (Msg_Prot_RTC_Init_Status_t Status)
{

	Msg_Prot_RTC_Init_Status_Msg_t  perform_init_status_msg;

	int res;

		perform_init_status_msg.Msg_Hdr.Id 					= MSG_PROT_RTC_INIT_STATUS;

		perform_init_status_msg.Msg_Hdr.Serial_Number 		= Msg_Prot_Get_Msg_SN();

		perform_init_status_msg.Msg_Hdr.Size 				= sizeof(Msg_Prot_RTC_Init_Status_Msg_t);

		perform_init_status_msg.Status						= Status;

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
		perform_init_status_msg.Msg_Hdr.Time_Stamp = Tx_Time;
	//	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	//	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_RTC_INIT_STATUS");
		//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 40, __FILENAME__, __LINE__, __func__, (char*)"Send MSG_PROT_RTC_INIT_STATUS");
	//	ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
	//	Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_RTC_INIT_STATUS, 0, 0,0,0,0,0);
#endif // ONS_HOST


#ifdef ONS_RTC
    perform_init_status_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

		res = USBComm_Send_Message( (USBComm_Buffer_t)&perform_init_status_msg , perform_init_status_msg.Msg_Hdr.Size );

		return res;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_RTC_Init_Status
//
//  Purpose				: unpack RTC INIT Status
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				: Success or Error code
//
//  Description			: get RTC INIT status
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Unpack_RTC_Init_Status (ONS_BYTE* Msg_Buff,INT_16* Status)
{
	INT_16* msg_p;

	msg_p = (INT_16*)Msg_Buff;

	*Status = *msg_p;

#ifdef ONS_HOST

	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Receive MSG_PROT_RTC_INIT_STATUS : Status = %d", *Status);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 41, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
	printf("%s\n",msg_txt);

#endif

	return MSG_PROT_SUCCESS;
}



// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Ping_Pong_Msg
//
//  Purpose				: FOR TESTING : Pack & Send Ping Pong message
//
//  Inputs				: Ping_Initiator - the Initator of the Ping
//						  Initiator_Msg_Id - The Initiator Msg ID
//						  Data_1 -> Data_5 - data parames
//
//  Outputs				:
//
//  Returns				: Success or Error code
//
//  Description			:pack & send ping/pong message to/from HOST 
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Pack_n_Tx_Ping_Pong_Msg(Ping_Initiator_t Ping_Initiator, int Initiator_Msg_Id, int Data_1, int Data_2, int Data_3, int Data_4, int Data_5, int Data_6, int Data_7)
{
	Msg_Prot_Ping_Pong_Msg_t  ping_pong_msg;

	int res;

	ping_pong_msg.Msg_Hdr.Id = MSG_PROT_PING_PONG_MSG;

	ping_pong_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	ping_pong_msg.Msg_Hdr.Size = sizeof(ping_pong_msg.Msg_Hdr) + sizeof(ping_pong_msg.Info);

	ping_pong_msg.Info.Ping_Initiator = Ping_Initiator;
	ping_pong_msg.Info.Initiator_Msg_ID = Initiator_Msg_Id;
	ping_pong_msg.Info.Data_1 = Data_1;
	ping_pong_msg.Info.Data_2 = Data_2;
	ping_pong_msg.Info.Data_3 = Data_3;
	ping_pong_msg.Info.Data_4 = Data_4;
	ping_pong_msg.Info.Data_5 = Data_5;
	ping_pong_msg.Info.Data_6 = Data_6;
	ping_pong_msg.Info.Data_7 = Data_7;
#ifdef ONS_RTC
	ping_pong_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#else // ONS_HOST or RTC_SIM
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	ping_pong_msg.Msg_Hdr.Time_Stamp = Tx_Time;
	ping_pong_msg.Info.Time_tag = Tx_Time;
	//char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	//snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_PING_PONG_MSG");
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 42, __FILENAME__, __LINE__, __func__, (char*)"Send MSG_PROT_PING_PONG_MSG");
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
	//Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_PING_PONG_MSG, ping_pong_msg.Info.Time_tag, ping_pong_msg.Info.Ping_Initiator, ping_pong_msg.Info.Initiator_Msg_ID, ping_pong_msg.Info.Data_1, ping_pong_msg.Info.Data_4, ping_pong_msg.Info.Data_7);

#endif // ONS_HOST

	res = USBComm_Send_Message((USBComm_Buffer_t)&ping_pong_msg, ping_pong_msg.Msg_Hdr.Size);

	return res;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Ping_Pong_Msg
//
//  Purpose				: FOR TESTING : Unpack Ping Pong message
//
//  Inputs				: 
//
//  Outputs				: Time
//						  Ping_Initiator - the Initator of the Ping
//						  Initiator_Msg_Id - The Initiator Msg ID
//						  Data_1 -> Data_5 - data parames
//
//  Returns				: Success or Error code
//
//  Description			:pack & send ping/pong message to/from HOST 
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Unpack_Ping_Pong_Msg(ONS_BYTE* Msg_Buff, int* Time, Ping_Initiator_t* Ping_Initiator, int* Initiator_Msg_Id, int* Data_1, int* Data_2, int* Data_3, int* Data_4, int* Data_5, int* Data_6, int* Data_7)
{
	Msg_Prot_Ping_Pong_t* msg_p;

	msg_p = (Msg_Prot_Ping_Pong_t*)Msg_Buff;

	*Time = msg_p->Time_tag;
	*Ping_Initiator = (Ping_Initiator_t)msg_p->Ping_Initiator;
	*Initiator_Msg_Id = msg_p->Initiator_Msg_ID;
	*Data_1 = msg_p->Data_1;
	*Data_2 = msg_p->Data_2;
	*Data_3 = msg_p->Data_3;
	*Data_4 = msg_p->Data_4;
	*Data_5 = msg_p->Data_5;
	*Data_6 = msg_p->Data_6;
	*Data_7 = msg_p->Data_7;

#ifdef ONS_HOST
	//char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	//snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Receive MSG_PROT_PING_PONG_MSG");
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 43, __FILENAME__, __LINE__, __func__, (char*)"Receive MSG_PROT_PING_PONG_MSG");
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
	//Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_PING_PONG_MSG, *Time, *Ping_Initiator, *Initiator_Msg_Id, *Data_1, *Data_4, *Data_7);

#endif
	return MSG_PROT_SUCCESS;
}


// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Machine_Status_Request
//
//  Purpose				: pack request for Machine status and send it to RTC
//
//  Inputs				: Reply_Mode - request the machine status upon request/change/time 
//						  Time_Period - the time between status update (relevant only if time mode selected)
//
//  Outputs				:
//
//  Returns				: Success or Error code
//
//  Description			: the reuest determine the RTC status reply mode. 
//						  in case the mode is time the periode determine according to Time_Period value
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Pack_n_Tx_Machine_Status_Request(Status_Reply_Mode_t Reply_Mode, int Time_Period, unsigned int ID, int Act_1, int Act_2, int Act_3, int Act_4, int Act_5, int IO_1, int IO_2, int IO_3)
{
	Msg_Prot_Machine_Status_Request_Msg_t  Status_data_msg;

	int res;

	Status_data_msg.Msg_Hdr.Id = MSG_PROT_MACHINE_STATUS_REQUEST;

	Status_data_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	Status_data_msg.Info.Mode= Reply_Mode;

	Status_data_msg.Info.Time_Period = Time_Period;

	Status_data_msg.Info.Status_Msg_ID = ID;

	Status_data_msg.Info.Act_1_Mask = Act_1;

	Status_data_msg.Info.Act_2_Mask = Act_2;

	Status_data_msg.Info.Act_3_Mask = Act_3;

	Status_data_msg.Info.Act_4_Mask = Act_4;

	Status_data_msg.Info.Act_5_Mask = Act_5;

	Status_data_msg.Info.IO_1_Mask = IO_1;

	Status_data_msg.Info.IO_2_Mask = IO_2;

	Status_data_msg.Info.IO_3_Mask = IO_3;


	Status_data_msg.Msg_Hdr.Size = sizeof(Status_data_msg.Msg_Hdr) + sizeof(Status_data_msg.Info);

	#ifdef ONS_HOST
	unsigned long Tx_Time;
	#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
	#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
	#endif // ONS_WIN 
	Status_data_msg.Msg_Hdr.Time_Stamp = Tx_Time;
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_MACHINE_STATUS_REQUEST : Reply_Mode = %d,Time_Period=%d, ID=%d, Act_1=%d, Act_2=%d, Act_3=%d, Act_4=%d, IO_1=%d, IO_2=%d, IO_3=%d, IO_4=%d ",Reply_Mode, Time_Period, ID, Act_1, Act_2, Act_3, Act_4, Act_5, IO_1, IO_2, IO_3);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 44, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
//	Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_MACHINE_STATUS_REQUEST, Status_data_msg.Info.Mode, Status_data_msg.Info.Time_Period, 0, 0, 0, 0);

	#endif // ONS_HOST

#ifdef ONS_RTC
    Status_data_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&Status_data_msg, Status_data_msg.Msg_Hdr.Size);

	return res;
}


// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Machine_Status_Request
//
//  Purpose				:  Unpack Machine Status Request
//
//  Inputs				: Msg_Buff
//
//  Outputs				: Reply_Mode - the required mode of RTC status reply
//						  Time_Period
//						  Act and IO Mask
//
//  Returns				: Success or Error code
//
//  Description			:unpack machine status request info 
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Unpack_Machine_Status_Request(ONS_BYTE* Msg_Buff, Status_Reply_Mode_t* Reply_Mode, int* Time_Period, unsigned int* ID, int* Act_1, int* Act_2, int* Act_3, int* Act_4, int* Act_5, int* IO_1, int* IO_2, int* IO_3)
{
	Msg_Prot_Machine_Status_Request_t* msg_p;

	msg_p = (Msg_Prot_Machine_Status_Request_t*)Msg_Buff;

	*Reply_Mode = (Status_Reply_Mode_t)msg_p->Mode;
	*Time_Period =  msg_p->Time_Period;
	*ID = msg_p->Status_Msg_ID;
	*Act_1 = msg_p->Act_1_Mask;
	*Act_2 = msg_p->Act_2_Mask;
	*Act_3 = msg_p->Act_3_Mask;
	*Act_4 = msg_p->Act_4_Mask;
	*Act_5 = msg_p->Act_5_Mask;
	*IO_1  = msg_p->IO_1_Mask;
	*IO_2  = msg_p->IO_2_Mask;
	*IO_3  = msg_p->IO_3_Mask;

#ifdef ONS_HOST
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Receive MSG_PROT_MACHINE_STATUS_REQUEST , Mode=%d,Time_Period=%d,Status_Msg_ID=%d,Act_1_Mask=%d,Act_2_Mask=%d,Act_3_Mask=%d, Act_4_Mask=%d, Act_5_Mask=%d,IO_1_Mask=%d,IO_2_Mask=%d,IO_3_Mask=%d", msg_p->Mode, msg_p->Time_Period, msg_p->Status_Msg_ID, msg_p->Act_1_Mask, msg_p->Act_2_Mask, msg_p->Act_3_Mask, msg_p->Act_4_Mask, msg_p->Act_5_Mask, msg_p->IO_1_Mask, msg_p->IO_2_Mask, msg_p->IO_3_Mask);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 45, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
	//Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_MACHINE_STATUS_REQUEST, *Reply_Mode, *Time_Period, 0, 0, 0, 0);

#endif
	return MSG_PROT_SUCCESS;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Machine_Status_Reply
//
//  Purpose				: pack RTC status replay and send it to HOST
//
//  Inputs				: SH_Lock - sewhead lock 
//						  XY_Lock - xy lock
//						  Needel_Down
//
//
//
//  Outputs				:
//
//  Returns				: Success or Error code
//
//  Description			: request RTC to perform INIT operation
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Pack_n_Tx_Machine_Status_Reply(int M_S_Msg_ID, int Act_1, int Act_2, int Act_3, int Act_4, int Act_5, int IO_1, int IO_2, int IO_3, int Servo_Ind)
{
	Msg_Prot_Machine_Status_Reply_Msg_t  status_reply_msg;

	int res;

	status_reply_msg.Msg_Hdr.Id = MSG_PROT_MACHINE_STATUS_REPLY;

	status_reply_msg.Machine_Status.Status_Msg_ID = M_S_Msg_ID;

	status_reply_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	status_reply_msg.Machine_Status.Act_1_Data = Act_1;

	status_reply_msg.Machine_Status.Act_2_Data = Act_2;

	status_reply_msg.Machine_Status.Act_3_Data = Act_3;

	status_reply_msg.Machine_Status.Act_4_Data = Act_4;

	status_reply_msg.Machine_Status.Act_5_Data = Act_5;

	status_reply_msg.Machine_Status.IO_1_Data = IO_1;

	status_reply_msg.Machine_Status.IO_2_Data = IO_2;

	status_reply_msg.Machine_Status.IO_3_Data = IO_3;


	status_reply_msg.Machine_Status.Servo_Ind_Data = Servo_Ind;

	status_reply_msg.Msg_Hdr.Size = sizeof(status_reply_msg.Msg_Hdr) + sizeof(status_reply_msg.Machine_Status);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	status_reply_msg.Msg_Hdr.Time_Stamp = Tx_Time;

	//char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	//snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_MACHINE_STATUS_REPLY");
	// : Status_Msg_ID=%d, Servo_Ind_Data=%d, Act_1_Data=%d, Act_2_Data=%d, Act_3_Data=%d, IO_1_Data=%d,IO_2_Data=%d,IO_3_Data=%d,IO_4_Data=%d", 
	//	status_reply_msg.Machine_Status.Status_Msg_ID, status_reply_msg.Machine_Status.Servo_Ind_Data, status_reply_msg.Machine_Status.Act_1_Data, status_reply_msg.Machine_Status.Act_2_Data, status_reply_msg.Machine_Status.Act_3_Data, status_reply_msg.Machine_Status.IO_1_Data, status_reply_msg.Machine_Status.IO_2_Data, status_reply_msg.Machine_Status.IO_3_Data, status_reply_msg.Machine_Status.IO_4_Data);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 46, __FILENAME__, __LINE__, __func__, (char*)"Send MSG_PROT_MACHINE_STATUS_REPLY");
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
//	Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_MACHINE_STATUS_REPLY, status_reply_msg.Machine_Status.Status_Msg_ID, status_reply_msg.Machine_Status.Servo_Ind_Data, status_reply_msg.Machine_Status.Act_1_Data, status_reply_msg.Machine_Status.Act_2_Data, status_reply_msg.Machine_Status.IO_1_Data, status_reply_msg.Machine_Status.IO_2_Data);

#endif // ONS_HOST

#ifdef ONS_RTC
	status_reply_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&status_reply_msg, status_reply_msg.Msg_Hdr.Size);

	return res;
}


// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Machine_Status_Reply
//
//  Purpose				:  Unpack Machine Status Reply
//
//  Inputs				: Msg_Buff
//
//  Outputs				: SH_Lock 
//						  XY_Lock
//						  Needel_Down
//						  APF_Up_Pos
//
//  Returns				: Success or Error code
//
//  Description			:unpack machine status request info 
//
// ---------------------------------------------------------------------------
int	 Msg_Prot_Unpack_Machine_Status_Reply(ONS_BYTE* Msg_Buff, int* M_S_Msg_ID, int* Act_1, int* Act_2, int* Act_3, int* Act_4, int* Act_5, int* IO_1, int* IO_2, int* IO_3, int* Servo_Ind)
{
	Msg_Prot_Machine_Status_Reply_t* msg_p;

	msg_p = (Msg_Prot_Machine_Status_Reply_t*)Msg_Buff;

	*Act_1		= msg_p->Act_1_Data;
	*Act_2		= msg_p->Act_2_Data;
	*Act_3		= msg_p->Act_3_Data;
	*Act_4		= msg_p->Act_4_Data;
	*Act_5		= msg_p->Act_5_Data;
	*IO_1		= msg_p->IO_1_Data;
	*IO_2		= msg_p->IO_2_Data;
	*IO_3		= msg_p->IO_3_Data;
	*Servo_Ind	= msg_p->Servo_Ind_Data;
	*M_S_Msg_ID = msg_p->Status_Msg_ID;

#ifdef ONS_HOST_LINUX

	//char			msg_txt[MAX_SYSLOG_DESC_SIZE];
	//snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "MSG_PROT_MACHINE_STATUS_REPLY");// : Status_Msg_ID=%d,Servo_Ind_Data=%d, Act_1_Data=0x%x, Act_2_Data=0x%x, Act_3_Data=0x%x, Act_4_Data=0x%x, , Act_5_Data=0x%x, IO_1_Data=0x%x,IO_2_Data=0x%x,IO_3_Data=0x%x", *M_S_Msg_ID, *Servo_Ind, *Act_1, *Act_2, *Act_3, *Act_4, *Act_5, *IO_1, *IO_2, *IO_3);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 47, __FILENAME__, __LINE__, __func__, (char*)"MSG_PROT_MACHINE_STATUS_REPLY");
	//snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "MSG_PROT_MACHINE_STATUS_REPLY : Status_Msg_ID=%d,Servo_Ind_Data=%d, Act_1_Data=0x%x, Act_2_Data=0x%x, Act_3_Data=0x%x, Act_4_Data=0x%x, , Act_5_Data=0x%x, IO_1_Data=0x%x,IO_2_Data=0x%x,IO_3_Data=0x%x", *M_S_Msg_ID, *Servo_Ind, *Act_1, *Act_2, *Act_3, *Act_4, *Act_5, *IO_1, *IO_2, *IO_3);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
#endif

	return MSG_PROT_SUCCESS;
}


//Machine Info
// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Machine_Info_Request
//
//  Purpose				: pack request for Machine Info and send it to RTC
//
//  Inputs				: Reply_Mode - request the machine Info upon request/change/time 
//						  Time_Period - the time between Info update (relevant only if time mode selected)
//
//  Outputs				:
//
//  Returns				: Success or Error code
//
//  Description			: the reuest determine the RTC status reply mode. 
//						  in case the mode is time the periode determine according to Time_Period value
//
// ---------------------------------------------------------------------------
int	 Msg_Prot_Pack_n_Tx_Machine_Info_Request(Status_Reply_Mode_t Reply_Mode, int Time_Period, unsigned int ID)
{
	Msg_Prot_Machine_Info_Request_Msg_t  Info_data_msg;

	int res;

	Info_data_msg.Msg_Hdr.Id = MSG_PROT_MACHINE_INFO_REQUEST;

	Info_data_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	Info_data_msg.Info.Mode = Reply_Mode;

	Info_data_msg.Info.Time_Period = Time_Period;

	Info_data_msg.Info.Status_Msg_ID = ID;

	Info_data_msg.Msg_Hdr.Size = sizeof(Info_data_msg.Msg_Hdr) + sizeof(Info_data_msg.Info);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	Info_data_msg.Msg_Hdr.Time_Stamp = Tx_Time;
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_MACHINE_INFO_REQUEST : Reply_Mode = %d,Time_Period=%u, ID=%d", Reply_Mode, Time_Period,ID);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 13, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);

#endif // ONS_HOST

#ifdef ONS_RTC
	Info_data_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&Info_data_msg, Info_data_msg.Msg_Hdr.Size);

	return res;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Machine_Info_Request
//
//  Purpose				:  Unpack Machine Info Reply
//
//  Inputs				: Msg_Buff
//
//  Outputs				: Bobbin_Couter_Mask 
//						  Winder_Couter_Mask
//
//  Returns				: Success or Error code
//
//  Description			:unpack machine info request info 
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Unpack_Machine_Info_Request(ONS_BYTE* Msg_Buff, Status_Reply_Mode_t* Reply_Mode, int* Time_Period, unsigned int* ID)
{
	Msg_Prot_Machine_Info_Request_t* msg_p;

	msg_p = (Msg_Prot_Machine_Info_Request_t*)Msg_Buff;

	*ID = msg_p->Status_Msg_ID;
	*Reply_Mode = (Status_Reply_Mode_t)msg_p->Mode;
	*Time_Period = msg_p->Time_Period;

#ifdef ONS_HOST

	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	*Time_Period = msg_p->Time_Period;
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "MSG_PROT_MACHINE_STATUS_REQEST : Status_Msg_ID=%d,Reply_Mode =%d,  Time_Period =%d", *ID, *Reply_Mode,*Time_Period);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 48, __FILENAME__, __LINE__, __func__, msg_txt);
//	ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);

#endif
	return MSG_PROT_SUCCESS;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Machine_Info_Reply
//
//  Purpose				: pack RTC info replay and send it to HOST
//
//  Inputs				: 
//
//
//
//  Outputs				:
//
//  Returns				: Success or Error code
//
//  Description			: 
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Pack_n_Tx_Machine_Info_Reply(int M_S_Msg_ID, int Spool_Counters_Status, unsigned int Bobbin_Counter_Value, unsigned int Winder_Counter_Value,  unsigned int Thread_Counter_Value, int Tension_Value, int Bobbin_Changer_Status, int Bobbin_Changer_Error_Code, int Bobbin_Changer_Step, int Z_Servo_Pos)
{
	Msg_Prot_Machine_Info_Reply_Msg_t  info_reply_msg;

	int res;

	info_reply_msg.Msg_Hdr.Id = MSG_PROT_MACHINE_INFO_REPLY;

	info_reply_msg.Machine_Info.Status_Msg_ID = M_S_Msg_ID;

	info_reply_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	info_reply_msg.Machine_Info.Spool_Counters_Status = Spool_Counters_Status;

	info_reply_msg.Machine_Info.Bobbin_Counter_Value = Bobbin_Counter_Value;

	info_reply_msg.Machine_Info.Winder_Counter_Value = Winder_Counter_Value;

	info_reply_msg.Machine_Info.Tension_Value = Tension_Value;

	info_reply_msg.Machine_Info.Bobbin_Changer_Status = Bobbin_Changer_Status;

	info_reply_msg.Machine_Info.Bobbin_Changer_Step = Bobbin_Changer_Step;

	info_reply_msg.Machine_Info.Bobbin_Changer_Error_Code = Bobbin_Changer_Error_Code;
	info_reply_msg.Machine_Info.Z_Servo_Pos = Z_Servo_Pos;

	info_reply_msg.Msg_Hdr.Size = sizeof(info_reply_msg.Msg_Hdr) + sizeof(info_reply_msg.Machine_Info);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	info_reply_msg.Msg_Hdr.Time_Stamp = Tx_Time;

	//char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	//snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_MACHINE_INFO_REPLY");
	// : Status_Msg_ID=%d, Servo_Ind_Data=%d, Act_1_Data=%d, Act_2_Data=%d, Act_3_Data=%d, IO_1_Data=%d,IO_2_Data=%d,IO_3_Data=%d,IO_4_Data=%d", 
	//	status_reply_msg.Machine_Status.Status_Msg_ID, status_reply_msg.Machine_Status.Servo_Ind_Data, status_reply_msg.Machine_Status.Act_1_Data, status_reply_msg.Machine_Status.Act_2_Data, status_reply_msg.Machine_Status.Act_3_Data, status_reply_msg.Machine_Status.IO_1_Data, status_reply_msg.Machine_Status.IO_2_Data, status_reply_msg.Machine_Status.IO_3_Data, status_reply_msg.Machine_Status.IO_4_Data);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 49, __FILENAME__, __LINE__, __func__, (char*)"Send MSG_PROT_MACHINE_INFO_REPLY");
//	ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
	//	Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_MACHINE_STATUS_REPLY, status_reply_msg.Machine_Status.Status_Msg_ID, status_reply_msg.Machine_Status.Servo_Ind_Data, status_reply_msg.Machine_Status.Act_1_Data, status_reply_msg.Machine_Status.Act_2_Data, status_reply_msg.Machine_Status.IO_1_Data, status_reply_msg.Machine_Status.IO_2_Data);

#endif // ONS_HOST

#ifdef ONS_RTC
	info_reply_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&info_reply_msg, info_reply_msg.Msg_Hdr.Size);

	return res;
}


// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Machine_Info_Reply
//
//  Purpose				:  Unpack Machine Status Reply
//
//  Inputs				: Msg_Buff
//
//  Outputs				: SH_Lock 
//						  XY_Lock
//						  Needel_Down
//						  APF_Up_Pos
//
//  Returns				: Success or Error code
//
//  Description			:unpack machine status request info 
//
// ---------------------------------------------------------------------------
int Msg_Prot_Unpack_Machine_Info_Reply(ONS_BYTE* Msg_Buff, int* M_S_Msg_ID, int* Spool_Counters_Status, int* Bobbin_Counter_Value, int* Winder_Counter_Value, int* Thread_Counter_Value, int* Tension_Value, int* Bobbin_Changer_Status, int* Bobbin_Changer_Error_Code, int* Bobbin_Changer_Step, int* Z_Servo_Pos)
{
	Msg_Prot_Machine_Info_Reply_t* msg_p;

	msg_p = (Msg_Prot_Machine_Info_Reply_t*)Msg_Buff;

	*Spool_Counters_Status = msg_p->Spool_Counters_Status;
	*Bobbin_Counter_Value = msg_p->Bobbin_Counter_Value;
	*Winder_Counter_Value = msg_p->Winder_Counter_Value;
	*Thread_Counter_Value = msg_p->Thread_Counter_Value;
	*Tension_Value		  = msg_p->Tension_Value;
	*M_S_Msg_ID = msg_p->Status_Msg_ID;
	*Bobbin_Changer_Status = msg_p->Bobbin_Changer_Status;
	*Bobbin_Changer_Error_Code = msg_p->Bobbin_Changer_Error_Code;
	*Bobbin_Changer_Step = msg_p->Bobbin_Changer_Step;
	*Z_Servo_Pos = msg_p->Z_Servo_Pos;
#ifdef ONS_HOST

	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
//	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "MSG_PROT_MACHINE_INFO_REPLY");// : Status_Msg_ID=%d,Spool_Couters_Status  = 0x%x Bobbin_Couter_Value=%d, Winder_Couter_Value=%d, Thread_Couter_Value=%d Tension_Value = %d, Bobbin_Changer_Status = 0x%x, Bobbin_Changer_Error_Code = %d, *Bobbin_Changer_Step = %d, Z_Servo_Pos=%d", *M_S_Msg_ID, *Spool_Counters_Status, *Bobbin_Counter_Value, *Winder_Counter_Value, *Thread_Counter_Value, *Tension_Value, *Bobbin_Changer_Status, *Bobbin_Changer_Error_Code, *Bobbin_Changer_Step, *Z_Servo_Pos);
//	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "MSG_PROT_MACHINE_INFO_REPLY");// : Status_Msg_ID=%d,Spool_Couters_Status  = 0x%x Bobbin_Couter_Value=%d, Winder_Couter_Value=%d, Thread_Couter_Value=%d Tension_Value = %d, Bobbin_Changer_Status = 0x%x, Bobbin_Changer_Error_Code = %d, *Bobbin_Changer_Step = %d, Z_Servo_Pos=%d", *M_S_Msg_ID, *Spool_Counters_Status, *Bobbin_Counter_Value, *Winder_Counter_Value, *Thread_Counter_Value, *Tension_Value, *Bobbin_Changer_Status, *Bobbin_Changer_Error_Code, *Bobbin_Changer_Step, *Z_Servo_Pos);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 50, __FILENAME__, __LINE__, __func__, (char*)"MSG_PROT_MACHINE_INFO_REPLY");
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "MSG_PROT_MACHINE_INFO_REPLY : Status_Msg_ID=%d,Spool_Cout_Status  = 0x%x Cout_Value : Bobbin=%d, Winder=%d, Thread=%d. Tension_Value = %d, Bobbin_Changer: Status = 0x%x, Error_Code = %d, *Step = %d. Z_Servo_Pos=%d", *M_S_Msg_ID, *Spool_Counters_Status, *Bobbin_Counter_Value, *Winder_Counter_Value, *Thread_Counter_Value, *Tension_Value, *Bobbin_Changer_Status, *Bobbin_Changer_Error_Code, *Bobbin_Changer_Step, *Z_Servo_Pos);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_DEBUG, __FILENAME__, __LINE__, __func__, msg_txt);

#endif
	return MSG_PROT_SUCCESS;
}


//TTS Status
// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_TTS_Status_Request
//
//  Purpose				: pack request for TTS status and send it to RTC
//
//  Inputs				: Reply_Mode - request the machine status upon request/change/time 
//						  Time_Period - the time between status update (relevant only if time mode selected)
//
//  Outputs				:
//
//  Returns				: Success or Error code
//
//  Description			: the reest determine the RTC status reply mode. 
//						  in case the mode is time the period determine according to Time_Period value
//
// ---------------------------------------------------------------------------

/*
int	Msg_Prot_Pack_n_Tx_TTS_Status_Request(Status_Reply_Mode_t Reply_Mode, int Time_Period, unsigned int ID )
{
	Msg_Prot_TTS_Status_Request_Msg_t  Status_data_msg;

	int res;

	Status_data_msg.Msg_Hdr.Id = MSG_PROT_TTS_STATUS_REQUEST;

	Status_data_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	Status_data_msg.Info.Mode= Reply_Mode;

	Status_data_msg.Info.Time_Period = Time_Period;

	Status_data_msg.Info.Status_Msg_ID = ID;

	Status_data_msg.Msg_Hdr.Size = sizeof(Status_data_msg.Msg_Hdr) + sizeof(Status_data_msg.Info);

	#ifdef ONS_HOST
	unsigned long Tx_Time;
	#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
	#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
	#endif // ONS_WIN 
	Status_data_msg.Msg_Hdr.Time_Stamp = Tx_Time;
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_TTS_STATUS_REQUEST : Reply_Mode = %d,Time_Period=%d, ID=%d",Reply_Mode, Time_Period, ID);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, __LINE__, __FILENAME__, __LINE__, __func__, msg_txt);

	#endif // ONS_HOST

#ifdef ONS_RTC
    Status_data_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&Status_data_msg, Status_data_msg.Msg_Hdr.Size);

	return res;
}
*/

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_TTS_Status_Request
//
//  Purpose				:  Unpack TTS Status Request
//
//  Inputs				: Msg_Buff
//
//  Outputs				: Reply_Mode - the required mode of RTC status reply
//						  Time_Period
//						  Act and IO Mask
//
//  Returns				: Success or Error code
//
//  Description			:unpack machine status request info 
//
// ---------------------------------------------------------------------------
/*
int	Msg_Prot_Unpack_TTS_Status_Request(ONS_BYTE* Msg_Buff, Status_Reply_Mode_t* Reply_Mode, int* Time_Period, unsigned int* ID)
{
	Msg_Prot_TTS_Status_Request_t* msg_p;

	msg_p = (Msg_Prot_TTS_Status_Request_t*)Msg_Buff;

	*Reply_Mode = (Status_Reply_Mode_t)msg_p->Mode;
	*Time_Period =  msg_p->Time_Period;
	*ID = msg_p->Status_Msg_ID;
	
#ifdef ONS_HOST
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Receive MSG_PROT_TTS_STATUS_REQUEST , Mode=%d,Time_Period=%d,Status_Msg_ID=%d", msg_p->Mode, msg_p->Time_Period, msg_p->Status_Msg_ID);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, __LINE__, __FILENAME__, __LINE__, __func__, msg_txt);

#endif
	return MSG_PROT_SUCCESS;
}
*/

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_TTS_Status_Reply
//
//  Purpose				: pack RTC status replay and send it to HOST
//
//  Inputs				: 
//
//  Outputs				:
//
//  Returns				: Success or Error code
//
//  Description			: request RTC to perform INIT operation
//
// ---------------------------------------------------------------------------

/*
int	Msg_Prot_Pack_n_Tx_TTS_Status_Reply(int M_S_Msg_ID, int Act_1, int Act_2 , int IO_1)
{
	Msg_Prot_TTS_Status_Reply_Msg_t  status_reply_msg;

	int res;

	status_reply_msg.Msg_Hdr.Id = MSG_PROT_TTS_STATUS_REPLY;

	status_reply_msg.TTS_Status.Status_Msg_ID = M_S_Msg_ID;

	status_reply_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	status_reply_msg.TTS_Status.Act_1_Data = Act_1;
    
    status_reply_msg.TTS_Status.Act_2_Data = Act_2;

	status_reply_msg.TTS_Status.IO_1_Data = IO_1;

	status_reply_msg.Msg_Hdr.Size = sizeof(status_reply_msg.Msg_Hdr) + sizeof(status_reply_msg.TTS_Status);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	status_reply_msg.Msg_Hdr.Time_Stamp = Tx_Time;

	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_TTS_STATUS_REPLY");
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, __LINE__, __FILENAME__, __LINE__, __func__, msg_txt);

#endif // ONS_HOST

#ifdef ONS_RTC
	status_reply_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&status_reply_msg, status_reply_msg.Msg_Hdr.Size);

	return res;
}
*/

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_TTS_Status_Reply
//
//  Purpose				:  Unpack Machine Status Reply
//
//  Inputs				: Msg_Buff
//
//  Outputs				: SH_Lock 
//						  XY_Lock
//						  Needel_Down
//						  APF_Up_Pos
//
//  Returns				: Success or Error code
//
//  Description			:unpack machine status request info 
//
// ---------------------------------------------------------------------------

/*
int	 Msg_Prot_Unpack_TTS_Status_Reply(ONS_BYTE* Msg_Buff, int* M_S_Msg_ID, int* Act_1, int* Act_2, int* IO_1)
{
	Msg_Prot_TTS_Status_Reply_t* msg_p;

	msg_p = (Msg_Prot_TTS_Status_Reply_t*)Msg_Buff;

	*Act_1		= msg_p->Act_1_Data;
    *Act_2		= msg_p->Act_2_Data;
	*IO_1		= msg_p->IO_1_Data;
	*M_S_Msg_ID = msg_p->Status_Msg_ID;

#ifdef ONS_HOST

	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Receive MSG_PROT_TTS_STATUS_REPLY : Status_Msg_ID=%d, Act_1_Data=0x%x,Act_2_Data=0x%x,  IO_1_Data=0x%x", *M_S_Msg_ID,*Act_1, *Act_2 ,*IO_1);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, __LINE__, __FILENAME__, __LINE__, __func__, msg_txt);

#endif
	return MSG_PROT_SUCCESS;
}

*/

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Profile_Info
//
//  Purpose				: pack and send Profile Info 
//
//  Inputs				: Profile_Number - the relevant profile number
//						  Info	- profile data					
//
//  Outputs				: 
//
//  Returns				: Success or Error code
//
//  Description			: pack and send the current Profile Info that the RTC use
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Pack_n_Tx_Profile_Info(int Profile_Number, int* Info)
{
	Msg_Prot_Profile_Info_Msg_t  profile_info_msg;

	int res;

	profile_info_msg.Msg_Hdr.Id = MSG_PROT_PROFILE_INFO;

	profile_info_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	memcpy(profile_info_msg.Profile_info.Info,Info,sizeof(profile_info_msg.Profile_info.Info));

	profile_info_msg.Profile_info.Profile_Num = Profile_Number;

	profile_info_msg.Msg_Hdr.Size = sizeof(profile_info_msg.Msg_Hdr) + sizeof(profile_info_msg.Profile_info);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	profile_info_msg.Msg_Hdr.Time_Stamp = Tx_Time;

	//char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	//snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_PROFILE_INFO");
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 51, __FILENAME__, __LINE__, __func__, (char*)"Send MSG_PROT_PROFILE_INFO");
//	ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
	//Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_PROFILE_INFO, profile_info_msg.Profile_info.Profile_Num, profile_info_msg.Profile_info.Info[0], profile_info_msg.Profile_info.Info[1], profile_info_msg.Profile_info.Info[2], profile_info_msg.Profile_info.Info[3], profile_info_msg.Profile_info.Info[3]);

#endif // ONS_HOST

#ifdef ONS_RTC
	profile_info_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&profile_info_msg, profile_info_msg.Msg_Hdr.Size);

	return res;
}


// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Profile_Info
//
//  Purpose				:  Unpack Profile Info
//
//  Inputs				: Msg_Buff
//
//  Outputs				: Profile_Number - the relevant profile number
//						  Info	- profile data
//
//  Returns				: Success or Error code
//
//  Description			:unpack Rtc profile Data 
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Unpack_Profile_Info(ONS_BYTE* Msg_Buff, int* Profile_Num, INT_16* Info)
{
	Msg_Prot_Profile_Info_t* msg_p;

	msg_p = (Msg_Prot_Profile_Info_t*)Msg_Buff;

	*Profile_Num = msg_p->Profile_Num;
	memcpy (Info, msg_p->Info, (sizeof(INT_16)*MSG_PROT_PROFILE_MAX_DATA_SIZE));//sizeof(msg_p->Info));

	
#ifdef ONS_HOST
//	Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_PROFILE_INFO, *Profile_Num, msg_p->Info[0], msg_p->Info[1], msg_p->Info[2], msg_p->Info[3], msg_p->Info[4]);
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Receive MSG_PROT_PROFILE_INFO : Profile_Num = %d", *Profile_Num);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 52, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
	//Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_PROFILE_INFO, *Profile_Num, Info[0], Info[1], Info[2], Info[3], Info[4]);
#endif
	return MSG_PROT_SUCCESS;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Profile_Info_Req
//
//  Purpose				: pack & transmit  Profile Info request from RTC
//
//  Inputs				: 
//
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			: send to RTC Profile Info request in order to initiate sending of Profile Info to Host
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Pack_n_Tx_Profile_Info_Req(void)
{
	Msg_Prot_Profile_Info_Req_Msg_t  Profile_Req_msg;

	int res;


	Profile_Req_msg.Msg_Hdr.Id = MSG_PROT_PROFILE_INFO_REQ;

	Profile_Req_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	Profile_Req_msg.Msg_Hdr.Size = sizeof(Profile_Req_msg.Msg_Hdr);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	Profile_Req_msg.Msg_Hdr.Time_Stamp = Tx_Time;
//	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
//	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_PROFILE_INFO_REQ");
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 53, __FILENAME__, __LINE__, __func__, (char*)"Send MSG_PROT_PROFILE_INFO_REQ");
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
	//Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_PROFILE_INFO_REQ, 0, 0, 0, 0, 0, 0);
#endif // ONS_HOST

#ifdef ONS_RTC
	Profile_Req_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&Profile_Req_msg, Profile_Req_msg.Msg_Hdr.Size);

	return res;
}

// APF

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_APF_Set_Position_Request
//
//  Purpose				: pack & transmit  APF position request
//
//  Inputs				: PF_Pos - Up - 1  or  Down - 0
//						  APF_Value - position value in steps 
//
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			: send set Presher Foot  position request to RTC 
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Pack_n_Tx_APF_Set_Position_Request(ONS_Up_Down_State_t PF_Pos, int APF_Value)
{
	int res;
	Msg_Prot_Set_APF_Position_Msg_t		APF_Set_Msg;


	APF_Set_Msg.Msg_Hdr.Id = MSG_PROT_APF_SET_POSITION_REQ;

	APF_Set_Msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	APF_Set_Msg.Msg_Hdr.Size = sizeof(APF_Set_Msg.Msg_Hdr)+ sizeof(APF_Set_Msg.APF);

	APF_Set_Msg.APF.Position = (INT_16)PF_Pos;
	APF_Set_Msg.APF.Value    = (INT_16)APF_Value;

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	APF_Set_Msg.Msg_Hdr.Time_Stamp = Tx_Time;
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_APF_SET_POSITION_REQ : PF Pos = %d, APF pos value = %d ", APF_Set_Msg.APF.Position, APF_Set_Msg.APF.Value);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 54, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt); 
#endif // ONS_HOST

#ifdef ONS_RTC
	APF_Set_Msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&APF_Set_Msg, APF_Set_Msg.Msg_Hdr.Size);

	return res;

}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_APF_Set_Position_Request
//
//  Purpose				:  Unpack APF required position 
//
//  Inputs				: Msg_Buff
//
//  Outputs				: PF_Pos	- Pf  position Up or Down
//						  APF_Value - APF position Value in steps
//
//  Returns				: Success or Error code
//
//  Description			:unpack Rtc profile Data 
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Unpack_APF_Set_Position_Request(ONS_BYTE* Msg_Buff, ONS_Up_Down_State_t* PF_Pos, int* APF_Value)
{
	Msg_Prot_Set_APF_Position_t* msg_p;

	msg_p = (Msg_Prot_Set_APF_Position_t*)Msg_Buff;

	*PF_Pos		= (ONS_Up_Down_State_t)msg_p->Position;
	*APF_Value	= msg_p->Value;

#ifdef ONS_HOST
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Receive MSG_PROT_APF_SET_POSITION_REQ :  PF Pos = %d, APF pos value = %d ", *PF_Pos,*APF_Value);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 55, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt); 
#endif
	return MSG_PROT_SUCCESS;

}


//Manual Cut

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Manual_Cut_Req
//
//  Purpose				: pack & transmit  Profile Info request from RTC
//
//  Inputs				: 
//
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			: send to RTC Profile Info request in order to initiate sending of Profile Info to Host
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Pack_n_Tx_Manual_Cut_Req(void)
{
	Msg_Prot_Manual_Cat_Msg_t  Manual_Cut_Req_msg;

	int res;


	Manual_Cut_Req_msg.Msg_Hdr.Id = MSG_PROT_MANUAL_CUT_REQ;

	Manual_Cut_Req_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	Manual_Cut_Req_msg.Msg_Hdr.Size = sizeof(Manual_Cut_Req_msg.Msg_Hdr);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	Manual_Cut_Req_msg.Msg_Hdr.Time_Stamp = Tx_Time;
	//char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	//snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_MANUAL_CUT_REQ");
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 56, __FILENAME__, __LINE__, __func__, (char*)"Send MSG_PROT_MANUAL_CUT_REQ");
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
//	Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_PROFILE_INFO_REQ, 0, 0, 0, 0, 0, 0);
#endif // ONS_HOST

#ifdef ONS_RTC
	Manual_Cut_Req_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&Manual_Cut_Req_msg, Manual_Cut_Req_msg.Msg_Hdr.Size);

	return res;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Quick_stop
//
//  Purpose				: pack & transmit  Quick stop to RTC
//
//  Inputs				: 
//
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			: send to RTC Quick Stop 
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Pack_n_Tx_Quick_stop(void)
{
	Msg_Prot_Quick_Stop_Msg_t  Quick_Stop_msg;

	int res;


	Quick_Stop_msg.Msg_Hdr.Id = MSG_PROT_QUICK_STOP_REQ;

	Quick_Stop_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	Quick_Stop_msg.Msg_Hdr.Size = sizeof(Quick_Stop_msg.Msg_Hdr);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	Quick_Stop_msg.Msg_Hdr.Time_Stamp = Tx_Time;
	//char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	//snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_QUICK_STOP_REQ");
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 57, __FILENAME__, __LINE__, __func__, (char*)"Send MSG_PROT_QUICK_STOP_REQ");
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
	//Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_PROFILE_INFO_REQ, 0, 0, 0, 0, 0, 0);
#endif // ONS_HOST

#ifdef ONS_RTC
	Quick_Stop_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&Quick_Stop_msg, Quick_Stop_msg.Msg_Hdr.Size);

	return res;
}



// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Alarm_Info
//
//  Purpose				: pack & transmit RTC Alarm Info to the HOST side
//
//  Inputs				: Alarm_State 	-  state according to define
//
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			: send to RTC the DR_Delay_Update notification with the mode relevant info so RTC can Update DR Delay  
//
// ---------------------------------------------------------------------------
int		Msg_Prot_Pack_n_Tx_Alarm_Info(int Alarm_State)
{
	Msg_Prot_Alarm_State_Msg_t  alarm_state_msg;

	int res;

	alarm_state_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	alarm_state_msg.Msg_Hdr.Id = MSG_PROT_RTC_ALARM_MSG;

	alarm_state_msg.Msg_Hdr.Size = sizeof(alarm_state_msg.Msg_Hdr) + sizeof(alarm_state_msg.Alarm_State);

	alarm_state_msg.Alarm_State = Alarm_State;

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 

	alarm_state_msg.Msg_Hdr.Time_Stamp = Tx_Time;

	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_RTC_ALARM_MSG : Alarm_State = 0x%X", alarm_state_msg.Alarm_State);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 58, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);

	//Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_RTC_ALARM_MSG, Mode, Delay, Direction, 0, 0, 0);
#endif // ONS_HOST

#ifdef ONS_RTC
	alarm_state_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif	

	res = USBComm_Send_Message((USBComm_Buffer_t)&alarm_state_msg, alarm_state_msg.Msg_Hdr.Size);

	return res;
}


// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Alarm_Info
//
//  Purpose				: unpack Alarm info from the receive from RTC
//
//  Inputs				: Msg_Buff - pointer to the received message (without the generic header) .
//
//  Outputs				: Alarm State (bit wise acording to pre define mask
//
//  Returns				: Success
//
//  Description			: get the Alarm Info from RTC
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Unpack_Alarm_Info(ONS_BYTE* Msg_Buff, int* Alarm_State)
{

	UINT_16* msg_p;

	msg_p = (UINT_16*)Msg_Buff;

	*Alarm_State = *msg_p;
#ifdef ONS_HOST
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Receive Msg_Prot_Unpack_Alarm_Info , Alarm_State = 0x%X", *Alarm_State);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 59, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
	//Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_MACHINE_STATUS_REQUEST, *Reply_Mode, *Time_Period, 0, 0, 0, 0);

#endif
	return MSG_PROT_SUCCESS;
}



// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Set_Tention_Req
//
//  Purpose				: pack & transmit RTC Alarm Info to the HOST side
//
//  Inputs				: Alarm_State 	-  state according to define
//
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			: send to RTC the DR_Delay_Update notification with the mode relevant info so RTC can Update DR Delay  
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Pack_n_Tx_Set_Tention_Req(int Value)
{
	Msg_Prot_Set_Tension_Msg_t  Tension_Msg;

	int res;

	Tension_Msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	Tension_Msg.Msg_Hdr.Id = MSG_PROT_SET_TENSION_REQ;

	Tension_Msg.Msg_Hdr.Size = sizeof(Tension_Msg.Msg_Hdr) + sizeof(Tension_Msg.Tension_Value);

	Tension_Msg.Tension_Value = Value;

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 

	Tension_Msg.Msg_Hdr.Time_Stamp = Tx_Time;

	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_SET_TENSION_REQ : Tension Value = %d", Tension_Msg.Tension_Value);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 62, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);

	//Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_RTC_ALARM_MSG, Mode, Delay, Direction, 0, 0, 0);
#endif // ONS_HOST

#ifdef ONS_RTC
	Tension_Msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif	

	res = USBComm_Send_Message((USBComm_Buffer_t)&Tension_Msg, Tension_Msg.Msg_Hdr.Size);

	return res;
}


// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Set_Tention_Req
//
//  Purpose				: unpack Tension value from the HOST
//
//  Inputs				: Msg_Buff - pointer to the received message (without the generic header) .
//
//  Outputs				: 
//
//  Returns				: Success
//
//  Description			: 
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Unpack_Set_Tention_Req(ONS_BYTE* Msg_Buff, int* Value)
{

	UINT_16* msg_p;

	msg_p = (UINT_16*)Msg_Buff;

	*Value = *msg_p;
#ifdef ONS_HOST
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Receive Msg_Prot_Unpack_Set_Tention , Alarm_State = %d", *Value);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 61, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
	//Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_MACHINE_STATUS_REQUEST, *Reply_Mode, *Time_Period, 0, 0, 0, 0);

#endif
	return MSG_PROT_SUCCESS;
}




// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Entity_Get_State
//
//  Purpose				: pack & transmit Get Entity State according to ID from RTC
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				:
//
//  Description			: send to RTC request to get Entity state according to Entity ID
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Pack_n_Tx_Entity_Get_State(int Entity_Id)
{
	Msg_Prot_Entity_State_Req_Msg_t  entity_info_req_msg;

	int res;

	entity_info_req_msg.Msg_Hdr.Id = MSG_PROT_ENTITY_GET_STATE;

	entity_info_req_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	entity_info_req_msg.Msg_Hdr.Size = sizeof(entity_info_req_msg.Msg_Hdr) + sizeof(entity_info_req_msg.Entity_Id);

	entity_info_req_msg.Entity_Id = Entity_Id;

	#ifdef ONS_HOST
	unsigned long Tx_Time;
	#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
	#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
	#endif // ONS_WIN
	entity_info_req_msg.Msg_Hdr.Time_Stamp = Tx_Time;
	//char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	//snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_ENTITY_GET_STATE");
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 60, __FILENAME__, __LINE__, __func__, (char*)"Send MSG_PROT_ENTITY_GET_STATE");
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
	#endif // ONS_HOST

	#ifdef ONS_RTC
	entity_info_req_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
	#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&entity_info_req_msg, entity_info_req_msg.Msg_Hdr.Size);

	return res;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Reset_Alarm_Req
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
int	 Msg_Prot_Pack_n_Tx_Reset_Alarm_Req(int Alarm_Mask)
{
	
	Msg_Prot_Reset_Alarm_Msg_t  Reset_Alarm_msg;
	int res;


	Reset_Alarm_msg.Msg_Hdr.Id = MSG_PROT_RESET_ALARM;

	Reset_Alarm_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	Reset_Alarm_msg.Msg_Hdr.Size = sizeof(Msg_Prot_Reset_Alarm_Msg_t);
	Reset_Alarm_msg.Alarm_Mask = Alarm_Mask;
#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	Reset_Alarm_msg.Msg_Hdr.Time_Stamp = Tx_Time;
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_RESET_ALARM Alarm_Mask = %d", Alarm_Mask);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 63, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
	//Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_PROFILE_INFO_REQ, 0, 0, 0, 0, 0, 0);
#endif // ONS_HOST

#ifdef ONS_RTC
	Reset_Alarm_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&Reset_Alarm_msg, Reset_Alarm_msg.Msg_Hdr.Size);

	return res;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Reset_Alarm_Req(int Alarm_Mask)
//
//  Purpose				: unpack Tension value from the HOST
//
//  Inputs				: Msg_Buff - pointer to the received message (without the generic header) .
//
//  Outputs				: 
//
//  Returns				: Success
//
//  Description			: 
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Unpack_Reset_Alarm_Req(ONS_BYTE* Msg_Buff, int* Alarm_Mask)
{

	UINT_16* msg_p;

	msg_p = (UINT_16*)Msg_Buff;

	*Alarm_Mask = *msg_p;
#ifdef ONS_HOST
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Receive Msg_Prot_Unpack_Set_Tention , Alarm_Mask = %d", *Alarm_Mask);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 64, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
	//Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_MACHINE_STATUS_REQUEST, *Reply_Mode, *Time_Period, 0, 0, 0, 0);

#endif
	return MSG_PROT_SUCCESS;
}


// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Set_Alarm_Enable_State_Req
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
int		Msg_Prot_Pack_n_Tx_Set_Alarm_Enable_State_Req(int Enable, int Alarm_Mask)
{
	Msg_Prot_Alarm_Enable_State_Msg_t  alarm_enable_state_msg;
	int res;


	alarm_enable_state_msg.Msg_Hdr.Id = MSG_PROT_ENABLE_ALARM_STATE_CMD;

	alarm_enable_state_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	alarm_enable_state_msg.Msg_Hdr.Size = sizeof(Msg_Prot_Alarm_Enable_State_Msg_t);
	alarm_enable_state_msg.Alarm_Enable_State.Alarm_Mask = Alarm_Mask;
	alarm_enable_state_msg.Alarm_Enable_State.Enable_State = Enable;
#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	alarm_enable_state_msg.Msg_Hdr.Time_Stamp = Tx_Time;
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send alarm_enable_state_msg Alarm_Mask = %d, Enable_State = %d", Alarm_Mask, Enable);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 65, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
	//Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_PROFILE_INFO_REQ, 0, 0, 0, 0, 0, 0);
#endif // ONS_HOST

#ifdef ONS_RTC
	alarm_enable_state_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&alarm_enable_state_msg, alarm_enable_state_msg.Msg_Hdr.Size);

	return res;
}
// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Set_Alarm_Enable_State_Req
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
int	Msg_Prot_Unpack_Set_Alarm_Enable_State_Req(ONS_BYTE* Msg_Buff, int* Enable, int* Alarm_Mask)
{

	Msg_Prot_Alarm_Enable_State_t* msg_p;

	msg_p = (Msg_Prot_Alarm_Enable_State_t*)Msg_Buff;

	*Alarm_Mask = msg_p->Alarm_Mask;
	*Enable = msg_p->Enable_State;

#ifdef ONS_HOST
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Msg_Prot_Unpack_Set_Alarm_Enable_State_Req Alarm_Mask = %d, Enable_State = %d", *Alarm_Mask, *Enable);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 66, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
	//Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_MACHINE_STATUS_REQUEST, *Reply_Mode, *Time_Period, 0, 0, 0, 0);

#endif
	return MSG_PROT_SUCCESS;
}
// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Entity_Get_State
//
//  Purpose				: unpack Entity get state request from host
//
//  Inputs				: Msg_Buff - pointer to the received message (without the generic header) .
//
//  Outputs				: Entity ID - the Entity id to relay on with current state
//
//  Returns				: Success
//
//  Description			: get state request from host
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Unpack_Entity_Get_State(ONS_BYTE* Msg_Buff,int* Entity_Id)
{
	UINT_16* msg_p;

	msg_p = (UINT_16*)Msg_Buff;

	*Entity_Id = *msg_p;
#ifdef ONS_HOST
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Receive Msg_Prot_Unpack_Entity_Get_State , Entity_Id = %d", *Entity_Id);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 67, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
	//Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_MACHINE_STATUS_REQUEST, *Reply_Mode, *Time_Period, 0, 0, 0, 0);

#endif
	return MSG_PROT_SUCCESS;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Entity_Set_Cmd
//
//  Purpose				: pack & transmit to RTC Entity Set Command
//
//  Inputs				: Entity - include 8 Entity and for each one ID + required value 
//
//
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			: Send RTC Set Entity command to set the Entity to the value host request
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Pack_n_Tx_Entity_Set_Cmd(Msg_Prot_Entity_List_t Entities_To_Set)
{

	Msg_Prot_Entity_Data_Set_Msg_t		entity_data_set_msg;
#ifdef ONS_HOST
	char								msg_txt[MAX_SYSLOG_DESC_SIZE];
#endif
	int									res;

	entity_data_set_msg.Msg_Hdr.Id = MSG_PROT_ENTITY_SET_VAL_CMD;

	entity_data_set_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	entity_data_set_msg.Entity_List = Entities_To_Set;

	entity_data_set_msg.Msg_Hdr.Size = sizeof(entity_data_set_msg.Msg_Hdr) + sizeof(entity_data_set_msg.Entity_List);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	entity_data_set_msg.Msg_Hdr.Time_Stamp = Tx_Time;

	//snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_ENTITY_SET_VAL_CMD");
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 68, __FILENAME__, __LINE__, __func__, (char*)"Send MSG_PROT_ENTITY_SET_VAL_CMD");
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE,"%d-%d , %d-%d , %d-%d , %d-%d , %d-%d , %d-%d , %d-%d , %d-%d ",
			entity_data_set_msg.Entity_List.Entity[0].Entity_Id, entity_data_set_msg.Entity_List.Entity[0].Entity_Value,
			entity_data_set_msg.Entity_List.Entity[1].Entity_Id, entity_data_set_msg.Entity_List.Entity[1].Entity_Value,
			entity_data_set_msg.Entity_List.Entity[2].Entity_Id, entity_data_set_msg.Entity_List.Entity[2].Entity_Value,
			entity_data_set_msg.Entity_List.Entity[3].Entity_Id, entity_data_set_msg.Entity_List.Entity[3].Entity_Value,
			entity_data_set_msg.Entity_List.Entity[4].Entity_Id, entity_data_set_msg.Entity_List.Entity[4].Entity_Value,
			entity_data_set_msg.Entity_List.Entity[5].Entity_Id, entity_data_set_msg.Entity_List.Entity[5].Entity_Value,
			entity_data_set_msg.Entity_List.Entity[6].Entity_Id, entity_data_set_msg.Entity_List.Entity[6].Entity_Value,
			entity_data_set_msg.Entity_List.Entity[7].Entity_Id, entity_data_set_msg.Entity_List.Entity[7].Entity_Value);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 69, __FILENAME__, __LINE__, __func__, msg_txt);
//	ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);

	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
#endif // ONS_HOST

#ifdef ONS_RTC
	entity_data_set_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&entity_data_set_msg, entity_data_set_msg.Msg_Hdr.Size);

	return res;
}


// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Entity_Set_Cmd
//
//  Purpose				: unpack
//
//  Inputs				: Msg_Buff
//
//  Outputs				: Entities_To_Set - include 8 Entity and for each one ID + required value
//
//  Returns				: Success
//
//  Description			: Get entity to set command from HOST
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Unpack_Entity_Set_Cmd(ONS_BYTE* Msg_Buff, Msg_Prot_Entity_List_t* Entities_To_Set)
{
	Msg_Prot_Entity_List_t* msg_p;

	msg_p = (Msg_Prot_Entity_List_t*)Msg_Buff;

	memcpy(Entities_To_Set, msg_p, sizeof(Msg_Prot_Entity_List_t));

	return MSG_PROT_SUCCESS;
}





// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Entity_State
//
//  Purpose				: pack & transmit entity state info
//
//  Inputs				: Entity_Info - include Entity  ID + current state value 
//
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			: Send Host specific Entity state info (according to the Host request )
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Pack_n_Tx_Entity_State(Msg_Prot_Entity_Data_t Entity_Info)
{

	Msg_Prot_Entity_State_Msg_t		entity_state_msg;
#ifdef ONS_HOST
	//char							msg_txt[MAX_SYSLOG_DESC_SIZE];
#endif
	int								res;

	entity_state_msg.Msg_Hdr.Id = MSG_PROT_ENTITY_STATE_INFO;

	entity_state_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	entity_state_msg.Entity_State = Entity_Info;

	entity_state_msg.Msg_Hdr.Size = sizeof(entity_state_msg.Msg_Hdr) + sizeof(entity_state_msg.Entity_State);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	entity_state_msg.Msg_Hdr.Time_Stamp = Tx_Time;

	//snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_ENTITY_SET_VAL_CMD");
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 12, __FILENAME__, __LINE__, __func__, (char*)"Send MSG_PROT_ENTITY_SET_VAL_CMD");
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
#endif // ONS_HOST

#ifdef ONS_RTC
	entity_state_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&entity_state_msg, entity_state_msg.Msg_Hdr.Size);

	return res;
}


// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Entity_Set_Cmd
//
//  Purpose				: unpack
//
//  Inputs				: Msg_Buff
//
//  Outputs				: Entities_To_Set - include 8 Entity and for each one ID + required value
//
//  Returns				: Success
//
//  Description			: Get entity to set command from HOST
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Unpack_Entity_State(ONS_BYTE* Msg_Buff, Msg_Prot_Entity_Data_t* Entity_Info)
{
	Msg_Prot_Entity_Data_t* msg_p;

	msg_p = (Msg_Prot_Entity_Data_t*)Msg_Buff;

	memcpy(Entity_Info, msg_p, sizeof(Msg_Prot_Entity_Data_t));

#ifdef ONS_HOST
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Entity_State %d-%d , %d-%d , %d-%d , %d-%d , %d-%d , %d-%d , %d-%d , %d-%d ",
		Entity_Info[0].Entity_Id, Entity_Info[0].Entity_Value,
		Entity_Info[1].Entity_Id, Entity_Info[1].Entity_Value,
		Entity_Info[2].Entity_Id, Entity_Info[2].Entity_Value,
		Entity_Info[3].Entity_Id, Entity_Info[3].Entity_Value,
		Entity_Info[4].Entity_Id, Entity_Info[4].Entity_Value,
		Entity_Info[5].Entity_Id, Entity_Info[5].Entity_Value,
		Entity_Info[6].Entity_Id, Entity_Info[6].Entity_Value,
		Entity_Info[7].Entity_Id, Entity_Info[7].Entity_Value);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 70, __FILENAME__, __LINE__, __func__, msg_txt);
//	ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);

#endif
	return MSG_PROT_SUCCESS;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Load_RTC_To_XY_Position
//
//  Purpose				: set the rtc current location to the values received from SrvDrv module 
//
//  Inputs				: X_Position, Y_Position - current location from SrvDrv module.
//
//  Outputs				:
//
//  Returns				: Success
//
//  Description			: 
//
// ---------------------------------------------------------------------------
int	 Msg_Prot_Pack_n_Tx_Load_RTC_To_XY_Position(int X_Position, int Y_Position)
{
	Msg_Prot_RTC_Load_Pos_XY_Msg_t		XY_Pos_msg;
#ifdef ONS_HOST
	//char								msg_txt[MAX_SYSLOG_DESC_SIZE];
#endif
	int									res;

	XY_Pos_msg.Msg_Hdr.Id = MSG_PROT_RTC_LOAD_XY_POS_REQ;

	XY_Pos_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	XY_Pos_msg.Position.X_pos = X_Position;
	XY_Pos_msg.Position.Y_pos = Y_Position;

	XY_Pos_msg.Msg_Hdr.Size = sizeof(XY_Pos_msg.Msg_Hdr) + sizeof(XY_Pos_msg.Position);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	XY_Pos_msg.Msg_Hdr.Time_Stamp = Tx_Time;

	//snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_RTC_LOAD_XY_POS");
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 71, __FILENAME__, __LINE__, __func__, (char*)"Send MSG_PROT_RTC_LOAD_XY_POS");
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
#endif // ONS_HOST

#ifdef ONS_RTC
	XY_Pos_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&XY_Pos_msg, XY_Pos_msg.Msg_Hdr.Size);
	return res;
}
// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Load_RTC_To_XY_Position
//
//  Purpose				: unpack 
//
//  Inputs				: Msg_Buff - pointer to the received message (without the generic header).
//
//  Outputs				: X_Position, Y_Position - current location from SrvDrv module.
//
//  Returns				: Success
//
//  Description			: unpack srvdrv location and use it as current RTC position
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Unpack_Load_RTC_To_XY_Position(ONS_BYTE* Msg_Buff, int*  X_Position, int* Y_Position)
{
	Msg_Prot_Pos_XY_t*		msg_p;

	msg_p = (Msg_Prot_Pos_XY_t*)Msg_Buff;
	*X_Position = msg_p->X_pos;
	*Y_Position = msg_p->Y_pos;

#ifdef ONS_HOST
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Receive XY position x=%d, y=%d ", *X_Position, *Y_Position);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 72, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
#endif
	return MSG_PROT_SUCCESS;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Load_RTC_To_XY_Position_Response
//
//  Purpose				: RTC load xy pos responce 
//
//  Inputs				: State.
//
//  Outputs				:
//
//  Returns				: Success
//
//  Description			: 
//
// ---------------------------------------------------------------------------
int	 Msg_Prot_Pack_n_Tx_Load_RTC_To_XY_Position_Response(ONS_Load_XY_Pos_State_t State)
{
	Msg_Prot_RTC_Load_Pos_XY_Response_Msg_t		load_XY_response;
	int											res;
#ifdef ONS_HOST
	//char										msg_txt[MAX_SYSLOG_DESC_SIZE];
#endif

	load_XY_response.Msg_Hdr.Id				= MSG_PROT_RTC_LOAD_XY_POS_RESPONSE;

	load_XY_response.Msg_Hdr.Serial_Number	= Msg_Prot_Get_Msg_SN();

	load_XY_response.State					= State;

	load_XY_response.Msg_Hdr.Size			= sizeof(load_XY_response.Msg_Hdr) + sizeof(load_XY_response.State);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	load_XY_response.Msg_Hdr.Time_Stamp = Tx_Time;

	//snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_RTC_LOAD_XY_POS_RESPONSE");
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 73, __FILENAME__, __LINE__, __func__, (char*)"Send MSG_PROT_RTC_LOAD_XY_POS_RESPONSE");
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
#endif // ONS_HOST

#ifdef ONS_RTC
	load_XY_response.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&load_XY_response, load_XY_response.Msg_Hdr.Size);
	return res;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Load_RTC_To_XY_Position
//
//  Purpose				: unpack 
//
//  Inputs				: Msg_Buff - pointer to the received message (without the generic header).
//
//  Outputs				: State - Load xy pos responce state.
//
//  Returns				: Success
//
//  Description			: unpack and get response state for RTC load pos
//
// ---------------------------------------------------------------------------
int Msg_Prot_Unpack_Load_RTC_To_XY_Position_Response(ONS_BYTE* Msg_Buff, INT_16*  State)
{
	//ONS_Load_XY_Pos_State_t*		msg_p;

	//msg_p = (ONS_Load_XY_Pos_State_t*)Msg_Buff;
	*State = (INT_16)*Msg_Buff;

#ifdef ONS_HOST
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Receive XY position Response TMP=%d State = %d", (INT_16)*Msg_Buff, *State);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 74, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
#endif
	return MSG_PROT_SUCCESS;
}





// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Set_Servo_Clamp_Cmd
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
int	 Msg_Prot_Pack_n_Tx_Set_Servo_Clamp_Cmd(ONS_Servo_Clamp_Mode_t Mode, int Level)
{
	Msg_Prot_Servo_Clamp_Cmd_Msg_t		servo_clamp_cmd_msg;
	int									res;

#ifdef ONS_HOST
	char										msg_txt[MAX_SYSLOG_DESC_SIZE];
#endif

	servo_clamp_cmd_msg.Msg_Hdr.Id = MSG_PROT_SET_SERVO_CLAMP_CMD;

	servo_clamp_cmd_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	servo_clamp_cmd_msg.Command.Mode = Mode;
	servo_clamp_cmd_msg.Command.Level = Level;

	servo_clamp_cmd_msg.Msg_Hdr.Size = sizeof(servo_clamp_cmd_msg.Msg_Hdr) + sizeof(servo_clamp_cmd_msg.Command);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	servo_clamp_cmd_msg.Msg_Hdr.Time_Stamp = Tx_Time;

	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROTSET_SERVO_CLAMP_CMD  Mode = %d Level = %d", servo_clamp_cmd_msg.Command.Mode, servo_clamp_cmd_msg.Command.Level);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 75, __FILENAME__, __LINE__, __func__, msg_txt);
//	ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
#endif // ONS_HOST

#ifdef ONS_RTC
	servo_clamp_cmd_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&servo_clamp_cmd_msg, servo_clamp_cmd_msg.Msg_Hdr.Size);

	return res;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Set_Servo_Clamp_Cmd
//
//  Purpose				: unpack 
//
//  Inputs				:  
//
//  Outputs				:  
//
//  Returns				: Success
//
//  Description			: 
//
// ---------------------------------------------------------------------------
int Msg_Prot_Unpack_Set_Servo_Clamp_Cmd(ONS_BYTE* Msg_Buff, ONS_Servo_Clamp_Mode_t* Mode, int* Level)
{
	Msg_Prot_Servo_Clamp_Cmd_t*		msg_p;
	 
	msg_p = (Msg_Prot_Servo_Clamp_Cmd_t*)Msg_Buff;

	*Mode = (ONS_Servo_Clamp_Mode_t)msg_p->Mode;
	*Level = msg_p->Level;

#ifdef ONS_HOST
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Receive Set Servo Clamp position to Mode = %d Level = %d", *Mode, *Level);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 76, __FILENAME__, __LINE__, __func__, msg_txt);
	//	ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
#endif
	return MSG_PROT_SUCCESS;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Set_Run_Time_RTC_Config_Data
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
int Msg_Prot_Pack_n_Tx_Set_RTC_Run_Time_Config_Data(int Air_Pressure_Enable, int TB_Enable, int Wiper_Enable, int Pallet_Enable, int APF_Error_Check_Enable)
{
	Msg_Prot_RTC_Config_Data_Msg_t			RTC_Config_Data_Msg;
	int										res;

#ifdef ONS_HOST
	char									msg_txt[MAX_SYSLOG_DESC_SIZE];
#endif
	
	RTC_Config_Data_Msg.Msg_Hdr.Id = MSG_PROT_SET_RTC_CONFIG_CMD;

	RTC_Config_Data_Msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	RTC_Config_Data_Msg.Config_Data = 0;

	if(TB_Enable)
	{
		RTC_Config_Data_Msg.Config_Data |= MSG_PROT_RTC_CONFIG_THREAD_BREAK_ENABLE_MASK;
	}	
	
	if (Pallet_Enable)
	{
		RTC_Config_Data_Msg.Config_Data |= MSG_PROT_RTC_CONFIG_PALLET_ENABLE_MASK;
	}
	if (Air_Pressure_Enable)
	{
		RTC_Config_Data_Msg.Config_Data |= MSG_PROT_RTC_CONFIG_AIR_PRESSUER_ENABLE_MASK;
	}

	if(Wiper_Enable)
	{
		RTC_Config_Data_Msg.Config_Data |= MSG_PROT_RTC_CONFIG_WIPER_ENABLE_MASK;
	}
	
	if (APF_Error_Check_Enable)
	{
		RTC_Config_Data_Msg.Config_Data |= MSG_PROT_RTC_CONFIG_APF_ERROR_CHECK_ENABLE_MASK;
	}

	RTC_Config_Data_Msg.Msg_Hdr.Size = sizeof(RTC_Config_Data_Msg);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	RTC_Config_Data_Msg.Msg_Hdr.Time_Stamp = Tx_Time;

	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send RTC Config = 0x%x\n", RTC_Config_Data_Msg.Config_Data);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 77, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
#endif // ONS_HOST

#ifdef ONS_RTC
	RTC_Config_Data_Msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&RTC_Config_Data_Msg, RTC_Config_Data_Msg.Msg_Hdr.Size);

	return res;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Set_RTC_Run_Time_Config_Data
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
int	Msg_Prot_Unpack_Set_RTC_Run_Time_Config_Data(ONS_BYTE* Msg_Buff, int* RTC_Config)
{
	int*		msg_p;

	msg_p = (int*)Msg_Buff;

	*RTC_Config = *msg_p;

#ifdef ONS_HOST
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Receive RTC Config = 0x%x", *RTC_Config);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 78, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
#endif
	return MSG_PROT_SUCCESS;
}


// RTC IO List Message
// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Init_RTC_IO_List
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
int Msg_Prot_Pack_n_Tx_Init_RTC_IO_List(char Start, char End, unsigned char* IO_Num)
{
	Msg_Prot_IO_List_Init_Msg_t			RTC_IO_List_Init_Msg;
	int									res;

#ifdef ONS_HOST
	char								msg_txt[MAX_SYSLOG_DESC_SIZE];
#endif

	RTC_IO_List_Init_Msg.Msg_Hdr.Id = MSG_PROT_INIT_IO_LIST_CMD;

	RTC_IO_List_Init_Msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	RTC_IO_List_Init_Msg.IO_List.Start	= Start;

	RTC_IO_List_Init_Msg.IO_List.End	= End;

	memcpy(RTC_IO_List_Init_Msg.IO_List.IO_Num, IO_Num, sizeof(unsigned char)*MSG_PROT_MAX_NUM_OF_ID_IN_IO_LIST);

	RTC_IO_List_Init_Msg.Msg_Hdr.Size = sizeof(Msg_Prot_IO_List_Init_Msg_t);

	

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	RTC_IO_List_Init_Msg.Msg_Hdr.Time_Stamp = Tx_Time;

	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "IO_List_Init start = %d, end = %d\n", RTC_IO_List_Init_Msg.IO_List.Start, RTC_IO_List_Init_Msg.IO_List.End);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 79, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
/*	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, " %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d \n", 
		RTC_IO_List_Init_Msg.IO_List.IO_Num[0], RTC_IO_List_Init_Msg.IO_List.IO_Num[1], RTC_IO_List_Init_Msg.IO_List.IO_Num[2], RTC_IO_List_Init_Msg.IO_List.IO_Num[3], 
		RTC_IO_List_Init_Msg.IO_List.IO_Num[4], RTC_IO_List_Init_Msg.IO_List.IO_Num[5], RTC_IO_List_Init_Msg.IO_List.IO_Num[6], RTC_IO_List_Init_Msg.IO_List.IO_Num[7], 
		RTC_IO_List_Init_Msg.IO_List.IO_Num[8], RTC_IO_List_Init_Msg.IO_List.IO_Num[9], RTC_IO_List_Init_Msg.IO_List.IO_Num[10], RTC_IO_List_Init_Msg.IO_List.IO_Num[11], 
		RTC_IO_List_Init_Msg.IO_List.IO_Num[12], RTC_IO_List_Init_Msg.IO_List.IO_Num[13], RTC_IO_List_Init_Msg.IO_List.IO_Num[14], RTC_IO_List_Init_Msg.IO_List.IO_Num[15], 
		RTC_IO_List_Init_Msg.IO_List.IO_Num[16], RTC_IO_List_Init_Msg.IO_List.IO_Num[17], RTC_IO_List_Init_Msg.IO_List.IO_Num[18], RTC_IO_List_Init_Msg.IO_List.IO_Num[19]);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
*/
#endif // ONS_HOST

#ifdef ONS_RTC
	RTC_IO_List_Init_Msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&RTC_IO_List_Init_Msg, RTC_IO_List_Init_Msg.Msg_Hdr.Size);

	return res;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_IO_Init_RTC_IO_List
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
int  Msg_Prot_Unpack_IO_Init_RTC_IO_List(ONS_BYTE* Msg_Buff, char* Start, char* End, unsigned char* IO_Num)
{
	Msg_Prot_IO_List_Init_t*		msg_p;

	msg_p = (Msg_Prot_IO_List_Init_t*)Msg_Buff;

	*Start = msg_p->Start;
	*End = msg_p->End;
	memcpy(IO_Num, msg_p->IO_Num, sizeof(unsigned char)*MSG_PROT_MAX_NUM_OF_ID_IN_IO_LIST);

#ifdef ONS_HOST
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Receive IO list Command");
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 80, __FILENAME__, __LINE__, __func__, msg_txt);
//	ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
#endif
	return MSG_PROT_SUCCESS;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_RTC_IO_List_Data
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
int	Msg_Prot_Pack_n_Tx_RTC_IO_List_Values(UINT_16 IO_List_Values_1, UINT_16 IO_List_Values_2, UINT_16 IO_List_Values_3, UINT_16 IO_List_Values_4, UINT_16 IO_List_Values_5, UINT_16 IO_List_Values_6, UINT_16 IO_List_Values_7, UINT_16 IO_List_Values_8)
{
	Msg_Prot_IO_List_Values_Msg_t			RTC_IO_List_Values_Msg;
	int									res;

#ifdef ONS_HOST
	char								msg_txt[MAX_SYSLOG_DESC_SIZE];
#endif

	RTC_IO_List_Values_Msg.Msg_Hdr.Id = MSG_PROT_RTC_IO_LIST_VALUES;

	RTC_IO_List_Values_Msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	RTC_IO_List_Values_Msg.RTC_Values.IO_List_Values_1 = IO_List_Values_1;

	RTC_IO_List_Values_Msg.RTC_Values.IO_List_Values_2 = IO_List_Values_2;

	RTC_IO_List_Values_Msg.RTC_Values.IO_List_Values_3 = IO_List_Values_3;

	RTC_IO_List_Values_Msg.RTC_Values.IO_List_Values_4 = IO_List_Values_4;

	RTC_IO_List_Values_Msg.RTC_Values.IO_List_Values_5 = IO_List_Values_5;
	
	RTC_IO_List_Values_Msg.RTC_Values.IO_List_Values_6 = IO_List_Values_6;
	
	RTC_IO_List_Values_Msg.RTC_Values.IO_List_Values_7 = IO_List_Values_7;

	RTC_IO_List_Values_Msg.RTC_Values.IO_List_Values_8 = IO_List_Values_8;
	
	RTC_IO_List_Values_Msg.Msg_Hdr.Size = sizeof(Msg_Prot_IO_List_Values_Msg_t);
#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	RTC_IO_List_Values_Msg.Msg_Hdr.Time_Stamp = Tx_Time;

	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "IO_List_Init IO_List : Values_1 = 0x%x, Values_2 = 0x%x, Values_3 = 0x%x, Values_4 = 0x%x, Values_5 = 0x%x, Values_6 = 0x%x, Values_7 = 0x%x, Values_8 = 0x%x\n", RTC_IO_List_Values_Msg.RTC_Values.IO_List_Values_1, RTC_IO_List_Values_Msg.RTC_Values.IO_List_Values_2, RTC_IO_List_Values_Msg.RTC_Values.IO_List_Values_3, RTC_IO_List_Values_Msg.RTC_Values.IO_List_Values_4, RTC_IO_List_Values_Msg.RTC_Values.IO_List_Values_5, RTC_IO_List_Values_Msg.RTC_Values.IO_List_Values_6, RTC_IO_List_Values_Msg.RTC_Values.IO_List_Values_7, RTC_IO_List_Values_Msg.RTC_Values.IO_List_Values_8);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 81, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
#endif // ONS_HOST

#ifdef ONS_RTC
	RTC_IO_List_Values_Msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&RTC_IO_List_Values_Msg, RTC_IO_List_Values_Msg.Msg_Hdr.Size);

	return res;


}


// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_RTC_IO_List_Values
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
int	Msg_Prot_Unpack_RTC_IO_List_Values(ONS_BYTE* Msg_Buff, UINT_16* IO_List_Values_1, UINT_16* IO_List_Values_2, UINT_16* IO_List_Values_3, UINT_16* IO_List_Values_4, UINT_16* IO_List_Values_5, UINT_16* IO_List_Values_6, UINT_16* IO_List_Values_7, UINT_16* IO_List_Values_8)
{
	Msg_Prot_IO_List_Values_t*		msg_p;

	msg_p = (Msg_Prot_IO_List_Values_t*)Msg_Buff;

	*IO_List_Values_1 = msg_p->IO_List_Values_1;
	*IO_List_Values_2 = msg_p->IO_List_Values_2;
	*IO_List_Values_3 = msg_p->IO_List_Values_3;
	*IO_List_Values_4 = msg_p->IO_List_Values_4;
	*IO_List_Values_5 = msg_p->IO_List_Values_5;
	*IO_List_Values_6 = msg_p->IO_List_Values_6;
	*IO_List_Values_7 = msg_p->IO_List_Values_7;
	*IO_List_Values_8 = msg_p->IO_List_Values_8;

#ifdef ONS_HOST
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Receive IO List values : IO_List_Values_1 = 0x%x, IO_List_Values_2 = 0x%x, IO_List_Values_3 = 0x%x IO_List_Values_4 = 0x%x, IO_List_Values_5 = 0x%x, IO_List_Values_6 = 0x%x, IO_List_Values_7 = 0x%x, IO_List_Values_8 = 0x%x", *IO_List_Values_1, *IO_List_Values_2, *IO_List_Values_3, *IO_List_Values_4, *IO_List_Values_5, *IO_List_Values_6, *IO_List_Values_7, *IO_List_Values_8);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 82, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
#endif
	return MSG_PROT_SUCCESS;
}







// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_IO_List_Status_Request n 
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
int	Msg_Prot_Pack_n_Tx_IO_List_Status_Request(Status_Reply_Mode_t Reply_Mode, int Time_Period)
{
	Msg_Prot_IO_List_Status_Req_Msg_t		IO_List_Status_Req_Msg;
	int										res;

#ifdef ONS_HOST
	char								msg_txt[MAX_SYSLOG_DESC_SIZE];
#endif

	IO_List_Status_Req_Msg.Msg_Hdr.Id = MSG_PROT_IO_LIST_STATUS_REQUEST;

	IO_List_Status_Req_Msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	IO_List_Status_Req_Msg.IO_List_Status_Req.Reply_Mode = (INT_16)Reply_Mode;

	IO_List_Status_Req_Msg.IO_List_Status_Req.Time_Period = Time_Period;


	IO_List_Status_Req_Msg.Msg_Hdr.Size = sizeof(Msg_Prot_IO_List_Status_Req_Msg_t);
#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	IO_List_Status_Req_Msg.Msg_Hdr.Time_Stamp = Tx_Time;

	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "IO_List_Status_Req : Reply_Mode = %d, Time_Period = %d\n", IO_List_Status_Req_Msg.IO_List_Status_Req.Reply_Mode, IO_List_Status_Req_Msg.IO_List_Status_Req.Time_Period);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 83, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
#endif // ONS_HOST

#ifdef ONS_RTC
	IO_List_Status_Req_Msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&IO_List_Status_Req_Msg, IO_List_Status_Req_Msg.Msg_Hdr.Size);

	return res;


}


// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_IO_List_Status_Request
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
int	Msg_Prot_Unpack_IO_List_Status_Request(ONS_BYTE* Msg_Buff, Status_Reply_Mode_t* Reply_Mode, int* Time_Period)
{
	Msg_Prot_IO_List_Status_Req_t*		msg_p;

	msg_p = (Msg_Prot_IO_List_Status_Req_t*)Msg_Buff;

	*Reply_Mode = (Status_Reply_Mode_t)msg_p->Reply_Mode;
	*Time_Period = msg_p->Time_Period;
	

#ifdef ONS_HOST
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Receive IO List Request : Reply_Mode = %d, Time_Period = %d", *Reply_Mode, *Time_Period);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 84, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
#endif
	return MSG_PROT_SUCCESS;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Set_Lamp_Cmd
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
int	Msg_Prot_Pack_n_Tx_Set_Lamp_Cmd(Msg_Prot_Lamp_Command_t Command)
{
	Msg_Prot_Lamp_Command_Msg_t				Lamp_Command_Msg;
	int										res;

#ifdef ONS_HOST
	char								msg_txt[MAX_SYSLOG_DESC_SIZE];
#endif

	Lamp_Command_Msg.Msg_Hdr.Id = MSG_PROT_SET_LAMP_COMMAND;	

	Lamp_Command_Msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	Lamp_Command_Msg.Lamp_Command = Command;


	Lamp_Command_Msg.Msg_Hdr.Size = sizeof(Msg_Prot_Lamp_Command_Msg_t);
#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	Lamp_Command_Msg.Msg_Hdr.Time_Stamp = Tx_Time;

	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Lamp_Command = %d\n", Lamp_Command_Msg.Lamp_Command);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 85, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
#endif // ONS_HOST

#ifdef ONS_RTC
	Lamp_Command_Msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&Lamp_Command_Msg, Lamp_Command_Msg.Msg_Hdr.Size);

	return res;

}




// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Set_Lamp_Cmd
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
int Msg_Prot_Unpack_Set_Lamp_Cmd(ONS_BYTE* Msg_Buff, Msg_Prot_Lamp_Command_t* Command)
{
	Msg_Prot_Lamp_Command_t*		msg_p;

	msg_p = (Msg_Prot_Lamp_Command_t*)Msg_Buff;

	*Command = *msg_p;


#ifdef ONS_HOST
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Loader Connamd = %d", *Command);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 86, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
#endif
	return MSG_PROT_SUCCESS;
}

// -------------------------------------------------------------------------- -
//  Function name		: Msg_Prot_Pack_n_Tx_Set_Lamp_Cmd
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
int	Msg_Prot_Pack_n_Tx_Set_Spool_Counters_Cmd(Msg_Prot_Counter_Command_t Bobbin_Command, int Bobbin_Command_Value, Msg_Prot_Counter_Command_t Winder_Command, int Winder_Command_Value, Msg_Prot_Counter_Command_t Thread_Command, int Thread_Command_Value)
{
	Msg_Prot_Spool_Countrs_Cmd_Msg_t		Spool_Countrs_Command_Msg;
	int										res;

#ifdef ONS_HOST
	char								msg_txt[MAX_SYSLOG_DESC_SIZE];
#endif

	Spool_Countrs_Command_Msg.Msg_Hdr.Id = MSG_PROT_SEND_RTC_SPOOL_COUNTERS_CMD;

	Spool_Countrs_Command_Msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	Spool_Countrs_Command_Msg.Spool_Counters_Cmd.Bobbin_Counter_Cmd.Command = Bobbin_Command;
	Spool_Countrs_Command_Msg.Spool_Counters_Cmd.Bobbin_Counter_Cmd.Command_Value = Bobbin_Command_Value;

	Spool_Countrs_Command_Msg.Spool_Counters_Cmd.Winder_Counter_Cmd.Command = Winder_Command;
	Spool_Countrs_Command_Msg.Spool_Counters_Cmd.Winder_Counter_Cmd.Command_Value = Winder_Command_Value;

	Spool_Countrs_Command_Msg.Spool_Counters_Cmd.Thread_Counter_Cmd.Command = Thread_Command;
	Spool_Countrs_Command_Msg.Spool_Counters_Cmd.Thread_Counter_Cmd.Command_Value = Thread_Command_Value;

	Spool_Countrs_Command_Msg.Msg_Hdr.Size = sizeof(Msg_Prot_Spool_Countrs_Cmd_Msg_t);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	Spool_Countrs_Command_Msg.Msg_Hdr.Time_Stamp = Tx_Time;

	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Bobbin Cmd = %d, val= %d, Winder Cmd = %d, val = %d, Thread Cmd =%d, val =%d\n", Bobbin_Command,  Bobbin_Command_Value,  Winder_Command,  Winder_Command_Value,  Thread_Command,  Thread_Command_Value);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 87, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
#endif // ONS_HOST

#ifdef ONS_RTC
	Spool_Countrs_Command_Msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&Spool_Countrs_Command_Msg, Spool_Countrs_Command_Msg.Msg_Hdr.Size);

	return res;

}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Set_Spool_Counters_Cmd
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
int		Msg_Prot_Unpack_Set_Spool_Counters_Cmd(ONS_BYTE* Msg_Buff, Spool_Countrs_Cmd_t* Bobbin_Counter, Spool_Countrs_Cmd_t* Winder_Counter, Spool_Countrs_Cmd_t* Thread_Counter)
{
	Msg_Prot_Spool_Countrs_Cmd_t*		msg_p;

	msg_p = (Msg_Prot_Spool_Countrs_Cmd_t*)Msg_Buff;

	Bobbin_Counter->Command = msg_p->Bobbin_Counter_Cmd.Command;
	Bobbin_Counter->Command_Value = msg_p->Bobbin_Counter_Cmd.Command_Value;
	Winder_Counter->Command = msg_p->Winder_Counter_Cmd.Command;
	Winder_Counter->Command_Value = msg_p->Winder_Counter_Cmd.Command_Value;
	Thread_Counter->Command = msg_p->Thread_Counter_Cmd.Command;
	Thread_Counter->Command_Value = msg_p->Thread_Counter_Cmd.Command_Value;


#ifdef ONS_HOST
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Bobbin Connamd = %d, val =%d Wider Connamd = %d, val =%d Thread Connamd = %d, val =%d", Bobbin_Counter->Command, Bobbin_Counter->Command_Value, Winder_Counter->Command, Winder_Counter->Command_Value, Thread_Counter->Command, Thread_Counter->Command_Value);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 88, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
#endif
	return MSG_PROT_SUCCESS;
}

//RPM Command
// -------------------------------------------------------------------------- -
//  Function name		: Msg_Prot_Pack_n_Tx_Set_RPM_Cmd
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
int	Msg_Prot_Pack_n_Tx_Set_RPM_Cmd(int RPM_Speed)
{
	Msg_Prot_RPM_Command_Msg_t		RPM_Command_Msg;
	int										res;

#ifdef ONS_HOST
	char								msg_txt[MAX_SYSLOG_DESC_SIZE];
#endif

	RPM_Command_Msg.Msg_Hdr.Id = MSG_PROT_RTC_SET_RPM;

	RPM_Command_Msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	RPM_Command_Msg.RPM_Speed = RPM_Speed;
	RPM_Command_Msg.Msg_Hdr.Size = sizeof(Msg_Prot_RPM_Command_Msg_t);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	RPM_Command_Msg.Msg_Hdr.Time_Stamp = Tx_Time;

	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "RPM Speed =%d\n", RPM_Speed);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 89, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
#endif // ONS_HOST

#ifdef ONS_RTC
	RPM_Command_Msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&RPM_Command_Msg, RPM_Command_Msg.Msg_Hdr.Size);

	return res;

}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Set_RPM_Cmd
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
int		Msg_Prot_Unpack_Set_RPM_Cmd(ONS_BYTE* Msg_Buff, int* RPM_Speed)
{
	int*		msg_p;

	msg_p = (int*)Msg_Buff;

	*RPM_Speed = *msg_p;


#ifdef ONS_HOST
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "RPM Speed = %d", *RPM_Speed);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 89, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
#endif
	return MSG_PROT_SUCCESS;
}

// -------------------------------------------------------------------------- -
//  Function name		: Msg_Prot_Pack_n_Tx_Set_IO_Latch_Cmd
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
int		Msg_Prot_Pack_n_Tx_Set_IO_Latch_Mode_Cmd(int IO_Latch_Mode_Active)
{
	Msg_Prot_Latch_Mode_Command_Msg_t		IO_Latch_Mode_Command_Msg;
	int										res;

#ifdef ONS_HOST
	char								msg_txt[MAX_SYSLOG_DESC_SIZE];
#endif

	IO_Latch_Mode_Command_Msg.Msg_Hdr.Id = MSG_PROT_RTC_SET_IO_LATCH_MODE;

	IO_Latch_Mode_Command_Msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	IO_Latch_Mode_Command_Msg.IO_Latch_Mode_Active = IO_Latch_Mode_Active;
	IO_Latch_Mode_Command_Msg.Msg_Hdr.Size = sizeof(Msg_Prot_Latch_Mode_Command_Msg_t);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	IO_Latch_Mode_Command_Msg.Msg_Hdr.Time_Stamp = Tx_Time;

	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "IO_Latch_Mode_Active =%d\n", IO_Latch_Mode_Command_Msg.IO_Latch_Mode_Active);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 90, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
#endif // ONS_HOST

#ifdef ONS_RTC
	IO_Latch_Mode_Command_Msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&IO_Latch_Mode_Command_Msg, IO_Latch_Mode_Command_Msg.Msg_Hdr.Size);

	return res;

}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Set_IO_Latch_Mode_Cmd
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
int		Msg_Prot_Unpack_Set_IO_Latch_Mode_Cmd(ONS_BYTE* Msg_Buff, int* IO_Latch_Mode_Active)
{
	int*		msg_p;

	msg_p = (int*)Msg_Buff;

	*IO_Latch_Mode_Active = *msg_p;


#ifdef ONS_HOST
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "IO_Latch_Mode_Active = %d", *IO_Latch_Mode_Active);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 91, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
#endif
	return MSG_PROT_SUCCESS;
}


// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_RTC_Move_X_Y_STC
//
//  Purpose				: pack & transmit Move like stitch command to RTC
//
//  Inputs				: X_Pos, Y_pos - the destination position for the movement
//						  Dt_X, Dt_Y - the movement dt for each axe.
//
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			: send to RTC the destination location for the movement and the dt for the axes
//
// ---------------------------------------------------------------------------
int Msg_Prot_Pack_n_Tx_RTC_Move_X_Y_STC(int  X_Pos, int Y_pos, int Dt_X, int Dt_Y)
{
	Msg_Prot_RTC_Move_X_Y_STC_Msg_t  RTC_Move_STC_msg;
	int								res;

	RTC_Move_STC_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	RTC_Move_STC_msg.Msg_Hdr.Id = MSG_PROT_RTC_MOVE_X_Y_STC;

	RTC_Move_STC_msg.Msg_Hdr.Size = sizeof(RTC_Move_STC_msg.Msg_Hdr) + sizeof(RTC_Move_STC_msg.Info);

	RTC_Move_STC_msg.Info.X_pos = X_Pos;

	RTC_Move_STC_msg.Info.Y_pos = Y_pos;

	RTC_Move_STC_msg.Info.Dt_X = Dt_X;

	RTC_Move_STC_msg.Info.Dt_Y = Dt_Y;

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	RTC_Move_STC_msg.Msg_Hdr.Time_Stamp = Tx_Time;
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send RTC_Move_STC_msg : X_Pos =%d, Y_pos = %d, Dt_X = %d, Dt_Y = %d", X_Pos, Y_pos, Dt_X, Dt_Y);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 92, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
#endif // ONS_HOST

	res = USBComm_Send_Message((USBComm_Buffer_t)&RTC_Move_STC_msg, RTC_Move_STC_msg.Msg_Hdr.Size);

	return res;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_RTC_Move_X_Y_STC
//
//  Purpose				: unpack Move like Stitch message
//
//  Inputs				: Msg_Buff - pointer to the received message (without the generic header) .
//
//  Outputs				: X_Pos, Y_pos - the destination position for the movement
//						  Dt_X, Dt_Y - the movement dt for each axe.
//
//  Returns				: Success
//
//  Description			: RTC gets the destination for the move commend and the dt for each axe
//
// ---------------------------------------------------------------------------
int  Msg_Prot_Unpack_RTC_Move_X_Y_STC(ONS_BYTE* Msg_Buff, int*  X_Pos, int* Y_Pos, int* Dt_X, int* Dt_Y)
{

	Msg_Prot_RTC_Move_X_Y_STC_t*  msg_p;

	msg_p = (Msg_Prot_RTC_Move_X_Y_STC_t*)Msg_Buff;

	*X_Pos = msg_p->X_pos;

	*Y_Pos = msg_p->Y_pos;

	*Dt_X = msg_p->Dt_X;

	*Dt_Y = msg_p->Dt_Y;

	return MSG_PROT_SUCCESS;
}



//***********************************************************************************************************************************************************************************
//  TEST MESSAGES
//***********************************************************************************************************************************************************************************

//Host Test Message

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_&_Tx_Host_Tst_Msg
//
//  Purpose				: pack & transmit host to RTC Test message
//
//  Inputs				:
//
//  Outputs				: Msg_Buff - data of the message.
//						: Size - the size of data.
//
//  Returns				: success or error code
//
//  Description			: update message header and data fields
//
// ---------------------------------------------------------------------------
int Msg_Prot_Pack_n_Tx_Host_Tst_Msg(int* Wait, int* Num_Of_Msg, int*  Delta_Time, int* Return_Size,int* Fill_Size,unsigned long* Tx_Time)
{

	Msg_Prot_Host_Tst_Msg_t  host_test_msg;

	int res;

	int index;

	USBComm_Buffer_t 	msg_p;

	int 				tmp_size;  //size of the Message info and the fill array (for checksum calculation)

    
	host_test_msg.Msg_Hdr.Id = MSG_PROT_TEST_USB_HOST_TO_RTC;

	host_test_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	host_test_msg.Test_msg_info.Wait = *Wait;

	host_test_msg.Test_msg_info.Num_Of_Msg = *Num_Of_Msg;

	host_test_msg.Test_msg_info.Delta_Time = *Delta_Time;

	host_test_msg.Test_msg_info.Return_Size = *Return_Size;

	host_test_msg.Test_msg_info.SN = host_test_msg.Msg_Hdr.Serial_Number;

	host_test_msg.Test_msg_info.Fill_Size =  *Fill_Size;

	host_test_msg.Test_msg_info.Checksum = 0;

	//Verify fill Size (to avoid array overflow)
	if (*Fill_Size > TEST_HOST_FILL_SIZE)
	{

		host_test_msg.Test_msg_info.Fill_Size = TEST_HOST_FILL_SIZE;

	}

	if (*Fill_Size < 0)
	{

		host_test_msg.Test_msg_info.Fill_Size = 0;

	}


	//update Fill array

	for  (index = 0; index < host_test_msg.Test_msg_info.Fill_Size; index++)
	{

		host_test_msg.Fill[index] = 1 + index;

	}

	msg_p = (USBComm_Buffer_t)&host_test_msg;

	msg_p    = &msg_p[sizeof(host_test_msg.Msg_Hdr)];    //msg_p point to the start of host test message info

	tmp_size =  sizeof(host_test_msg.Test_msg_info) + host_test_msg.Test_msg_info.Fill_Size;  //size of the Message info and the fill array (for checksum calculation)

	host_test_msg.Test_msg_info.Checksum = Msg_Prot_Checksum_calculator(msg_p, tmp_size);      //calculate checksum on given message (when checksum field = 0)

	host_test_msg.Msg_Hdr.Size = sizeof(host_test_msg.Msg_Hdr)+ sizeof(host_test_msg.Test_msg_info) + host_test_msg.Test_msg_info.Fill_Size;    //the real size of message (general_message+test message+fill)

#ifdef ONS_HOST
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	*Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	*Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
    host_test_msg.Msg_Hdr.Time_Stamp = *Tx_Time;
#endif // ONS_HOST

#ifdef ONS_RTC

    host_test_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();

#endif
	
	res = USBComm_Send_Message( (USBComm_Buffer_t)&host_test_msg , host_test_msg.Msg_Hdr.Size );

	return res;
}

 // ---------------------------------------------------------------------------
 //  Function name		: Msg_Prot_Unpack_Host_Test_Message
 //
 //  Purpose				: unpack test message
 //
 //  Inputs				: Msg_Buff - pointer to the received message (without the generic header) .
 //						: Msg_Size - the size of the received data.
 //
 //  Outputs				:Tst_info - all info from the header
 //
 //  Returns				: Success or Error code
 //
 //  Description			: Unpack message from Data_Buff validate the size
 //							: and the checksum. send message to log/to screen.
 //
 // ---------------------------------------------------------------------------
 int Msg_Prot_Unpack_Host_Test_Message(ONS_BYTE* Msg_Buff, Msg_Prot_Host_Tst_Msg_info_t* Tst_Info)
 {
	int	msg_size;
 	unsigned long calculated_checksum;
 	unsigned long tmp_checksum;
 	Msg_Prot_Host_Tst_Msg_info_t* msg_p;


	msg_p = (Msg_Prot_Host_Tst_Msg_info_t*)Msg_Buff;

	tmp_checksum = msg_p->Checksum;

	msg_size = msg_p->Fill_Size + sizeof(Msg_Prot_Host_Tst_Msg_info_t);

	msg_p->Checksum = 0;

 	memcpy(Tst_Info, Msg_Buff, sizeof(Msg_Prot_Host_Tst_Msg_info_t));


 	// validate checksum
 	calculated_checksum = Msg_Prot_Checksum_calculator(Msg_Buff, msg_size);      //calculate checksum on given message (when checksum field = 0)

 	if (tmp_checksum != calculated_checksum)
 	{
 		return  MSG_PROT_ERROR_CHECKSUM_MESSAGE;
 	}

 	return MSG_PROT_SUCCESS;
 }





 //RTC Test Message

 // ---------------------------------------------------------------------------
 //  Function name		: Msg_Prot_Pack_&_Tx_RTC_Tst_Msg
 //
 //  Purpose				: pack & transmit RTC to Host Test message
 //
 //  Inputs				:
 //
 //  Outputs				: Msg_Buff - data of the message.
 //							: Size - the size of fill array.
 //
 //  Returns				: success or error code
 //
 //  Description			: update message fields
 //
 // ---------------------------------------------------------------------------
  int Msg_Prot_Pack_n_Tx_RTC_Tst_Msg(unsigned long Origin_Msg_SN, int Return_Size)
 {

	 USBComm_Status_t Status;
	 Msg_Prot_RTC_Tst_Msg_t rtc_test_msg;
	 int index;
	 int res;
	 USBComm_Buffer_t 	msg_p;
	 int 				tmp_size;  //size of the Message info and the fill array (for checksum calculation)

     rtc_test_msg.Msg_Hdr.Id = MSG_PROT_TEST_USB_RTC_TO_HOST;
	 rtc_test_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	 rtc_test_msg.Tst_msg_info.Origin_Msg_SN = Origin_Msg_SN;

	 USBComm_Get_Status(&Status);
	 rtc_test_msg.Tst_msg_info.Status.Total_Rcv_Lost_Msgs = Status.RX_Total_Lost_Msgs;
	 rtc_test_msg.Tst_msg_info.Status.Total_Rcv_Msg = Status.RX_Total_Msgs;
	 rtc_test_msg.Tst_msg_info.Status.Total_Rcv_data_size = Status.Rx_Total_Data_Size;
	 rtc_test_msg.Tst_msg_info.Status.Total_Tx_Lost_Msgs = Status.TX_Total_Buffer_Full_Msgs;
	 rtc_test_msg.Tst_msg_info.Status.Total_Rcv_data_size = Status.TX_Total_Msgs;
	 rtc_test_msg.Tst_msg_info.Fill_Size = Return_Size - sizeof(rtc_test_msg.Msg_Hdr) - sizeof(rtc_test_msg.Tst_msg_info);
     rtc_test_msg.Tst_msg_info.Busy_Time = 0;//dummy value in Host (RTC use Get_Total_Busy(1))
	 if (Return_Size > (int)(sizeof(rtc_test_msg)+sizeof(rtc_test_msg.Tst_msg_info)))
	 {
		 Return_Size = sizeof(rtc_test_msg);
		 rtc_test_msg.Tst_msg_info.Fill_Size = TEST_RTC_FILL_SIZE;
	 }

	 if (Return_Size < (int)(sizeof(rtc_test_msg)+sizeof(rtc_test_msg.Tst_msg_info)))
	 {
		 rtc_test_msg.Tst_msg_info.Fill_Size = 0;
	 }

	 //update Fill array
	 for  (index = 0; index < rtc_test_msg.Tst_msg_info.Fill_Size; index++)
	 {
		  rtc_test_msg.Fill[index] = 1 + index;
 	 }

	 msg_p = (USBComm_Buffer_t)&rtc_test_msg;
  	 msg_p    = &msg_p[sizeof(rtc_test_msg.Msg_Hdr)];    //msg_p point to the start of host test message info
  	 tmp_size =  sizeof(rtc_test_msg.Tst_msg_info) + rtc_test_msg.Tst_msg_info.Fill_Size;  		//size of the Message info and the fill array (for checksum calculation)
  	 rtc_test_msg.Tst_msg_info.Checksum = Msg_Prot_Checksum_calculator(msg_p, tmp_size);      	//calculate checksum on given message (when checksum field = 0)

    

  	 rtc_test_msg.Msg_Hdr.Size = Return_Size;
#ifdef ONS_HOST
	 //rtc_test_msg.Msg_Hdr.Time_Stamp = Debug_Get_Time_in_Milliseconds();
#endif
#ifdef ONS_RTC
	 rtc_test_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif
   
	res = USBComm_Send_Message( (USBComm_Buffer_t)&rtc_test_msg , rtc_test_msg.Msg_Hdr.Size );
   
	 return res;
 }


// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_RTC_Test_Message
//
//  Purpose				: unpack test message
//
//  Inputs				: Msg_Buff - pointer to the received message (without the generic header) .
//						: Msg_Size - the size of the received data.
//
//  Outputs				:Tst_info - all info from the header
//
//  Returns				: Success or Error code
//
//  Description			: Unpack message from Data_Buff validate the size
//						: and the checksum. send message to log/to screen.
//
// ---------------------------------------------------------------------------
int Msg_Prot_Unpack_RTC_Test_Message(ONS_BYTE* Msg_Buff, Msg_Prot_RTC_Tst_Hdr_t* Tst_Info)
{
	int 	msg_size;
	unsigned long calculated_checksum;
	Msg_Prot_RTC_Tst_Hdr_t* msg_p;
	unsigned long  tmp_checksum;

	msg_p = (Msg_Prot_RTC_Tst_Hdr_t*)Msg_Buff;
	msg_size = msg_p->Fill_Size + sizeof(Msg_Prot_RTC_Tst_Hdr_t);


	memcpy(Tst_Info, Msg_Buff, msg_size);

	// validate checksum
	tmp_checksum = msg_p->Checksum;

	calculated_checksum = Msg_Prot_Checksum_calculator(Msg_Buff, msg_size);      //calculate checksum on given message (when checksum field = 0)

	calculated_checksum -=  tmp_checksum;
	if (tmp_checksum != calculated_checksum)
	{
	//	Debug_Print_Error_Message_With_Code("Data checksum failed - Calculated checksum = ", calculated_checksum);
	//	return  MSG_PROT_ERROR_CHECKSUM_MESSAGE;
	}




	return MSG_PROT_SUCCESS;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Checksum_calculator
//
//  Purpose				: calculate the data checksum
//
//  Inputs				:
//
//  Outputs				: Data - the data to calculate CS on.
//						: Size - the size of data.
//
//  Returns				: Checksum value
//
//  Description			: Get data and size, sum up the character value
//
// ---------------------------------------------------------------------------
static unsigned long Msg_Prot_Checksum_calculator(ONS_BYTE* Data, unsigned int Size)
{
	unsigned int index;
	unsigned long sum;
	sum = 0;

	for (index= 0; index < Size; index++)
	{
		sum += Data[index];
	}
	return sum;
}



// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Get_Msg_SN
//
//  Purpose				: give the message serial number.
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				: Serial number
//
//  Description			: return the Serial number for Msg_Port messages
//
// ---------------------------------------------------------------------------
static unsigned long Msg_Prot_Get_Msg_SN(void)
{
	//int temp;
	gMsg_SN++;

	//temp = (int)gMsg_SN;
	//printf("SN = %d\n\n", temp);
	//scanf("%d", &Msg_SN);
	//printf("sizeof %d",sizeof( temp );

	return gMsg_SN;			// Message Index minimum value is 1
}


#ifdef TESTING_MODE
// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Get_Rx_Time
//
//  Purpose				: TESTING MODE - gives the Rcv_Time
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				: recived time
//
//  Description			: return the time the RTC test message arrived in
//
// ---------------------------------------------------------------------------
unsigned long Msg_Prot_Get_Rx_Time(void)
{
	unsigned long tmp = Rcv_Time;
	Rcv_Time = 0;
	return tmp;
}
#endif




// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Start_Bobbin_Changer
//
//  Purpose				: pack & transmit  Profile Info request from RTC
//
//  Inputs				: 
//
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			: send to RTC Profile Info request in order to initiate sending of Profile Info to Host
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Pack_n_Tx_Bobbin_Changer_Cmd(Msg_Prot_Bobbin_Changer_Command_Code_t Command)
{
	Msg_Prot_Bobbin_Changer_Cmd_Msg_t  bobbin_changer_command_msg;

	int res;


	bobbin_changer_command_msg.Msg_Hdr.Id = MSG_PROT_BOBBIN_CHANGER_CMD;

	bobbin_changer_command_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	bobbin_changer_command_msg.Msg_Hdr.Size = sizeof(Msg_Prot_Bobbin_Changer_Cmd_Msg_t);

	bobbin_changer_command_msg.Command.Command_Type = (INT_16)Command;

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	bobbin_changer_command_msg.Msg_Hdr.Time_Stamp = Tx_Time;
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_BOBBIN_CHANGER_CMD : Command type %d", bobbin_changer_command_msg.Command.Command_Type);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 93, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
	//	Log_Write_Msg(__FILE__, __func__, __LINE__, MSG_PROT_PROFILE_INFO_REQ, 0, 0, 0, 0, 0, 0);
#endif // ONS_HOST

#ifdef ONS_RTC
	bobbin_changer_command_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&bobbin_changer_command_msg, bobbin_changer_command_msg.Msg_Hdr.Size);

	return res;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Bobbin_Changer_Cmd
//
//  Purpose				: unpack bobbin changer command
//
//  Inputs				: Msg_Buff - pointer to the received message (without the generic header) .
//
//  Outputs				: Command - bobbin changer command code
//
//  Returns				: Success
//
//  Description			: 
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Unpack_Bobbin_Changer_Cmd(ONS_BYTE* Msg_Buff, Msg_Prot_Bobbin_Changer_Command_Code_t*  Command)
{

	Msg_Prot_Bobbin_Changer_Cmd_t*  msg_p;

	msg_p = (Msg_Prot_Bobbin_Changer_Cmd_t*)Msg_Buff;

	*Command = (Msg_Prot_Bobbin_Changer_Command_Code_t)msg_p->Command_Type;

	return MSG_PROT_SUCCESS;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_APF_Lock_State
//
//  Purpose				: pack & transmit Host req for Lock APF
//
//  Inputs				: 
//
//
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			:
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Pack_n_Tx_APF_Lock_State(Msg_Prot_APF_Lock_State State)
{

	Msg_Prot_APF_Lock_Msg_t   Lock_APF_msg;

	int res;

	Lock_APF_msg.Msg_Hdr.Id = MSG_PROT_LOCK_APF_CMD;

	Lock_APF_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	Lock_APF_msg.Msg_Hdr.Size = sizeof(Msg_Prot_APF_Lock_Msg_t);

	Lock_APF_msg.APF_Lock_State = State;
#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	Lock_APF_msg.Msg_Hdr.Time_Stamp = Tx_Time;
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send Msg_Prot_Pack_n_Tx_APF_Lock_State");
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 94, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
#endif // ONS_HOST


#ifdef ONS_RTC
	Lock_APF_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&Lock_APF_msg, Lock_APF_msg.Msg_Hdr.Size);

	return res;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_APF_Lock_State
//
//  Purpose				: unpack bobbin changer command
//
//  Inputs				: Msg_Buff - pointer to the received message (without the generic header) .
//
//  Outputs				: Command - bobbin changer command code
//
//  Returns				: Success
//
//  Description			: 
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Unpack_APF_Lock_State(ONS_BYTE* Msg_Buff, Msg_Prot_APF_Lock_State*  State)
{
	UINT_16* msg_p;

	msg_p = (UINT_16*)Msg_Buff;

	*State = (Msg_Prot_APF_Lock_State)*msg_p;

	return MSG_PROT_SUCCESS;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_RTC_Reset_Cmd
//
//  Purpose				: Send RTC reset command at termination
//
//  Inputs				: Msg_Buff - pointer to the received message (without the generic header) .
//
//  Outputs				: Command - bobbin changer command code
//
//  Returns				: Success
//
//  Description			:
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Pack_n_Tx_RTC_Reset_Cmd(void)
{

	Msg_Prot_RTC_Reset_Cmd_Msg_t  rtc_reset_cmd_msg;

	int res;

	rtc_reset_cmd_msg.Msg_Hdr.Id 				= MSG_PROT_RTC_RESET_CMD;

	rtc_reset_cmd_msg.Msg_Hdr.Serial_Number 		= Msg_Prot_Get_Msg_SN();

	rtc_reset_cmd_msg.Msg_Hdr.Size 				= sizeof(rtc_reset_cmd_msg.Msg_Hdr);

#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN
	rtc_reset_cmd_msg.Msg_Hdr.Time_Stamp = Tx_Time;
	//char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	//snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_PERFORM_INIT_REQ");
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 95, __FILENAME__, __LINE__, __func__, (char*)"Send MSG_PROT_PERFORM_INIT_REQ");
	//ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
#endif // ONS_HOST

#ifdef ONS_RTC
		rtc_reset_cmd_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

		res = USBComm_Send_Message( (USBComm_Buffer_t)&rtc_reset_cmd_msg , rtc_reset_cmd_msg.Msg_Hdr.Size );

		return res;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Rotate_SH_to_Direction
//
//  Purpose				: Send RTC rotate to direction command 
//
//  Inputs				: RPM - for rotation speed
//						  Direction - CW/CCW
//						  Distance - rotate distance in steps
//
//  Outputs				: 
//
//  Returns				: Success
//
//  Description			:
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Pack_n_Tx_Rotate_SH_to_Direction(int RPM, Msg_Prot_SH_Rotation_Direction_t Direction, int Distance)
{
	Msg_Prot_Rotate_SH_To_Dir_Msg_t  rotate_sh_to_dir_cmd_msg;

	int res;

	rotate_sh_to_dir_cmd_msg.Msg_Hdr.Id = MSG_PROT_ROTATE_SH_TO_DIRECTION_CMD;

	rotate_sh_to_dir_cmd_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	rotate_sh_to_dir_cmd_msg.Msg_Hdr.Size = sizeof(Msg_Prot_Rotate_SH_To_Dir_Msg_t);

	rotate_sh_to_dir_cmd_msg.Data.RPM = RPM;
	
	rotate_sh_to_dir_cmd_msg.Data.Direction = Direction;

	rotate_sh_to_dir_cmd_msg.Data.Distance = Distance;

#ifdef ONS_HOST
	unsigned long Tx_Time;
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];

#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN
	rotate_sh_to_dir_cmd_msg.Msg_Hdr.Time_Stamp = Tx_Time;
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_PERFORM_INIT_REQ");
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, 95, __FILENAME__, __LINE__, __func__, (char*)"Send MSG_PROT_PERFORM_INIT_REQ");
#endif // ONS_HOST

#ifdef ONS_RTC
	rotate_sh_to_dir_cmd_msg.Msg_Hdr.Time_Stamp = SysTimer_GetTimeInMiliSeconds();
#endif

	res = USBComm_Send_Message((USBComm_Buffer_t)&rotate_sh_to_dir_cmd_msg, rotate_sh_to_dir_cmd_msg.Msg_Hdr.Size);

	return res;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_Rotate_SH_to_Direction
//
//  Purpose				: Send RTC rotate to direction command 
//
//  Inputs				: Msg_Buff
//						  
//  Outputs				: RPM - for rotation speed
//						  Direction - CW/CCW
//						  Distance - rotate distance in steps
//
//  Returns				: Success
//
//  Description			: RTC need to use the rpm, direction and distance in order to rotate the SewHead (Z Servo).
//
// ---------------------------------------------------------------------------
int	Msg_Prot_Unpack_Rotate_SH_to_Direction(ONS_BYTE* Msg_Buff, int* RPM, Msg_Prot_SH_Rotation_Direction_t* Direction, int* Distance)
{

	Msg_Prot_Rotate_SH_To_Dir_t*  msg_p;

	msg_p = (Msg_Prot_Rotate_SH_To_Dir_t*)Msg_Buff;

	*RPM = msg_p->RPM;

	*Direction = (Msg_Prot_SH_Rotation_Direction_t)msg_p->Direction;

	*Distance = msg_p->Distance;

	return MSG_PROT_SUCCESS;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_RTC_Diag_Act_Mode
//
//  Purpose				: pack & transmit Host Diag_Act mode
//
//  Inputs				:Diag_Act_Mode - the Diag_Act current Diag_Act mode
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			:
//
// ---------------------------------------------------------------------------
/*
int	Msg_Prot_Pack_n_Tx_Diag_Act_Mode(Msg_Prot_Diag_Act_Mode_t Diag_Act_Mode, int Direction_Logic, int Sensor_Logic, int Move_Speed, int Move_Distance, int Move_To_Home_Axis_Timeout)
{

	Msg_Prot_Diag_Act_Mode_Msg_t  Diag_Act_Mode_Data_Msg;
	int							  res;

	Diag_Act_Mode_Data_Msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	Diag_Act_Mode_Data_Msg.Msg_Hdr.Id = MSG_PROT_DIAG_ACT_MODE;

	Diag_Act_Mode_Data_Msg.Msg_Hdr.Size = sizeof(Msg_Prot_Diag_Act_Mode_Msg_t);

	Diag_Act_Mode_Data_Msg.Diag_Act_Mode_Data.Diag_Act_Mode = Diag_Act_Mode;
	Diag_Act_Mode_Data_Msg.Diag_Act_Mode_Data.Direction_Logic = Direction_Logic;
	Diag_Act_Mode_Data_Msg.Diag_Act_Mode_Data.Sensor_Logic = Sensor_Logic;
	Diag_Act_Mode_Data_Msg.Diag_Act_Mode_Data.Move_Distance = Move_Distance;
	Diag_Act_Mode_Data_Msg.Diag_Act_Mode_Data.Move_Speed = Move_Speed;
	Diag_Act_Mode_Data_Msg.Diag_Act_Mode_Data.Move_To_Home_Axis_Timeout = Move_To_Home_Axis_Timeout;
#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	Diag_Act_Mode_Data_Msg.Msg_Hdr.Time_Stamp = Tx_Time;
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_DIAG_ACT_MODE  MODE = %d", Diag_Act_Mode_Data_Msg.Diag_Act_Mode_Data.Diag_Act_Mode);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, __LINE__, __FILENAME__, __LINE__, __func__, msg_txt);
#endif // ONS_HOST

	res = USBComm_Send_Message((USBComm_Buffer_t)&Diag_Act_Mode_Data_Msg, Diag_Act_Mode_Data_Msg.Msg_Hdr.Size);

	return res;
}
*/
int	Msg_Prot_Pack_n_Tx_Diag_Act_Mode(Msg_Prot_Diag_Act_Mode_t Diag_Act_Mode, int Direction_Logic, int Move_Speed, int Move_Distance, int Move_To_Home_Axis_Timeout)
{

	Msg_Prot_Diag_Act_Mode_Msg_t  Diag_Act_Mode_Data_Msg;
	int							  res;

	Diag_Act_Mode_Data_Msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	Diag_Act_Mode_Data_Msg.Msg_Hdr.Id = MSG_PROT_DIAG_ACT_MODE;

	Diag_Act_Mode_Data_Msg.Msg_Hdr.Size = sizeof(Msg_Prot_Diag_Act_Mode_Msg_t);

	Diag_Act_Mode_Data_Msg.Diag_Act_Mode_Data.Diag_Act_Mode = Diag_Act_Mode;
	Diag_Act_Mode_Data_Msg.Diag_Act_Mode_Data.Direction_Logic = Direction_Logic;
	Diag_Act_Mode_Data_Msg.Diag_Act_Mode_Data.Move_Distance = Move_Distance;
	Diag_Act_Mode_Data_Msg.Diag_Act_Mode_Data.Move_Speed = Move_Speed;
	Diag_Act_Mode_Data_Msg.Diag_Act_Mode_Data.Move_To_Home_Axis_Timeout = Move_To_Home_Axis_Timeout;
#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	Diag_Act_Mode_Data_Msg.Msg_Hdr.Time_Stamp = Tx_Time;
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Send MSG_PROT_DIAG_ACT_MODE  MODE = %d", Diag_Act_Mode_Data_Msg.Diag_Act_Mode_Data.Diag_Act_Mode);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, __LINE__, __FILENAME__, __LINE__, __func__, msg_txt);
#endif // ONS_HOST

	res = USBComm_Send_Message((USBComm_Buffer_t)&Diag_Act_Mode_Data_Msg, Diag_Act_Mode_Data_Msg.Msg_Hdr.Size);

	return res;
}
// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_Diag_Act_Mode
//
//  Purpose				: unpack
//
//  Inputs				: Msg_Buff - pointer to the received message (without the generic header) .
//
//  Outputs				: 
//
//  Returns				: Success
//
//  Description			:
//
// ---------------------------------------------------------------------------
int Msg_Prot_Unpack_Diag_Act_Mode(ONS_BYTE* Msg_Buff, Msg_Prot_Diag_Act_Mode_t* Diag_Act_Mode, int* Direction_Logic, int* Move_Speed, int* Move_Distance, int* Move_To_Home_Axis_Timeout)
{
	Msg_Prot_Diag_Act_Mode_Data_t* msg_p;

	msg_p = (Msg_Prot_Diag_Act_Mode_Data_t*)Msg_Buff;

	*Diag_Act_Mode			= (Msg_Prot_Diag_Act_Mode_t)msg_p->Diag_Act_Mode;
	*Direction_Logic		= (int)msg_p->Direction_Logic;
	*Move_Distance			= (int)msg_p->Move_Distance;
	*Move_Speed				= (int)msg_p->Move_Speed;
	*Move_To_Home_Axis_Timeout = (int)msg_p->Move_To_Home_Axis_Timeout;

	return MSG_PROT_SUCCESS;
}


// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Pack_n_Tx_RTC_Diag_Act_Status
//
//  Purpose				: pack & transmit
//
//  Inputs				:
//  Outputs				:
//
//  Returns				: success or error code
//
//  Description			:
//
// ---------------------------------------------------------------------------
int Msg_Prot_Pack_n_Tx_RTC_Diag_Act_Status(RTC_State_t	RTC_Diag_Act_Mode, int X_Pos, int Y_Pos, int XY_Moving, int Sew_Head_Rotating, RTC_Diag_Act_Result_t Diag_Act_Res, int Error_Code)
{
	Msg_Prot_RTC_Diag_Act_Status_Msg_t  RTC_diag_act_status_msg;

	int res;

	RTC_diag_act_status_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();

	RTC_diag_act_status_msg.Msg_Hdr.Id = MSG_PORT_RTC_DIAG_ACT_STATUS;

	RTC_diag_act_status_msg.Msg_Hdr.Size = sizeof(Msg_Prot_RTC_Diag_Act_Status_Msg_t);

	RTC_diag_act_status_msg.RTC_Status.X_pos = X_Pos;

	RTC_diag_act_status_msg.RTC_Status.Y_pos = Y_Pos;

	RTC_diag_act_status_msg.RTC_Status.RTC_Diag_Act_Mode = RTC_Diag_Act_Mode;

	RTC_diag_act_status_msg.RTC_Status.Sew_Head_Rotating = Sew_Head_Rotating;

	RTC_diag_act_status_msg.RTC_Status.Diag_Act_Res = Diag_Act_Res;

	RTC_diag_act_status_msg.RTC_Status.XY_Moving = XY_Moving;

	RTC_diag_act_status_msg.RTC_Status.Error_Code = Error_Code;
#ifdef ONS_HOST
	unsigned long Tx_Time;
#ifdef ONS_HOST_LINUX
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
#else //ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Tx_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_WIN 
	RTC_diag_act_status_msg.Msg_Hdr.Time_Stamp = Tx_Time;
#endif // ONS_HOST

	res = USBComm_Send_Message((USBComm_Buffer_t)&RTC_diag_act_status_msg, RTC_diag_act_status_msg.Msg_Hdr.Size);
	return res;
}

// ---------------------------------------------------------------------------
//  Function name		: Msg_Prot_Unpack_RTC_Diag_Act_status
//
//  Purpose				: unpack
//
//  Inputs				: Msg_Buff - pointer to the received message (without the generic header) .
//
//  Outputs				:
//
//  Returns				: Success
//
//  Description			:
//
// ---------------------------------------------------------------------------
int  	Msg_Prot_Unpack_RTC_Diag_Act_Status(ONS_BYTE* Msg_Buff, RTC_State_t* RTC_Diag_Act_Mode, int* X_Pos, int* Y_Pos, int* XY_Moving, int* Sew_Head_Rotating, RTC_Diag_Act_Result_t* Diag_Act_Res, int* Error_Code)
{
	Msg_Prot_RTC_Diag_Act_Status_t* msg_p;

	msg_p = (Msg_Prot_RTC_Diag_Act_Status_t*)Msg_Buff;

	*RTC_Diag_Act_Mode = (RTC_State_t)msg_p->RTC_Diag_Act_Mode;
	*X_Pos = msg_p->X_pos;
	*Y_Pos = msg_p->Y_pos;
	*XY_Moving = msg_p->XY_Moving;
	*Sew_Head_Rotating = msg_p->Sew_Head_Rotating;
	*Diag_Act_Res = (RTC_Diag_Act_Result_t)msg_p->Diag_Act_Res;
	*Error_Code = msg_p->Error_Code;
#ifdef ONS_HOST
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Receive MSG_PORT_RTC_DIAG_ACT_STATUS : RTC_Diag_Act_Mode =%d,  X_Pos=%d,  Y_Pos=%d,  XY_Moving=%d, Sew_Head_Rotating=%d,  Diag_Act_Res=%d,  Error_Code=%d", *RTC_Diag_Act_Mode, *X_Pos, *Y_Pos, *XY_Moving, *Sew_Head_Rotating, *Diag_Act_Res, *Error_Code);
	//ONS_Log.Update_Log_Data(LOG_INFO_LEVEL, LOG_CATEGORY_RTC_COMM, __LINE__, __FILENAME__, __LINE__, __func__, msg_txt);
#endif
	return MSG_PROT_SUCCESS;
}

int Msg_Prot_Pack_Entity_OneShot( int Entity_Id, int millisec )
{
	int res;
	Msg_Prot_Entity_OneShot_Msg_t       entity_one_shot_msg;
	entity_one_shot_msg.Msg_Hdr.Serial_Number = Msg_Prot_Get_Msg_SN();
	entity_one_shot_msg.Msg_Hdr.Id       = MSG_PROT_ENTITY_ONESHOT;
	entity_one_shot_msg.Msg_Hdr.Size     = sizeof(Msg_Prot_Entity_OneShot_Msg_t);
	entity_one_shot_msg.Data.EntityId    = Entity_Id;
	entity_one_shot_msg.Data.OneShotTime = millisec;

	unsigned long Tx_Time;
	struct timeval  	tv;
	gettimeofday(&tv, NULL);
	Tx_Time = ((1000000 * (tv.tv_sec % 1000)) + tv.tv_usec);
	entity_one_shot_msg.Msg_Hdr.Time_Stamp = Tx_Time;

	res = USBComm_Send_Message((USBComm_Buffer_t)&entity_one_shot_msg, entity_one_shot_msg.Msg_Hdr.Size);
	return res;
}

int Msg_Prot_Unpack_Entity_OneShot_Fbk(ONS_BYTE* Msg_Buff, char* status )
{
	Msg_Prot_Entity_OneShot_Fbk_t* msg_p;
	msg_p = (Msg_Prot_Entity_OneShot_Fbk_t*)Msg_Buff;

	*status = msg_p->status;
	return MSG_PROT_SUCCESS;
}
