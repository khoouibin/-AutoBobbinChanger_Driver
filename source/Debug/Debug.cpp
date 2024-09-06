// ---------------------------------------------------------------------------
//  Filename: Debug.c
//  Created by: Nissim Avichail
//  Date:  30/06/15
//  Orisol (c)
// ---------------------------------------------------------------------------
#define _CRT_SECURE_NO_WARNINGS


#include <stdio.h>

#include <stdint.h>
#include "Definition.h"
#include "ONS_General.h"
#include "RTC_Stitch.h"

#ifdef ONS_WIN
#include <windows.h>
#endif

#ifdef ONS_PC_PLAT
#include <stdio.h>
#include <string.h>

#include "ONS_Debug.h"
#include "USBComm.h"
#include "Msg_Prot.h"
#include "UI_Prot.h"
#include "Control.h"
#endif
#include "UI_Comm.h"
#include <stdlib.h>

#ifdef ONS_HOST_LINUX
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "SM_Comm_IPC.h"

#ifdef USE_SYSLOG
#include "ONS_Syslog.h"				//ONS_SYSLOG
#include "SyslogMgr.h"
#endif //#ifdef USE_SYSLOG

#endif
#include "pdc_def.h"
#include "PDC_Control.h"
#include "ONS_Log.h"
#include "Machine_Mngr.h"
#include "IST_Def.h"
#include "IStitching_Control.h"

#ifdef ONS_WIN

#define __func__ __FUNCTION__

#endif

#define  TRACE_LOG_BUFF			5000000

typedef struct
{
	unsigned long long Time_Tag;
	int				   Event_Num;
}
	Trace_To_Log_t;


static FILE* 						Dump;						//Testing
#ifndef USE_SYSLOG
static FILE* 						Sys;						//Testing
#endif
static int							Syslog_Level;
static int							Debug_Level_Exist;
//static Trace_To_Log_t				Trace_Buff[TRACE_LOG_BUFF];
//static int							Trace_Buff_Index;
static	char						Log_Folder_Name[256];
//void Print_Trace_Log(void);
int Teminate_Syslog(void);


int Log_Init (char* File_Path, char* File_Name_Extntion)
{
	char	full_file_2_name[255];
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
#ifndef USE_SYSLOG
	char	full_file_1_name[255];
#endif

	//Trace_Buff_Index = 0;
	sprintf(Log_Folder_Name,"%s",File_Path);
#ifdef ONS_WIN
	sprintf(full_file_1_name, "%s\\%s_%s.txt", File_Path, "System_log", File_Name_Extntion);
	sprintf(full_file_2_name, "%s\\%s_%s.txt", File_Path, "RTC_Dump_log", File_Name_Extntion);
#else
#ifndef USE_SYSLOG
	sprintf(full_file_1_name, "%s/%s_%s.txt", File_Path, "System_log", File_Name_Extntion);
#endif
	sprintf(full_file_2_name, "%s/%s_%s.txt", File_Path, "RTC_Dump_log.txt", File_Name_Extntion);
#endif
	Syslog_Level = SB_SEVERITY_NOTICE;
	Debug_Level_Exist = ONS_FALSE;
//Open file for log data
#ifndef USE_SYSLOG
	if((Sys=fopen(full_file_1_name,"w+")) == NULL)
	{
		snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Unable to create file %s \n", full_file_1_name);
		Print_To_Screen(msg_txt);

		return(-1);
	}
#endif
	if((Dump=fopen(full_file_2_name,"w+")) == NULL)
	{
		snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, " Unable to create file %s \n", full_file_2_name);
		Print_To_Screen(msg_txt);
		Log_Write_Error(__FILE__, __func__, __LINE__, -1 ," Unable to create file RTC_Dump_log.txt");

		return(-1);
	}

	fprintf (Dump,"TIME     Type     Data\n");

	return 0;
}

// ---------------------------------------------------------------------------
//  Function name		: Debug_Print_Error_Message_With_Code
//
//  Purpose				: print Error_message with error code.
//
//  Inputs				: String 	- output text
//						: Code		- error code
//
//  Outputs				:
//						:
//  Returns				:
//
//  Description			: print Error_message with error code.
//
// ---------------------------------------------------------------------------
//void Debug_Print_Error_Message_With_Code ( const char* String , int Code )
//{
//#ifdef ONS_PC_PLAT
//	char			msg_txt[MAX_SYSLOG_DESC_SIZE];/
//
//	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "ERROR --> %s error code = %d  \n", String, Code);
//	ONS_Syslog_Send_Msg(SB_SEVERITY_ERR, __FILENAME__, __LINE__, __func__, msg_txt);
//	printf( "ERROR --> %s error code = %d  \n", String , Code );
//#endif
//}

#ifdef ONS_PC_PLAT
// ---------------------------------------------------------------------------
//  Function name		: Debug_Print_Debug_Message_With_Param
//
//  Purpose				: print Debug message with one parameter.
//
//  Inputs				: String 	- output text
//						: Param		- print Parameter
//
//  Outputs				:
//						:
//  Returns				:
//
//  Description			: print  Debug message with one parameter.
//
// ---------------------------------------------------------------------------
//void Debug_Print_Debug_Message_With_Param ( const char* String , int Param )
//{
//	
//	char			msg_txt[MAX_SYSLOG_DESC_SIZE];
//	//printf( "%s %d  \n",String,Param);
//	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "%s %d  \n", String, Param);
//	ONS_Syslog_Send_Msg(SB_SEVERITY_ERR, __FILENAME__, __LINE__, __func__, msg_txt);
//}


// ---------------------------------------------------------------------------
//  Function name		: Debug_Get_Time_in_Milliseconds
//
//  Purpose				: return time of day in milliseconds.
//
//  Inputs				:
//
//  Outputs				:
//
//  Returns				: Time of day in Milliseconds
//
//  Description			: Get time of day and change the units to milliseconds
//						  and return it as unsigned long.
//
// ---------------------------------------------------------------------------
unsigned long Debug_Get_Time_in_Milliseconds( void )
{
#ifdef ONS_HOST_LINUX
	struct timeval 	tv;

	gettimeofday(&tv,NULL);

	return (tv.tv_sec*1000 + tv.tv_usec/1000);			//change units to Milliseconds

#endif // ONS_HOST_LINUX

#ifdef ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	return( (time.wSecond * 1000) + time.wMilliseconds);

#endif // ONS_HOST_WIN


}
#endif



// ---------------------------------------------------------------------------
//  Function name		: Write_Error_To_Log
//
//  Purpose				: write error to log.
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
void Log_Write_Error(const char* File_Name, const char* Func_Name, int Line_Number, int Error_Code,const char* Error_String)
{
	//unsigned long sys_time;
	if (Control_Is_Shot_Down_In_Progress())
	{
		return;
	}
//	sys_time = Debug_Get_Time_in_Milliseconds();
	 
#ifdef ENABLE_WRITE_ERROR_TO_LOG
	unsigned long sys_time;

	sys_time = Debug_Get_Time_in_Milliseconds();
	fprintf (Sys,"%lu	%s	%s	%d : ERROR!!!  code: %d, %s  \n",sys_time, File_Name, Func_Name, Line_Number,Error_Code, Error_String);
#endif

//#ifdef ENABLE_PRINT_TO_SCREEN
	unsigned long sys_time1;

	sys_time1 = Debug_Get_Time_in_Milliseconds();
	printf ("HOST : %lu	%s	%s	%d : ERROR!!!  code: %d, %s  \n",sys_time1, File_Name, Func_Name, Line_Number,Error_Code, Error_String);
//#endif

//Print_Trace_Log();
#ifdef ENABLE_TERMINET_ON_ERROR

char			msg_txt[MAX_SYSLOG_DESC_SIZE];
int				res;

snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Need To Terminate Host - %s code %d %s(%d)", Error_String, Error_Code, Func_Name, Line_Number);
ONS_Log.Update_Log_Data(LOG_CRITICAL_LEVEL, LOG_CATEGORY_GENERAL, 1, __FILENAME__, __LINE__, __func__, (char*)"Need To Terminate Host - Error_String");

ONS_Log.Update_Log_Data(LOG_CRITICAL_LEVEL, LOG_CATEGORY_GENERAL, 1, __FILENAME__, __LINE__, __func__, (char*)"Need To Terminate Host - Error_String");

Control_Set_System_HW_To_Termination_Mode();
if (PDC_Ctrl.Is_PDC_Exist() || IST_Ctrl.Is_IST_Exist())
{
	if (Sew_Ctrl.Is_Host_Version_Sent_To_PDC())
	{
		PDC_Ctrl.Add_Event_Report(PDC_CTRL_POWER_OFF_EVENT, "");
	}
	PDC_Ctrl.Update_Events();
	res = PDC_Ctrl.Save_And_send_Event_File(ONS_TRUE, ONS_TRUE);

	//res = PDC_Ctrl.Save_And_send_Event_File(ONS_TRUE);
}

//res = Msg_Prot_Pack_n_Tx_RTC_Reset_Cmd();
#ifdef ONS_LINUX
usleep(2);
#endif

if (Machine_Mngr.Is_Exit_To_Terminal_Enable())
{
	Control_Handle_Error(POP_UP_MSG_SHUT_DOWN_WARNING, ONS_FALSE, (char*) "");
}
else
{
//	res = Msg_Prot_Pack_n_Tx_RTC_Reset_Cmd();
//#ifdef ONS_LINUX
//	usleep(2);
//#endif
	UI_Prot_Pack_n_Tx_Pop_Up_Req(UI_POP_UP_EMPTY_ID, POP_UP_MSG_SHUT_DOWN_WARNING, (char*) "");
	while (1)
	{
		Control_Handle_Incoming_Msg();
	}
}

res = Msg_Prot_Pack_n_Tx_RTC_Reset_Cmd();
#ifdef ONS_LINUX
usleep(2);
#endif

#ifdef USE_SYSLOG
Teminate_Syslog();

#else
fflush(Sys);
#endif
#ifdef ONS_LINUX

if (PDC_Ctrl.Is_PDC_Exist())
{
	usleep(300000);
	PDC_Ctrl.Send_Terminate_PDC();
	sleep(3);
	PDC_Ctrl.Free_PDC_Memory();
}
#else
fflush(Sys);
#endif

res = UI_Prot_Pack_n_Tx_Terminate_UI();		//Send Terminate message to UI 
printf( "HOST : UI_Prot_Pack_n_Tx_Terminate_UI (res = %d) \n",res);
snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "UI_Prot_Pack_n_Tx_Terminate_UI (res = %d)\n",res);
//ONS_Syslog_Send_Msg(SB_SEVERITY_ERR, __FILENAME__, __LINE__, __func__, msg_txt);
ONS_Log.Update_Log_Data(LOG_CRITICAL_LEVEL, LOG_CATEGORY_GENERAL, 2, __FILENAME__, __LINE__, __func__, msg_txt);
UI_Comm_Termination_Msg_Sent();

#ifdef ONS_LINUX
Print_To_Screen("5 SEC\n");
usleep(5000000); 
#else
Sleep(5);	//sleep for 5 milliseconds
#endif


//	Control_Termination();
#ifdef ONS_LINUX
	char *    SM_shm_p;
	int		  start_SM;

	usleep(10000);
	Print_To_Screen("5 SEC\n");
	usleep(5000000);
	SM_shm_p = SM_Comm_IPC_Get_Shared_Memory_Write_Pointer();
	start_SM = SM_EXIT;
	memcpy(SM_shm_p, &start_SM, sizeof(char));
	usleep(100000);

#else
	Sleep(10);	//sleep for 10 milliseconds
#endif

	USBComm_Termination();

	UI_Comm_Termination();
	exit(-1);
#endif

}

// ---------------------------------------------------------------------------
//  Function name		: Log_Dump_RTC_Data
//
//  Purpose				: Log_Dump_RTC_Data.
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
void Log_Dump_RTC_Data(unsigned int Time, unsigned int Type, unsigned int Data)
{

//#ifdef ENABLE_WRITE_ERROR_TO_LOG
	fprintf (Dump,"%u      %u         %u \n", Time, Type, Data);
	fflush(Dump);
#ifndef USE_SYSLOG
	fflush(Sys);
#endif
//#endif

#ifdef ENABLE_PRINT_TO_SCREEN
	char			msg_txt[MAX_SYSLOG_DESC_SIZE];
	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, ("%u       %u       %u \n", Time, Type, Data);
	ONS_Syslog_Send_Msg(SB_SEVERITY_ERR, (char*)__FILENAME__, __LINE__, (char*)__func__, msg_txt);
	//printf ("%u       %u       %u \n", Time, Type, Data);
#endif

}

// ---------------------------------------------------------------------------
//  Function name		: Log_Write_Msg
//
//  Purpose				: Log_Write_Msg.
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
//void Log_Write_Msg(const char* File_Name, const char* Func_Name, int Line_Number, int Msg_ID,int Data_1, int Data_2, int Data_3, int Data_4, int Data_5, int Data_6)
//{
//
//#ifdef ENABLE_WRITE_ERROR_TO_LOG
//	unsigned long sys_time;
//	fprintf (Sys,"%lu	%s	%s	%d : %d, %d, %d, %d, %d, %d, %d  \n",sys_time, File_Name, Func_Name, Line_Number,Msg_ID, Data_1, Data_2, Data_3, Data_4, Data_5, Data_6);
//			fflush(Sys);
//			 
//#endif
//
//#ifdef ENABLE_PRINT_TO_SCREEN
	//		printf ("%lu	%s	%s	%d : %d, %d, %d, %d, %d, %d, %d  \n",sys_time, File_Name, Func_Name, Line_Number,Msg_ID, Data_1, Data_2, Data_3, Data_4, Data_5, Data_6);
//#endif
//
//}


// ---------------------------------------------------------------------------
//  Function name		: Log_Write_String_Msg
//
//  Purpose				: Log_Write_String_Msg.
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
//void Log_Write_String_Msg(const char* File_Name, const char* Func_Name, int Line_Number, int Msg_ID,char *String)
//{
//
//
//#ifdef ENABLE_WRITE_ERROR_TO_LOG
//	unsigned long sys_time;
//
//	sys_time = Debug_Get_Time_in_Milliseconds();
//
//	fprintf (Sys,"%lu	%s	%s	%d : %d %s \n",sys_time, File_Name, Func_Name, Line_Number,Msg_ID, String);
//	fflush(Sys);
//
//#endif
//
//#ifdef ENABLE_PRINT_TO_SCREEN
		//printf ("%lu	%s	%s	%d : %d %s \n",sys_time, File_Name, Func_Name, Line_Number,Msg_ID, String);
//#endif
//
//}

//void Log_Write_Str(const char *Str)
//{
//
////	unsigned long sys_time;
//
////	sys_time = Debug_Get_Time_in_Milliseconds();
//
//#ifdef ENABLE_WRITE_ERROR_TO_LOG
//	fprintf(Sys, "%s\n", Str);
//	//fflush(Sys);
//
//#endif
//
//#ifdef ENABLE_PRINT_TO_SCREEN
	//printf ("%s \n", Str);
//#endif
//
//}

void Debug_Set_Debug_Level_Exist(int Exist)
{
	Debug_Level_Exist = Exist;
}


int Debug_Get_Debug_Level_Exist(void)
{
	return Debug_Level_Exist;
}

void Debug_Set_Syslog_Level (int Level)
{
	Syslog_Level = Level;
}

int Debug_Get_Syslog_Level ( void )
{
	return Syslog_Level;
}

#ifndef USE_SYSLOG
//This function will be used if there is no Syslog 
void ONS_Syslog_Send_Msg(SyslogBuff_Severity_t Msg_Severity, const char* File_Name, int Line, const char* Func_Name, char* Msg_Data)
{
	unsigned long long Data_Time;

#ifdef ONS_HOST_LINUX
	struct timeval 	tv;
	gettimeofday(&tv, NULL);
	Data_Time = (tv.tv_sec * 1000000 + tv.tv_usec);			//Value in microseconds
#endif // ONS_HOST_LINUX

#ifdef ONS_WIN
	SYSTEMTIME time;
	GetSystemTime(&time);
	Data_Time = ((time.wSecond * 1000) + time.wMilliseconds);
#endif // ONS_HOST_WIN

	//if (Sys == NULL)
	//{
	//	printf("%llu  %d  %s  %u  %s : %s \n", Data_Time, Msg_Severity, File_Name, Line, Func_Name, Msg_Data);// time severity file line func data
	//}
	//else
	if (Sys != NULL)
	{
		fprintf(Sys, "%llu  %d %s  %u  %s : %s \n", Data_Time, Msg_Severity, File_Name, Line, Func_Name, Msg_Data);// time severity file line func data
	}
	//fprintf(Sys, "%s \n", Msg_Data);// time severity file line func data

return;
}
#endif

void Print_To_Screen(const char * data)
{
	printf("HOST : %s \n", data); //print to screen
	fflush(stdout);
}




int ONS_Copy_File(char* Source_File_Name, char* Target_File_Name)
{
	char	tmp;
	FILE*	target;
	FILE*	source;
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];

	source = fopen(Source_File_Name, "r");
	if (source == NULL)
	{
		snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Open to create file %s \n", Source_File_Name);
		Print_To_Screen(msg_txt);

		return(-1);
	}
	
	target = fopen(Target_File_Name, "w+");
	if (target == NULL)
	{
		snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "Open to create file %s \n", Target_File_Name);
		Print_To_Screen(msg_txt);

		return(-1);
	}

	while ((tmp = fgetc(source)) != EOF)
		fputc(tmp, target);

	printf("HOST : Copy Ended successfully.\n");

	fclose(source);
	fclose(target);
	return 0;
}

int Teminate_Syslog(void)
{
#ifdef USE_SYSLOG

	char	source_file_name[LOG_NAME_BUFF_SIZE];
	char	target_folder_name[LOG_NAME_BUFF_SIZE];
	char	Syslog_File_Name[LOG_NAME_BUFF_SIZE];
	char	msg_txt[MAX_SYSLOG_DESC_SIZE];
	char	mv_command[255];
	int		res;

	ONS_Log.Update_Log_Data(LOG_CRITICAL_LEVEL, LOG_CATEGORY_GENERAL, 3, __FILENAME__, __LINE__, __func__, (char*)"Need To Terminate Syslog");

	snprintf(msg_txt,MAX_SYSLOG_DESC_SIZE, "Total Retry of SYSLOG = %d",ONS_Syslog_Get_Retry_Counter());
	ONS_Log.Update_Log_Data(LOG_CRITICAL_LEVEL, LOG_CATEGORY_GENERAL, 4, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Syslog_Send_Msg(SB_SEVERITY_NOTICE, __FILENAME__ , __LINE__ ,__func__, msg_txt );

	snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "SYSLOG Total Send Messages Counter  = %lu , Total Printed Messages Counter = %lu ", ONS_Syslog_Get_Send_Messages_Counter(), ONS_Syslog_Get_Printed_Messages_Counter());
	ONS_Log.Update_Log_Data(LOG_CRITICAL_LEVEL, LOG_CATEGORY_GENERAL, 5, __FILENAME__, __LINE__, __func__, msg_txt);
	//ONS_Syslog_Send_Msg(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);

	//get the file name from syslog chiled process
	ONS_Syslog_Get_Log_Name(Syslog_File_Name);

	sprintf(source_file_name, "%s/%s", LOG_FOLDER_PATH, Syslog_File_Name);
	sprintf(target_folder_name, "%s/%s",Log_Folder_Name, Syslog_File_Name);
	usleep(10000);
	ONS_Syslog_Close_Request();
	usleep(10000);

//Print_To_Screan()
	//if (system("mv source_file_name target_folder_name") != 0)
	res = system(mv_command);
	if (res)
		//	if (rename(source_file_name, target_folder_name))
	{
				printf("HOST : source_file_name = %s, target_folder_name=%s\n",source_file_name, target_folder_name );
		snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "source_file_name = %s, target_folder_name=%s res = %d", source_file_name, target_folder_name, res);
		ONS_Log.Update_Log_Data(LOG_CRITICAL_LEVEL, LOG_CATEGORY_GENERAL, 20, __FILENAME__, __LINE__, __func__, msg_txt);
		ONS_Log.Convert_Old_Syslog_Format_To_Temp_New_Format(SB_SEVERITY_NOTICE, __FILENAME__, __LINE__, __func__, msg_txt);
		ONS_Log.Update_Log_Data(LOG_CRITICAL_LEVEL, LOG_CATEGORY_GENERAL, 21, __FILENAME__, __LINE__, __func__, (char*)"Error: unable to rename the file");
		Print_To_Screen("Error: unable to rename the file");
		snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "source_file_name = %s, target_folder_name=%s res = %d", source_file_name, target_folder_name, res);
		Print_To_Screen(msg_txt);

	}
	else
	{
		Print_To_Screen("File renamed successfully");
	}
	usleep(1000000);


#else
	fflush(Sys);
#endif
	return (0);

}


// ---------------------------------------------------------------------------
//  Function name		: Write_Error_To_Log_USB_Err
//
//  Purpose				: write error to log.
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
void Log_Write_Error_USB(const char* File_Name, const char* Func_Name, int Line_Number, int Error_Code,const char* Error_String)
{
	//unsigned long sys_time;

//	sys_time = Debug_Get_Time_in_Milliseconds();

#ifdef ENABLE_WRITE_ERROR_TO_LOG
	unsigned long sys_time;

	sys_time = Debug_Get_Time_in_Milliseconds();
	fprintf (Sys,"%lu	%s	%s	%d : ERROR!!!  code: %d, %s  \n",sys_time, File_Name, Func_Name, Line_Number,Error_Code, Error_String);
#endif

//#ifdef ENABLE_PRINT_TO_SCREEN
	unsigned long sys_time1;

	sys_time1 = Debug_Get_Time_in_Milliseconds();
	printf ("HOST : %lu	%s	%s	%d : ERROR!!!  code: %d, %s  \n",sys_time1, File_Name, Func_Name, Line_Number,Error_Code, Error_String);
//#endif
	return;
//Print_Trace_Log();
#ifdef ENABLE_TERMINET_ON_ERROR

char			msg_txt[MAX_SYSLOG_DESC_SIZE];
int				res;

ONS_Log.Update_Log_Data(LOG_CRITICAL_LEVEL, LOG_CATEGORY_GENERAL, 7, __FILENAME__, __LINE__, __func__, (char*)"Need To Terminate Host");

if (PDC_Ctrl.Is_PDC_Exist() || IST_Ctrl.Is_IST_Exist())
{
	if (Sew_Ctrl.Is_Host_Version_Sent_To_PDC())
	{
		PDC_Ctrl.Add_Event_Report(PDC_CTRL_POWER_OFF_EVENT, "");
	}
	PDC_Ctrl.Update_Events();
	res = PDC_Ctrl.Save_And_send_Event_File(ONS_TRUE, ONS_TRUE);

	//res = PDC_Ctrl.Save_And_send_Event_File(ONS_TRUE);
}


//Control_Set_System_HW_To_Termination_Mode();
//res = Msg_Prot_Pack_n_Tx_RTC_Reset_Cmd();
//#ifdef ONS_LINUX
//usleep(2);
//#endif



if (Machine_Mngr.Is_Exit_To_Terminal_Enable())
{
	Control_Handle_Error(POP_UP_MSG_SHUT_DOWN_WARNING, ONS_FALSE, (char*) "");
}
else
{
//	res = Msg_Prot_Pack_n_Tx_RTC_Reset_Cmd();
//#ifdef ONS_LINUX
//	usleep(2);
//#endif
	UI_Prot_Pack_n_Tx_Pop_Up_Req(UI_POP_UP_EMPTY_ID, POP_UP_MSG_SHUT_DOWN_WARNING, (char*) "");
	while (1)
	{
		Control_Handle_Incoming_Msg();
	}
}

res = Msg_Prot_Pack_n_Tx_RTC_Reset_Cmd();
#ifdef ONS_LINUX
usleep(2);
#endif

#ifdef USE_SYSLOG
Teminate_Syslog();

#else
fflush(Sys);
#endif

#ifdef ONS_LINUX

if (PDC_Ctrl.Is_PDC_Exist())
{
	usleep(300000);
	PDC_Ctrl.Send_Terminate_PDC();
	sleep(3);
	PDC_Ctrl.Free_PDC_Memory();
}


#else
fflush(Sys);
#endif
res = UI_Prot_Pack_n_Tx_Terminate_UI();		//Send Terminate message to UI
printf( "HOST : UI_Prot_Pack_n_Tx_Terminate_UI (res = %d) \n",res);
snprintf(msg_txt, MAX_SYSLOG_DESC_SIZE, "UI_Prot_Pack_n_Tx_Terminate_UI (res = %d)\n",res);
//ONS_Syslog_Send_Msg(SB_SEVERITY_ERR, __FILENAME__, __LINE__, __func__, msg_txt);
ONS_Log.Update_Log_Data(LOG_CRITICAL_LEVEL, LOG_CATEGORY_GENERAL, 8, __FILENAME__, __LINE__, __func__, msg_txt);

#ifdef ONS_LINUX
Print_To_Screen("5 SEC\n");
usleep(5000000);
#else
Sleep(5);	//sleep for 5 milliseconds
#endif

//	Control_Termination();
#ifdef ONS_LINUX
	char *    SM_shm_p;
	int		  start_SM;

	usleep(10000);
	Print_To_Screen("5 SEC\n");
	usleep(5000000);
	SM_shm_p = SM_Comm_IPC_Get_Shared_Memory_Write_Pointer();
	start_SM = SM_EXIT;
	memcpy(SM_shm_p, &start_SM, sizeof(char));
	usleep(100000);


#else
	Sleep(10);	//sleep for 10 milliseconds
#endif

	USBComm_Termination();

	UI_Comm_Termination();
	exit(-1);
#endif

}
