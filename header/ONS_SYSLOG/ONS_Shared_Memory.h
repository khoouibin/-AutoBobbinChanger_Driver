// ---------------------------------------------------------------------------
//  Filename: ONS_Shared_Memory.h
//  Created by: Roey Huberman
//  Date:  20/01/16
//  Orisol (c)
// ---------------------------------------------------------------------------

#ifndef _ONS_SHARED_MEMORY_H_
#define _ONS_SHARED_MEMORY_H_

#define SYSBUFF_SIZE 10000		// size of buffer


typedef struct
{
	char 								time_stamp[ 13 ]; //HH:MM:SS:mmm
	SyslogBuff_Severity_t				severity;
	//char								origion_file_name[ SYSLOG_ORIGION_FILE_NAME_LENGTH ];
	//unsigned int						line_number;
	//char 								origion_func_name[ SYSLOG_ORIGION_FUNC_NAME_LENGTH ];
	char 								data[ MAX_SYSLOG_DESC_SIZE ];
}
	SysMsg_t;

typedef struct
{
  sem_t 								mutex;
  SysMsg_t 								SyslogBuff_Buffer[ SYSBUFF_SIZE ];	// buffer storage
  int 									SyslogBuff_Head;
  int 									SyslogBuff_Tail;
  int 									SyslogBuff_Full_Flag;
  int 									SyslogBuff_Overflow_Flag;
  int 									SyslogBuff_Close_Request;
  unsigned long 						messages_counter;
  char 									log_name[ LOG_NAME_BUFF_SIZE ];
}
	ONS_Syslog_Shared_Memory_t;

typedef enum
{
	ONS_Syslog_SM_RC_OK					= 0,
	ONS_Syslog_SM_RC_FILE_OPEN_ERR		= 1,
	ONS_Syslog_SM_RC_FILE_WRITE_ERR		= 2,
	ONS_Syslog_SM_RC_LSEEK_ERR			= 3,
	ONS_Syslog_SM_RC_MMAP_ERR			= 4,
	ONS_Syslog_SM_RC_SCH_ERR			= 5 //scheduler error
}
	ONS_Syslog_SM_RC_t;

int	ONS_Shared_Memory_Init( void );
int	ONS_Shared_Memory_Free( void );
#endif //_ONS_SHARED_MEMORY_H_
