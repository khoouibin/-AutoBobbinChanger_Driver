// ---------------------------------------------------------------------------
//  Filename: Debug.h
//  Created by: Nissim Avichail
//  Date:  30/06/15
//  Orisol (c)
// ---------------------------------------------------------------------------

#ifndef _DEBUG_H_
#define _DEBUG_H_

#ifdef __cplusplus
extern "C" {
#endif



#define DEBUG_MAX_MSG_SIZE		256


#define DO_NOT_RECORD				0
#define RECORD_ALL_MESSAGES			1
#define RECORD_NEW_MESSAGES			2

#ifndef USE_SYSLOG
//#define SB_SEVERITY_EMERG  0
//#define SB_SEVERITY_ALERT  1
//#define SB_SEVERITY_CRIT  2
//#define SB_SEVERITY_ERR  3
//#define SB_SEVERITY_WARNING  4
//#define SB_SEVERITY_NOTICE  5//default severity in ini file
//#define SB_SEVERITY_INFO  6
//#define SB_SEVERITY_DEBUG  7
typedef enum
{
	SB_SEVERITY_EMERG = 0,
	SB_SEVERITY_ALERT = 1,
	SB_SEVERITY_CRIT = 2,
	SB_SEVERITY_ERR = 3,
	SB_SEVERITY_WARNING = 4,
	SB_SEVERITY_NOTICE = 5,//default severity in ini file
	SB_SEVERITY_INFO = 6,
	SB_SEVERITY_DEBUG = 7
}
	SyslogBuff_Severity_t;

#define	MAX_SYSLOG_DESC_SIZE 256

#endif // !USE_SYSLOG

// ----------------
//   Prototypes
//------------------

//void Debug_Print_Error_Message_With_Code(const char* String, int Code);    
    
//void Debug_Print_Debug_Message_With_Param ( const char* String , int Param );
unsigned long Debug_Get_Time_in_Milliseconds ( void );
int Log_Init(char* File_Path, char* File_Name_Extntion);
void Log_Write_Error(const char* File_Name, const char* Func_Name, int Line_Number, int Error_Code,const char* Error_String);
void Log_Write_Error_USB(const char* File_Name, const char* Func_Name, int Line_Number, int Error_Code,const char* Error_String);

void Log_Dump_RTC_Data( unsigned int Type, unsigned int Time, unsigned int Data );
//void Log_Write_Msg(const char* File_Name, const char* Func_Name, int Line_Number, int Msg_ID,int Data_1, int Data_2, int Data_3, int Data_4, int Data_5, int Data_6);
//void Log_Write_String_Msg(const char* File_Name, const char* Func_Name, int Line_Number, int Msg_ID,char *String);
//void Log_Write_Str(const char *Str);
void Log_Set_Dump_Type_Value(int Type);
int	 Debug_Get_Syslog_Level(void);
void Debug_Set_Syslog_Level(int Level);
void Debug_Set_Debug_Level_Exist(int Exist);
int  Debug_Get_Debug_Level_Exist(void);

//void Trace_Log(int Ev_Num);
#ifndef USE_SYSLOG
void ONS_Syslog_Send_Msg(SyslogBuff_Severity_t Msg_Severity, const char* File_Name, int Line, const char* Func_Name, char* Msg_Data);
#endif
void Print_To_Screen(const char * data);

#ifdef __cplusplus
}
#endif
#endif  //_DEBUG_H_
