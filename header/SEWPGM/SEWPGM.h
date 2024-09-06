// ---------------------------------------------------------------------------
//  Filename:  SEWPGM.h
//  Created by: Shmulik Hass
//  Date:  16.08.15
//  Last Update:  14.10.18, 6.12.18 , 3.1.19 , 14.2.19 , 25.6.19 , 1.7.19 , 6.8.19
//  Orisol (c) 
// ---------------------------------------------------------------------------

#ifndef _SEWPGM_H_
#define _SEWPGM_H_
#include "SEWPGS.h"
#include "ONS_Modules.h"

#define SEWPGM_VERSION   "SEWPGM 0.3.46"

#define RTC_BUFFER_DEF_SIZE		7
#define RTC_BUFFER_DEF_MIN_SIZE		3
#define RTC_BUFFER_DEF_UPDATE_FREQ  2

#define MAX_PROGRAM_LOADED  8

#define SEWPGM_CANT_OPEN_PGS_FILE			SEWPGM_ERROR(84)
#define SEWPGM_ERROR_ILLEGAL_LEAD_POINT_SL	SEWPGM_ERROR(85)
#define SEWPGM_ERROR_ILL_SEWLINE_NO			SEWPGM_ERROR(86)
#define SEWPGM_ERROR_ILL_LEADPOINT_NO		SEWPGM_ERROR(87)
#define SEWPGM_ERROR_ILLEGAL_CP_AT_LP		SEWPGM_ERROR(88)
#define SEWPGM_CONTROL_POINT_ERROR			SEWPGM_ERROR(89)


#define SEWPGM_ERROR_PROGRAM_NOT_EXIST		SEWPGM_ERROR(92)		
#define SEWPGM_PGS_EMPTY_PROGRAM			SEWPGM_ERROR(93)
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
	ERROR_MSG_REC(SEWPGM_PGS_EMPTY_PROGRAM),
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

#ifndef GLOBAL_MODE_DEFINED

typedef enum
{
	GLOBAL_MODE_NONE = 0,
	GLOBAL_MODE_VALUE = 1,
	GLOBAL_MODE_OFFSET = 2

} Global_Mode_t;

#define GLOBAL_MODE_DEFINED
#endif


// for both Tension and APF setting



typedef struct 
{
	double   Length_Since_Bobbin_Count;
	int		 Limit;

} Bobbin_Break_Detector_t;

class Stc_Data_Log
{
	int Num_stitches_in_avg;
	int Max_Actual_Rpm;	
	long long Total_Actual_Rpm;
	int Max_Actual_TT;
	int Min_Actual_TT;
	long long Total_Actual_TT;

public:

	void Stc_Data_Log_Reset();
	void Stc_Data_Log_Add_Val(int Rpm , int TT);
	void Stc_Data_Log_Get_Vals(int &Max_Rpm, int &Avg_Rpm, int & Max_TT, int & Min_TT , int & Avg_TT );

	Stc_Data_Log()
	{
		Max_Actual_Rpm = 0;
		Num_stitches_in_avg = 0; 
		Total_Actual_Rpm = 0;
		Max_Actual_TT = 0 ;
		Min_Actual_TT = 0 ;
		Total_Actual_TT = 0 ;
	}

};



#define  BOBIN_BREAK_LIMIT_DEFAULT  20  // in mm


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

	PGS_Settings_t       PGS_Setting;
	Bobbin_Break_Detector_t Bobbin_Break_Detector;
	int					 Cut_Counts;

	class Stc_Data_Log		PGM_Stc_Data_Log;

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

		PGS_Setting.Open_Clamp_When_Stop = 0; // default is OFF
		PGS_Setting.Reduce_Speed_On_Corner = 0; // default is OFF
		PGS_Setting.Default_Reduced_Speed = 0 ; // undefined
		PGS_Setting.Fix_APF_Shift_For_Up = -1; // default for dynamic calculation
		PGS_Setting.End_Section_Ignore = 0;	// check [END] and use it as END but not mandate
		PGS_Setting.First_SP_as_SB = 0;		// no workaround for SP at first stitch.
		PGS_Setting.APF_Up_On_SP = 0; // default is not moving up

		PGS_Setting.Global_Apf_Mode = GLOBAL_MODE_NONE;
		PGS_Setting.Global_Apf_Value = PGS_Setting.Global_Apf_Offset = 0; 
		PGS_Setting.Global_Tension_Mode = GLOBAL_MODE_NONE;
		PGS_Setting.Global_Tension_Value = PGS_Setting.Global_Tension_Offset = 0;
		
		Stitch_Buff.Fix_APF_Shift_For_Up = -1;
		Bobbin_Break_Detector.Length_Since_Bobbin_Count = 0.;
		Bobbin_Break_Detector.Limit = BOBIN_BREAK_LIMIT_DEFAULT;
		Cut_Counts = 0;
	}

	static Sew_PGM *GetInstance();

	void SEWPGM_Init ( void);
	
	int SEWPGM_Set_RTC_Buffer_Limits ( int Buff_Size, int Min_Ahead, int Update_Freq );
	
	void SEWPGM_Set_Open_Clamp_On_Stop_flag ( int Open_Clamp_When_Stop)   { PGS_Setting.Open_Clamp_When_Stop = Open_Clamp_When_Stop; };
	void SEWPGM_Set_Reduce_Speed_On_Corner_flag ( int Reduce_Speed_On_Corner)   { PGS_Setting.Reduce_Speed_On_Corner = Reduce_Speed_On_Corner; };
	void SEWPGM_Set_Default_Reduced_Speed ( int Default_Reduced_Speed)   { PGS_Setting.Default_Reduced_Speed = Default_Reduced_Speed; };
	void SEWPGM_Set_Fix_APF_Shift_For_Up(int Fix_APF_Shift_For_Up) { PGS_Setting.Fix_APF_Shift_For_Up = Stitch_Buff.Fix_APF_Shift_For_Up = Fix_APF_Shift_For_Up; };
	void SEWPGM_Set_End_Section_Ignore(int End_Section_Ignore) { PGS_Setting.End_Section_Ignore = End_Section_Ignore; };
	void SEWPGM_Set_First_SP_as_SB(int First_SP_as_SB) { PGS_Setting.First_SP_as_SB = First_SP_as_SB; };
	void SEWPGM_Set_APF_Up_On_SP(int APF_Up_At_SP) { PGS_Setting.APF_Up_On_SP = APF_Up_At_SP; 	};

	void SEWPGM_Set_Tension_Global(Global_Mode_t Mode, int Value, int Offset) {
		PGS_Setting.Global_Tension_Mode = Mode;  PGS_Setting.Global_Tension_Value = Value; PGS_Setting.Global_Tension_Offset = Offset;
	};
	void SEWPGM_Set_APF_Global(Global_Mode_t Mode, int Value, int Offset) {
		PGS_Setting.Global_Apf_Mode = Mode;  PGS_Setting.Global_Apf_Value = Value; PGS_Setting.Global_Apf_Offset = Offset;
	};

	void SEWPGM_Get_Tension_Global(Global_Mode_t & Mode, int & Value, int & Offset) {
		Mode = (Global_Mode_t)PGS_Setting.Global_Tension_Mode ;  Value = PGS_Setting.Global_Tension_Value ; Offset = PGS_Setting.Global_Tension_Offset ;
	};
	void SEWPGM_Get_APF_Global(Global_Mode_t & Mode, int & Value, int & Offset) {
		Mode = (Global_Mode_t)PGS_Setting.Global_Apf_Mode ;  Value = PGS_Setting.Global_Apf_Value ; Offset = PGS_Setting.Global_Apf_Offset ;
	};
	
	void SEWPGM_Set_Bobbin_Break_Detector_Limit(int Bobbin_Break_Detector_Limit) {	Bobbin_Break_Detector.Limit = Bobbin_Break_Detector_Limit; }
	
		
	int	SEWPGM_Load_PGS(int PGM_No, const char * PGS_File_Name);
	int SEWPGM_Save_PGS(int PGM_No, char * PGS_File_Name); // not to be used for now 
	int SEWPGM_Load_PGS_Header ( char * PGS_File_Name, PGS_Header_t & PGS_Header );
	int SEWPGM_Set_XY_Offset(int PGM_No, double X_Offset, double Y_Offset);
	int	SEWPGM_Select_Program(int PGM_No );	
	int SEWPGM_Verify_All_XY_In_Range(int PGM_No);
	int SEWPGM_Is_Program_Empty(int PGM_NO);
	
	int SEWPGM_Is_Sewline_Skipped(int SL_No, int & Skipped);
	int SEWPGM_Select_Sewline(int SL_No);
	int SEWPGM_Is_Sewline_First_non_Skipped(int SL_No, int & First);

	int SEWPGM_Set_Stitch_Position(SEWPGM_STITCH_POSITION_t  Stitch_Position );
	int SEWPGM_Update_Stitch_Position(int Stitch_No);

	int SEWPGM_Next_Stitch(RTC_Stitch_t & Stitch_Def, int & Flush_Buffer );
	int SEWPGM_Peek_Stitch(int Stitch_No, RTC_Stitch_t & Stitch_Def, int & Current, int & Last_Sent);
	int SEWPGM_Set_Stitching_Mode(SEWPGM_MODES_t  Mode);

	int SEWPGM_Get_Current_Stitch_Position(int & Cur_SL, int & Cur_Pos) { Cur_SL = Current_Sewline; Cur_Pos = Current_Stitch ; return (0); }

	int SEWPGM_Get_Stitch_Index(int St_Pos, int & St_Index);	  // return Stitch index ( PGS list index, i.e. no BT ) in current SL
	int SEWPGM_Get_Stitch_By_Index(int St_Index, int & St_Pos);  // reverse function.
	
	int SEWPGM_Get_Current_Stitch_XY(int & X_Pos, int & Y_Pos);
	int SEWPGM_Get_Current_Apf_Height ( int & Apf_Pos );
	int SEWPGM_Get_Current_Tension ( int & Tension_Level );
	int SEWPGM_Get_Current_RPM  ( int & RPM );
	
	int SEWPGM_Is_Stop_Stitch(int & At_Stop, int & At_Last, int & Wait_Time, int & Wait_IO );
	int SEWPGM_Is_First_Stitch(int & At_First);
	int SEWPGM_Is_Last_Stitch ( int & At_Last);

	int SEWPGM_Get_Number_Of_Sewlines(int & N_Sewlines);
	int SEWPGM_Get_Total_Number_Of_Stitches(int & Total_Stitches);

	int SEWPGM_Get_Number_Of_Leadpoints ( int SL_No , int & N_Leadpoint );
	int SEWPGM_Get_Leadpoints(int SL_No, int LP_No, Lead_Point_t & Lead_Point); 	

	int SEWPGM_Update_RPM_In_Sewline(void);
	int SEWPGM_Stop_Stitching (void);

	int SEWPGM_Update_Stopped(void);
	
	int SEWPGM_Get_Stop_Time ( int & Stop_Time );
	int SEWPGM_Get_Stop_IO ( int & IO_Def, int & IO_Val );
	int SEWPGM_Get_Stop_Open_Clamp(int & Open_Clamp);
	int SEWPGM_Get_Stop_APF_Up(int & APF_Up);

	char * Get_Version(void)
	{
		static char Version[] = SEWPGM_VERSION;
		return Version;
	}

	int SEWPGM_Get_SewLine_Thread_Length ( int SL_No , long int & Length , int & N_Stitches );
	int SEWPGM_Get_Pgm_Thread_Length(long int & Length, int & N_Sewlines, int & N_Stitches);

	//int SEWPGM_Get_Program_Loading_Point ( double & Loading_Point_X , double & Loading_Point_Y ); // TBD to change it when reading header only
	//int SEWPGM_Get_Program_Ref_Point ( double & Ref_Point_X , double & Ref_Point_Y );// TBD to change it when reading header only


	int SEWPGM_Trace_Stitch(int Stitch_No);
	
	//  for Control Points Editor

	int SEWPGM_Get_Sewline_Def(int SL_No, SewLine_Def_t & SewLineDef );
	int SEWPGM_Set_Sewline_Def(int SL_No, SewLine_Def_t & SewLineDef );// be careful - don't change first 3 fields: N_Stitches, Start, End !! not suported yet
	int SEWPGM_Get_Stitch_Data(int SID, PGS_Stitch_t & PGS_Stitch );
	int SEWPGM_Get_Stitch_Control_Data(int SID, Control_Point_Values_t & Control_Point_values ); 
	int SEWPGM_Set_Stitch_Control_Data(int SID, Control_Point_Values_t & Control_Point_values); // be careful trusted client !! // not supported yet

	// Bobbin_Break_Detector

	void SEWPGM_Bobbin_Detector_Reset();
	int SEWPGM_Bobbin_Detector_Add_Stitch(int Stitch_Id);
	int SEWPGM_Bobbin_Detector_Check_Alarm();

	// Cut counter

	void SEWPGM_Set_Cut_Counts( int Counts) { Cut_Counts = Counts;  }
	int  SEWPGM_Get_Cut_Counts() { return Cut_Counts; }

	// RPM log 
	void SEWPGM_Reset_Stc_Data_Log();
	void SEWPGM_Get_Stc_Data_Log(int & Max_Rpm, int & Avg_Rpm, int & Max_TT , int & Min_TT , int & Avg_TT );


};

// extern class Sew_PGM Sew_PGM;

#endif
