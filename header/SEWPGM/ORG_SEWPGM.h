// ---------------------------------------------------------------------------
//  Filename:  SEWPGM.h
//  Created by: Shmulik Hass
//  Date:  16.08.15
//  Orisol (c) 
// ---------------------------------------------------------------------------

#ifndef _SEWPGM_H_
#define _SEWPGM_H_


#define SEWPGM_VERSION   "SEWPGM 0.2.5"

#define RTC_BUFFER_DEF_SIZE		7
#define RTC_BUFFER_DEF_MIN_SIZE		3
#define RTC_BUFFER_DEF_UPDATE_FREQ  2

#define MAX_PROGRAM_LOADED  4

#define SEWPGM_CANT_OPEN_PGS_FILE			SEWPGM_ERROR(84)
#define SEWPGM_ERROR_ILLEGAL_LEAD_POINT_SL	SEWPGM_ERROR(85)
#define SEWPGM_ERROR_ILL_SEWLINE_NO			SEWPGM_ERROR(86)
#define SEWPGM_ERROR_ILL_LEADPOINT_NO		SEWPGM_ERROR(87)
#define SEWPGM_ERROR_ILLEGAL_CP_AT_LP		SEWPGM_ERROR(88)
#define SEWPGM_CONTROL_POINT_ERROR			SEWPGM_ERROR(89)


#define SEWPGM_ERROR_PROGRAM_NOT_EXIST		SEWPGM_ERROR(92)		
#define SEWPGM_ERROR_NO_SEWLINES			SEWPGM_ERROR(93)
#define SEWPGM_ERROR_ILL_STATE				SEWPGM_ERROR(94)
#define SEWPGM_ERROR_ILL_STITCH_NO			SEWPGM_ERROR(95)

#define SEWPGM_ERROR_NO_NEXT_STITCH			SEWPGM_ERROR(96)
#define SEWPGM_ERROR_ILL_MODE				SEWPGM_ERROR(97)


#define SEWPGM_ERROR_SKIP_SEWLINE			SEWPGM_ERROR(98)	

#define SEWPGM_ERROR_NO_LOADING_POINT		SEWPGM_ERROR(100)	
#define SEWPGM_ERROR_NO_REF_POINT			SEWPGM_ERROR(101)	



#ifdef  ONS_ERRORS

static Error_Rec_t  SEWPGM_Errors[] =
{
	ERROR_MSG_REC(SEWPGM_CANT_OPEN_PGS_FILE),
	ERROR_MSG_REC(SEWPGM_ERROR_ILLEGAL_LEAD_POINT_SL),
	ERROR_MSG_REC(SEWPGM_ERROR_ILL_SEWLINE_NO),
	ERROR_MSG_REC(SEWPGM_ERROR_ILL_LEADPOINT_NO),
	ERROR_MSG_REC(SEWPGM_ERROR_ILLEGAL_CP_AT_LP),
	ERROR_MSG_REC(SEWPGM_CONTROL_POINT_ERROR),
	ERROR_MSG_REC(SEWPGM_ERROR_PROGRAM_NOT_EXIST),
	ERROR_MSG_REC(SEWPGM_ERROR_NO_SEWLINES),
	ERROR_MSG_REC(SEWPGM_ERROR_ILL_STATE),
	ERROR_MSG_REC(SEWPGM_ERROR_ILL_STITCH_NO),
	ERROR_MSG_REC(SEWPGM_ERROR_NO_NEXT_STITCH),
	ERROR_MSG_REC(SEWPGM_ERROR_ILL_MODE),
	ERROR_MSG_REC(SEWPGM_ERROR_SKIP_SEWLINE),
	ERROR_MSG_REC(SEWPGM_ERROR_NO_LOADING_POINT),	
	ERROR_MSG_REC(SEWPGM_ERROR_NO_REF_POINT),	

	ERROR_MSG_REC(0)

};


#endif




typedef enum {
					SEWPGM_STATE_INIT = 0,
					SEWPGM_STATE_PGM_READY ,
					SEWPGM_STATE_SL_READY ,
					SEWPGM_STATE_STITCHING ,
					SEWPGM_STATE_DRYRUN 
} SEWPGM_STATES_t;


typedef enum {
					SEWPGM_MODE_STOPPED ,
					SEWPGM_MODE_STITCHING, 
					SEWPGM_MODE_DRYRUN_FW,
					SEWPGM_MODE_DRYRUN_BW 
					
} SEWPGM_MODES_t ;


typedef enum
{
					STITCH_POSITION_FIRST_IN_SEWLINE,
					STITCH_POSITION_LAST_IN_SEWLINE,
					STITCH_POSITION_NEXT_STITCH,
					STITCH_POSITION_PREV_STITCH 
} SEWPGM_STITCH_POSITION_t ;


class Sew_PGM
{
public:
	class PGS_Program    * PGS_Programs[MAX_PROGRAM_LOADED];
	int					 Current_Program ;
	int					 Current_Sewline ;
	int					 Current_Mode ;

	class Stitch_List	 Stitch_Buff ;
	int					 Current_Stitch ;

	int					 State ;				

	int					 Stitch_Position_Updated ; // TBD needed ??
	struct {
		int	Size;
		int	Min_Size;
		int	Update_Freq;
		int Checks;
		int Last_Sent_Stitch ;		

	}  RTC_Buffer ;
	
	Sew_PGM()
	{
		for (int i = 0; i < MAX_PROGRAM_LOADED; i++)
		{ 
			PGS_Programs[i] = NULL; 
		}
		SEWPGM_Init();
		RTC_Buffer.Size = RTC_BUFFER_DEF_SIZE ;
		RTC_Buffer.Min_Size = RTC_BUFFER_DEF_MIN_SIZE ;
		RTC_Buffer.Update_Freq = RTC_BUFFER_DEF_UPDATE_FREQ ;
		

	}

	void SEWPGM_Init ( void);
	
	int SEWPGM_Set_RTC_Buffer_Limits ( int Buff_Size, int Min_Ahead, int Update_Freq );
	
	int	SEWPGM_Load_PGS(int PGM_No, char * PGS_File_Name);
	int	SEWPGM_Select_Program(int PGM_No );
	int SEWPGM_Is_Sewline_Skipped(int SL_No, int & Skipped);
	int SEWPGM_Select_Sewline(int SL_No);

	int SEWPGM_Set_Stitch_Position(SEWPGM_STITCH_POSITION_t  Stitch_Position );
	int SEWPGM_Update_Stitch_Position(int Stitch_No);

	int SEWPGM_Next_Stitch(RTC_Stitch_t & Stitch_Def, int & Flush_Buffer );
	int SEWPGM_Peek_Stitch(int Stitch_No, RTC_Stitch_t & Stitch_Def, int & Current, int & Last_Sent);
	int SEWPGM_Set_Stitching_Mode(SEWPGM_MODES_t  Mode);

	int SEWPGM_Get_Current_Stitch_Position(int & Cur_SL, int & Cur_Pos) { Cur_SL = Current_Sewline; Cur_Pos = Current_Stitch ; return (0); }
	
	int SEWPGM_Get_Current_Stitch_XY(int & X_Pos, int & Y_Pos);

	int SEWPGM_Is_Stop_Stitch(int & At_Stop, int & At_Last, int & Wait_Time, int & Wait_IO);
	int SEWPGM_Is_First_Stitch(int & At_First);

	int SEWPGM_Get_Number_Of_Sewlines(int & N_Sewlines);

	int SEWPGM_Get_Number_Of_Leadpoints ( int SL_No , int & N_Leadpoint );
	int SEWPGM_Get_Leadpoints(int SL_No, int LP_No, Lead_Point_t & Lead_Point); 	

	int SEWPGM_Update_RPM_In_Sewline(void);
	int SEWPGM_Stop_Stitching (void);

	int SEWPGM_Update_Stopped(void);
	
	int SEWPGM_Get_Stop_Time ( int & Stop_Time );
	int SEWPGM_Get_Stop_IO ( int & IO_Def, int & IO_Val );

	char * Get_Version(void)
	{
		static char Version[] = SEWPGM_VERSION;
		return Version;
	}

	int SEWPGM_Get_SewLine_Thread_Length ( int SL_No , long int & Length , int & N_Stitches );
	int SEWPGM_Get_Pgm_Thread_Length(long int & Length, int & N_Sewlines, int & N_Stitches);

	int SEWPGM_Get_Program_Loading_Point ( int & Loading_Point_X , int & Loading_Point_Y );
	int SEWPGM_Get_Program_Ref_Point ( int & Ref_Point_X , int & Ref_Point_Y );


	int SEWPGM_Trace_Stitch(int Stitch_No);

};


#endif
