// ---------------------------------------------------------------------------
//  Filename:  INIFile.h
//  Created by: Shmulik Hass
//  Date:  14.02.18
//  Orisol (c) 
// ---------------------------------------------------------------------------


#ifndef _INIFILE_H_
#define _INIFILE_H_
#include "INI_File.h"

#define INIFILE_VERSION  "INI_FILE 0.1.6"

#define INI_FILE_TOO_MANY_SECTIONS_ERROR			INIFILE_ERROR(41)
#define INI_FILE_TOO_MANY_NAMES_PER_SECTION_ERROR	INIFILE_ERROR(42)

#define INI_FILE_CANT_OPEN_ERROR					INIFILE_ERROR(43)
#define INI_FILE_READ_ERROR							INIFILE_ERROR(44)

#define  INI_FILE_DUPLICATED_SECTION_NAME_ERROR		INIFILE_ERROR(45)
#define  INI_FILE_ILLEGAL_SECTION_DEF				INIFILE_ERROR(46)
#define  INI_FILE_ASSIGN_SYNTAX_ERROR				INIFILE_ERROR(47)
#define  INI_FILE_SECTION_PROBLEM_ERROR				INIFILE_ERROR(48)
#define  INI_FILE_DUPLICATE_NAME_ERROR				INIFILE_ERROR(49)


#define  INI_FILE_SECTION_NOT_FOUND					INIFILE_ERROR(50)

#define  INI_FILE_NAME_NOT_FOUND					INIFILE_ERROR(51)
#define  INI_FILE_EMPTY_FILE						INIFILE_ERROR(52)




#ifdef  ONS_ERRORS

static Error_Rec_t  INIFILE_Errors[] =
{

	ERROR_MSG_REC(INI_FILE_TOO_MANY_SECTIONS_ERROR),	
	ERROR_MSG_REC(INI_FILE_TOO_MANY_SECTIONS_ERROR),
	ERROR_MSG_REC(INI_FILE_TOO_MANY_NAMES_PER_SECTION_ERROR),
	ERROR_MSG_REC(INI_FILE_CANT_OPEN_ERROR),
	ERROR_MSG_REC(INI_FILE_READ_ERROR),
	ERROR_MSG_REC(INI_FILE_DUPLICATED_SECTION_NAME_ERROR),
	ERROR_MSG_REC(INI_FILE_ILLEGAL_SECTION_DEF),
	ERROR_MSG_REC(INI_FILE_ASSIGN_SYNTAX_ERROR),
	ERROR_MSG_REC(INI_FILE_SECTION_PROBLEM_ERROR),
	ERROR_MSG_REC(INI_FILE_DUPLICATE_NAME_ERROR),
	ERROR_MSG_REC(INI_FILE_SECTION_NOT_FOUND),
	ERROR_MSG_REC(INI_FILE_NAME_NOT_FOUND),
	ERROR_MSG_REC(INI_FILE_EMPTY_FILE),

	ERROR_MSG_REC(0)
};


#endif






#define MAX_SECTIONS 50
#define MAX_NAMES_PER_SECTION 300

typedef struct
{
	int N_Names ;
	char * Names [ MAX_NAMES_PER_SECTION];
	char * Values[MAX_NAMES_PER_SECTION];
} Section_Def_t ;

class INI_File
{

public:
	int N_Sections;
	char * Section_Names[MAX_SECTIONS];
	Section_Def_t * Section_Defs[MAX_SECTIONS];
	int Error_Code; //= 0 ;
	int Error_Line;//= 0 ;

	INI_File()
	{
		Error_Code = 0 ;
		Error_Line = 0 ;
		N_Sections = 0;
		Error_Code = 0;
	}
	~INI_File()
	{
		int i, j;
		for (i = 0; i < N_Sections; i++)
		{
			for (j = 0; j < Section_Defs[i]->N_Names; j++)
			{
				delete[]Section_Defs[i]->Names[j];
				delete[]Section_Defs[i]->Values[j];
			}
			delete Section_Defs[i];

			if (Section_Names[i])
				delete[]Section_Names[i];


		}

	}

	int Load_INI_File ( const char * File_Path);
	int Create_INI_File ( const char * File_Path );
	int Get_Name_Value ( const char * Section_Name, const char * Name, char * & Value );
	int Set_Name_Value( const char * Section_Name, const char * Name, char * Value);


	int Add_Section(const char * Section_Name, int & Section_No);
	int Add_Name(int Section_No, const char * Name, char * Value);
	int Find_Section ( const char * Section_Name , int & Section_No );
	int Find_Name ( int Section_No , const char * Name , char * &Value );
	//int Find_Name_Set_New_Value(int Section_No, const char * Name, char * Value);
	
	int Last_Error(int & Err_Code, int & Err_Line )
	{
		Err_Code = Error_Code ;
		Err_Line = Error_Line ;
		Error_Code = 0 ;
		Error_Line = 0 ;
		return ( Error_Code);
	}
	int Set_Error(int Err_Code, int Err_Line)
	{
		if (! Error_Code)		// keep last error unchanged
		{
			Error_Code = Err_Code;
			Error_Line = Err_Line;
		}
		return (Err_Code);
	}

	char * Get_Version(void)
	{
		static char Version[] = INIFILE_VERSION;
		return Version;
	}


#ifdef _DEBUG

	void Debug_INI_Def ( void );
	void Total_INI_Def(int & N_Sections, int & N_Names);

#endif



};

#endif
