// ---------------------------------------------------------------------------
//  Filename: SyslogMgr.h
//  Created by: Roey Huberman
//  Date:  20/01/16
//  Orisol (c)
// ---------------------------------------------------------------------------

#ifndef _SYSLOGMGR_H_
#define _SYSLOGMGR_H_

#define LOG_FOLDER_PATH "/var/log/orisol"

typedef enum
{
	SyslogMgr_RC_OK								= 0,
	SyslogMgr_RC_FILE_OPEN_ERROR				= 1,
	SyslogMgr_RC_FILE_WRITE_ERROR				= 2
}
	SyslogMgr_RC_t;


SyslogMgr_RC_t SyslogMgr_Init( void );
void SyslogMgr_Get_Log_File_Name( char* Log_Name );
SyslogMgr_RC_t SyslogMgr_Main( void );

#endif // _SYSLOGMGR_H_
