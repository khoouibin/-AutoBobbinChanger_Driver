// ---------------------------------------------------------------------------
//  Filename: ONS_Syslog.h
//  Created by: Roey Huberman
//  Date:  2/02/16
//  Orisol (c)
// ---------------------------------------------------------------------------
#ifndef _ONS_SYSLOG_H_
#define _ONS_SYSLOG_H_

#define MAX_SYSLOG_DESC_SIZE 256 //size of description string
#define SYSLOG_ORIGION_FILE_NAME_LENGTH 30
#define SYSLOG_ORIGION_FUNC_NAME_LENGTH 50
#define LOG_NAME_BUFF_SIZE 256

typedef enum
{
	ONS_Syslog_RC_OK						= 0,
	ONS_Syslog_RC_INIT_FAILED				= 1,
	ONS_Syslog_RC_INPUT_DATA_ERROR			= 2,
	ONS_Syslog_RC_PUT_DATA_ERROR			= 3
}
	ONS_Syslog_RC_t;


typedef enum
{
	SB_SEVERITY_EMERG                       = 0,
	SB_SEVERITY_ALERT                  	    = 1,
	SB_SEVERITY_CRIT						= 2,
	SB_SEVERITY_ERR							= 3,
	SB_SEVERITY_WARNING						= 4,
	SB_SEVERITY_NOTICE						= 5,//default severity in ini file
	SB_SEVERITY_INFO						= 6,
	SB_SEVERITY_DEBUG						= 7
}
	SyslogBuff_Severity_t;


ONS_Syslog_RC_t ONS_Syslog_Init( void );
ONS_Syslog_RC_t ONS_Syslog_Add_Msg( SyslogBuff_Severity_t Msg_Severity , char* Msg_Data );
ONS_Syslog_RC_t ONS_Syslog_Send_Msg( SyslogBuff_Severity_t Msg_Severity , char* File_Name , unsigned int Line , char* Func_Name , char* Msg_Data );
int ONS_Syslog_Get_Last_Error_Code( void );
int ONS_Syslog_Get_Retry_Counter( void );
SyslogBuff_Severity_t ONS_Syslog_Get_Severity_Level( void );
void ONS_Syslog_Set_Severity_Level( SyslogBuff_Severity_t Level );
void ONS_Syslog_Close_Request( void );
unsigned long ONS_Syslog_Get_Send_Messages_Counter( void );
unsigned long ONS_Syslog_Get_Printed_Messages_Counter( void );
void ONS_Syslog_Get_Log_Name( char* Log_Name );
#endif //_ONS_SYSLOG_H_
