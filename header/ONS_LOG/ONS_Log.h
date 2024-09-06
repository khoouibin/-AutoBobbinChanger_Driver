// ---------------------------------------------------------------------------
//  Filename:  ONS_Log.h
//  Created by: Nissim Avichail
//  Date: 30.11.17
//  Orisol (c) 
// ---------------------------------------------------------------------------


#ifndef _ONS_LOG_H_
#define _ONS_LOG_H_

#define		MAX_LEVEL_NAME_SIZE				3
#define		MAX_CATEGORY_NAME_SIZE			15


#define	LOG_CATEGORY_NAME_GENERAL				"GENERAL"
#define	LOG_CATEGORY_NAME_RTC					"RTC"
#define	LOG_CATEGORY_NAME_RTC_COMM				"RTC_COMM"
#define	LOG_CATEGORY_NAME_RTC_DEBUG				"RTC_DEBUG"
#define	LOG_CATEGORY_NAME_UI					"UI"
#define	LOG_CATEGORY_NAME_UI_COMM				"UI_COMM"
#define	LOG_CATEGORY_NAME_SEW_EDITOR			"SEW_ETITOR"
#define	LOG_CATEGORY_NAME_SEW_EDITOR_COMM		"SEW_EDITOR_COMM"
#define	LOG_CATEGORY_NAME_TRACE					"TRACE"
#define	LOG_CATEGORY_NAME_HOST					"HOST"

#define	LOG_LEVEL_NAME_NO_DATA					"NO"
#define	LOG_LEVEL_NAME_CRITICAL					"CRT"
#define	LOG_LEVEL_NAME_ERROR					"ERR"
#define	LOG_LEVEL_NAME_WARNING					"WAR"
#define	LOG_LEVEL_NAME_NOTICE					"NTC"
#define	LOG_LEVEL_NAME_INFO						"INF"
#define	LOG_LEVEL_NAME_DEBUG					"DBG"


typedef enum
{
	LOG_NO_DATA_LEVEL	= 0,     //nothing print to log
	LOG_CRITICAL_LEVEL	= 1,
	LOG_ERROR_LEVEL		= 2,
	LOG_WARNING_LEVEL	= 3,
	LOG_NOTICE_LEVEL	= 4,
	LOG_INFO_LEVEL		= 5,
	LOG_DEBUG_LEVEL		= 6
}
	ONS_Log_Data_Level_t;

typedef enum
{
	LOG_CATEGORY_GENERAL			= 0,
	LOG_CATEGORY_RTC				= 1,
	LOG_CATEGORY_RTC_COMM			= 2,
	LOG_CATEGORY_RTC_DEBUG			= 3,
	LOG_CATEGORY_UI					= 4,
	LOG_CATEGORY_UI_COMM			= 5,
	LOG_CATEGORY_SEW_EDITOR			= 6,
	LOG_CATEGORY_SEW_EDITOR_COMM	= 7,
	LOG_CATEGORY_TRACE				= 8,
	LOG_CATEGORY_HOST				= 9,
	LOG_CATEGORY_OLD				= 10,

}
	ONS_Log_Data_Category_t;


class ONS_Log
{

private:
	//ONS_Log_Data_Level_t Log_System_Level;
	ONS_Log_Data_Level_t Log_General_Category_Level;
	ONS_Log_Data_Level_t Log_RTC_Category_Level;
	ONS_Log_Data_Level_t Log_RTC_Communication_Category_Level;
	ONS_Log_Data_Level_t Log_RTC_Debug_Category_Level;
	ONS_Log_Data_Level_t Log_UI_Communication_Category_Level;
	ONS_Log_Data_Level_t Log_UI_Category_Level;
	ONS_Log_Data_Level_t Log_Sew_Editor_Category_Level;
	ONS_Log_Data_Level_t Log_Sew_Editor_Communication_Category_Level;
	ONS_Log_Data_Level_t Log_Trace_Category_Level;
	ONS_Log_Data_Level_t Log_Host_Category_Level;
	ONS_Log_Data_Level_t Log_Old_Category_Level;
public:

	ONS_Log()
	{
		//Log_System_Level = LOG_NOTICE_LEVEL;
		Log_General_Category_Level = LOG_DEBUG_LEVEL;
		Log_RTC_Category_Level = LOG_DEBUG_LEVEL;
		Log_RTC_Communication_Category_Level = LOG_DEBUG_LEVEL;
		Log_RTC_Debug_Category_Level = LOG_DEBUG_LEVEL;
		Log_UI_Communication_Category_Level = LOG_DEBUG_LEVEL;
		Log_UI_Category_Level = LOG_DEBUG_LEVEL;
		Log_Sew_Editor_Category_Level = LOG_DEBUG_LEVEL;
		Log_Sew_Editor_Communication_Category_Level = LOG_DEBUG_LEVEL;
		Log_Trace_Category_Level = LOG_DEBUG_LEVEL;
		Log_Host_Category_Level = LOG_DEBUG_LEVEL;
		Log_Old_Category_Level = LOG_DEBUG_LEVEL;
	}

	int	Set_Log_Level(int Level);
	int Set_Log_Category(int Category, int Level);
	void Update_Log_Data(ONS_Log_Data_Level_t Data_Level, ONS_Log_Data_Category_t Data_Category, int ID, const char* File_Name, int Line, const char* Func_Name, char* Data_String);
	void Convert_Old_Syslog_Format_To_Temp_New_Format(int Msg_Severity, const char* File_Name, int Line, const char* Func_Name, char* Msg_Data);
	void Set_Log_General_Category_Level(int Level) { Log_General_Category_Level = (ONS_Log_Data_Level_t)Level; };
	void Set_Log_RTC_Category_Level(int Level) { Log_RTC_Category_Level  = (ONS_Log_Data_Level_t)Level; };
	void Set_Log_RTC_Communication_Category_Level(int Level) {	Log_RTC_Communication_Category_Level = (ONS_Log_Data_Level_t)Level;};
	void Set_Log_RTC_Debug_Category_Level(int Level) { Log_RTC_Debug_Category_Level = (ONS_Log_Data_Level_t)Level; };
	void Set_Log_UI_Communication_Category_Level(int Level) { Log_UI_Communication_Category_Level = (ONS_Log_Data_Level_t)Level; };
	void Set_Log_UI_Category_Level(int Level) { Log_UI_Category_Level = (ONS_Log_Data_Level_t)Level; };
	void Set_Log_Sew_Editor_Category_Level(int Level) { Log_Sew_Editor_Category_Level = (ONS_Log_Data_Level_t)Level; };
	void Set_Log_Sew_Editor_Communication_Category_Level(int Level) { Log_Sew_Editor_Communication_Category_Level = (ONS_Log_Data_Level_t)Level; };
	void Set_Log_Trace_Category_Level(int Level) { Log_Trace_Category_Level = (ONS_Log_Data_Level_t)Level; };
	void Set_Log_Host_Category_Level(int Level) { Log_Host_Category_Level = (ONS_Log_Data_Level_t)Level; };
	void Set_Log_Old_Category_Level(int Level) {Log_Old_Category_Level = (ONS_Log_Data_Level_t)Level; };

	ONS_Log_Data_Level_t Get_Log_General_Category_Level(void)					{ return Log_General_Category_Level; };
	ONS_Log_Data_Level_t Get_Log_RTC_Category_Level(void)						{ return Log_RTC_Category_Level; };
	ONS_Log_Data_Level_t Get_Log_RTC_Communication_Category_Level(void)			{ return Log_RTC_Communication_Category_Level;};
	ONS_Log_Data_Level_t Get_Log_RTC_Debug_Category_Level(void)					{ return Log_RTC_Debug_Category_Level; };
	ONS_Log_Data_Level_t Get_Log_UI_Communication_Category_Level(void)			{ return Log_UI_Communication_Category_Level; };
	ONS_Log_Data_Level_t Get_Log_UI_Category_Level(void)						{ return Log_UI_Category_Level;	};
	ONS_Log_Data_Level_t Get_Log_Sew_Editor_Category_Level(void)				{ return Log_Sew_Editor_Category_Level; };
	ONS_Log_Data_Level_t Get_Log_Sew_Editor_Communication_Category_Level(void)	{ return Log_Sew_Editor_Communication_Category_Level; };
	ONS_Log_Data_Level_t Get_Log_Trace_Category_Level(void)						{ return Log_Trace_Category_Level; };
	ONS_Log_Data_Level_t Get_Log_Host_Category_Level(void)						{ return Log_Host_Category_Level; };
	ONS_Log_Data_Level_t Get_Log_Old_Category_Level(void)						{ return Log_Old_Category_Level; };


};


extern class ONS_Log	ONS_Log;



#endif  //_ONS_LOG_H_
