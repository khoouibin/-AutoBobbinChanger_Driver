// ---------------------------------------------------------------------------
//  FileName: ONS_Errors.h
//  Created by: Shmulik Hass
//  Date:  16/12/2015
//  Orisol (c) 
// ---------------------------------------------------------------------------


#ifndef _ONS_ERRORS_H_
#define _ONS_ERRORS_H_

#define ONSERR_VERSION   "ONS_Errors 0.1.1"


#endif


typedef struct
{
	int Module_Id;
	int Error_Id;
	const char * Error_Msg;
} Error_Rec_t;


class ONS_Error
{
	public:
	const char * ONS_Err_Get_Message(int Err_Code);
	Error_Rec_t ** ONS_Error_Get_Error_Recs();
	const char ** ONS_Get_Module_Names();
	
	int ONS_Err_Num_Of_Modules();
	

	char * Get_Version(void)
	{
		static char Version[] = ONSERR_VERSION ;
		return Version;
	}
	

};
