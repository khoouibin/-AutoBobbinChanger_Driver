// ---------------------------------------------------------------------------
//  Filename: SyslogBuff.h
//  Created by: Roey Huberman
//  Date:  20/01/16
//  Orisol (c) 
// ---------------------------------------------------------------------------


#ifndef _SYSLOGBUFF_H_
#define _SYSLOGBUFF_H_

#include "ONS_Shared_Memory.h"

// SyslogBuff Module Error Code Definition
typedef enum
{
	SB_SUCCESS                             = 0,
	SB_ERROR_BUFFER_FULL                   = 1,
	SB_ERROR_BUFFER_EMPTY                  = 2,
	SB_SEMAPHORE_FAILED	                   = 3

}
	SyslogBuff_Error_Code_t;


int SyslogBuff_Init ( ONS_Syslog_Shared_Memory_t* Shared_Memory );
int SyslogBuff_Put_SysMsg ( SysMsg_t* SysMsg_Data );
int SyslogBuff_Pull_SysMsg ( SysMsg_t* SysMsg_Data );
void SyslogBuff_Reset ( void );
int SyslogBuff_Get_Messages_Number ( void );
int SyslogBuff_Is_Empty ( void );
int SyslogBuff_Is_Full ( void );
int SyslogBuff_Is_Overflow_Occurred ( void );
void SyslogBuff_Reset_Overflow_Flag ( void );
void SyslogBuff_Set_Close_Request ( void );
int SyslogBuff_Get_Close_Request ( void );
unsigned long SyslogBuff_Get_Messages_Counter( void );
void SyslogBuff_Get_Log_Name( char* Log_Name );
ONS_Syslog_Shared_Memory_t* SyslogBuff_Get_SM_Ptr( void );
#endif	// _SYSLOGBUFF_H_
