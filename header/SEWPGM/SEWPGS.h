
// ---------------------------------------------------------------------------
//  Filename:  SEWPGS.h
//  Created by: Shmulik Hass
//  Date:  22.07.15
//  Update:  14.10.18. 6.12.18
//  Orisol (c) 
// ---------------------------------------------------------------------------




#ifndef _SEWPGS_H_
#define _SEWPGS_H_
#include "RTC_Stitch.h"
#include "ONS_General.h"

#define PGS_ERR_FILE_NOT_FOUND			SEWPGS_ERROR(61)
#define PGS_ERR_FILE_ERROR				SEWPGS_ERROR(62)
#define PGS_ERR_ILL_FORMAT				SEWPGS_ERROR(63)
#define PGS_ERR_TOO_MANY_STITCHES		SEWPGS_ERROR(64)
#define PGS_CONTROL_POINT_ERROR			SEWPGS_ERROR(65)
#define PGS_ERR_TOO_MANY_LEAD_POINTS	SEWPGS_ERROR(66)
#define PGS_ERR_ILLEGAL_LEAD_POINT_SL	SEWPGS_ERROR(67)
#define PGS_ERR_TOO_MANY_SEWLINES		SEWPGS_ERROR(68)
#define PGS_ERR_EMPTY_FILE_ERROR		SEWPGS_ERROR(69)
#define PGS_ERR_SM_TOO_MANY_STITCHES    SEWPGS_ERROR(70)


#ifdef  ONS_ERRORS

static Error_Rec_t  SEWPGS_Errors[] = 
{

	ERROR_MSG_REC (PGS_ERR_FILE_NOT_FOUND),
	ERROR_MSG_REC (PGS_ERR_FILE_ERROR),
	ERROR_MSG_REC (PGS_ERR_ILL_FORMAT),
	ERROR_MSG_REC (PGS_ERR_TOO_MANY_STITCHES),
	ERROR_MSG_REC (PGS_CONTROL_POINT_ERROR),
	ERROR_MSG_REC (PGS_ERR_TOO_MANY_LEAD_POINTS),
	ERROR_MSG_REC (PGS_ERR_ILLEGAL_LEAD_POINT_SL),
	ERROR_MSG_REC (PGS_ERR_TOO_MANY_SEWLINES),
	ERROR_MSG_REC (PGS_ERR_EMPTY_FILE_ERROR),
	ERROR_MSG_REC (PGS_ERR_SM_TOO_MANY_STITCHES),
		
	ERROR_MSG_REC(0)

};


#endif





#define MAX_PGS_STITCHES  9999
#define MAX_SEWLINES		256
#define MAX_LEAD_POINTS		100

#define  MAX_RTC_STITCHES  2000    // define Max stitches per sewline


typedef struct
{

	int		SS;		// start of sewline (number of backtachs )
	int     SE;		// end of sewline ( number of end backtachs )
	int		BK;		// backtach mode 1 = one pass, 2= two pass, 3 = repeated, 0= closed
	int		SK;		// sewline to be skipped
	int		SGZ;	// gloabl stitch size
	int		SZ;		// stitch size
	int		RPM;	// change RPM ( 120-2500 )
	int		SP;		// stop point
	int		ST;		// stop point with time limit
	int		WT;		// wait for input
	int		WTB;	// to wait for 0 or 1
	int		CR;		// corner point
	int		NT;		// No trimming , but stopping at end SL
	int		NS;		// No trimming , and no stooping at end SL
	int		NU;		// Needle up and halt
	int		ND;		// Needle down and HALT
//	int		OP;		// IO output	- OP goes to OPX
	int		OPX;	// IO inputs - all 16 bits
	int		OPXB;   // OPX mask	
	int		PF;		// PF Height (height index )
	int		TT;		// Thread Tenstion ( index )
	int		PH;		// pounching
	int		NR;		// Start no Rotation
	int		RR;		// END of Rotation
	int		EY;		// Eyelet
	int     CL;     // Open Clamp
	int     RE;     // Reduced RPM
	int		SB;		// Stop Before
	int		UW;		// UT Welder level
	int		SM;     // SM stitching mode
	
}
Control_Point_Values_t;

typedef struct
{
	char	Name[30];
	double		Offset_X;
	double		Offset_Y;
	double		Load_X;
	double		Load_Y;
	double		Ref_X;
	double		Ref_Y;
	double	Bobbin_Length;
	int		Sew_Line_Num;
	double  Sew_Stitch_size ;
	int		Rotation_180;
	int     Speed_Index ;
	char	Created_By[30];

} PGS_Header_t;

// static PGS_Header_t PGS_Header; // temp TBD delete


typedef struct
{
	int  Open_Clamp_When_Stop; // to activate CL control point by default
	int  Reduce_Speed_On_Corner; // reduce speed on corner points
	int  Default_Reduced_Speed; // Default reduced speed to use in case of Corner or RE(0)
	int  Fix_APF_Shift_For_Up; // number of stitches to move APF backward, -1 = dynamic calculation (default)
	int  End_Section_Ignore; // how to handle [end] - 0 - check- ( default), 1 - ignore . -1 - enforce ( check and reject if not exist)
	int  First_SP_as_SB;    // Consider SP on first stitch of sewline as SB ( stop before sewline), to support old pgs method
	int  APF_Height_To_Disable_Wiper; // If APF Height (in 0.1 mm units) is above this, Wiper will be disabled during cut cycle. if 0 then disabled.
	int  APF_Up_On_SP; // flag to define if to move APF Up after Stop at SP.
	
	int Global_Tension_Mode; // 0 - regular , 1- fixed value, 2 - offset 
	int Global_Tension_Value; // level value ( 0-127)
	int	Global_Tension_Offset; // offset (+/- level delta )
	
	int Global_Apf_Mode;  // 0 - regular , 1- fixed value, 2 - offset 
	int Global_Apf_Value; // level value in 0.1 mm units. (0-81)
	int Global_Apf_Offset;// offset (+/- level delta )
	

} PGS_Settings_t;

#ifndef GLOBAL_MODE_DEFINED

typedef enum
{
	GLOBAL_MODE_NONE = 0,
	GLOBAL_MODE_VALUE = 1,
	GLOBAL_MODE_OFFSET = 2

} Global_Mode_t;

#define GLOBAL_MODE_DEFINED
#endif


typedef struct

{
	double	X_Pos;
	double	Y_Pos;
	int		Act_RPM ;  // active RPM as result of CP
	int	    Act_PF;    // Active PF as result of CP
	int     Act_TT ;   // Active TT as result of TT 
	char *  Control_Def;
}
PGS_Stitch_t;



typedef struct

{
	int		N_Sewline ;
	double		X_Pos;
	double		Y_Pos;
	char *  Control_Def;
}
PGS_Lead_Point_t;

typedef  struct
{
	int		X_Pos;
	int		Y_Pos;
	int		Stop ;
	int     Open_Clamp;
	Out_Port_t		Out_Port;		// bit def of out control & OP/OPX
	Out_Port_Mask_t Out_Port_Mask;  // bit mask of out controls	
	int		Punch ;		// count of punchs as defined by PH(n) CP
	int		Eyelet ;	// in case of Eyelet defined
	int     Welder_Level; // 0 - no action. >0 - level of welder action (traslated to action time)

}
Lead_Point_t;

#define  NT_CP_MODE  1
#define  NS_CP_MODE  2

typedef struct
{
	int		N_Stitches; // number of stitches per sewline
	int		First_Stitch; // index
	int		Last_Stitch;  // index
	int		First_Stitch_After_BT ; // index to RTC stitch list
	int		Last_Stitch_Before_BT ; // index to RTC stitch list
	int		BK_Mode;	// type of Backtach
	int		SS;		// number of start stitches (SS)
	int		SE;		// number of end stitches (SE)
	int		Skipped;	// this sewline should be skipped!! (SK)
	int		NTNS;		// this sewline without trim, and 1=Stop (NT),2=No Stop(NS)
	int		SB;			// stop before sewline
	int		SP_As_SB;  // SP at first used as SB
}
SewLine_Def_t;




class PGS_Program
{


//private:
public: // temp
	PGS_Header_t		PGS_Header;
	double				X_Offset ;
	double				Y_Offset ;
	int					N_Stitches;
	PGS_Stitch_t		Stitches[MAX_PGS_STITCHES];
	int					N_Sewlines ;
	SewLine_Def_t		SewLines[MAX_SEWLINES];
	int					N_Lead_Points ;
	PGS_Lead_Point_t    Lead_Points[MAX_LEAD_POINTS];
	int					First_Lead_Point_Per_SL[MAX_SEWLINES+2];// +2 - to point to last lead point as well.
	PGS_Settings_t      PGS_Setting;
	
public:
	PGS_Program()
	{
		N_Stitches = 0;
		N_Lead_Points = 0 ;
		PGS_Header.Name[0] = '\0';
		PGS_Header.Offset_X = 0.;
		PGS_Header.Offset_Y = 0.;
		PGS_Header.Load_X = -1 ;	// -1 means undefine
		PGS_Header.Load_Y = -1;
		PGS_Header.Ref_X = -1;
		PGS_Header.Ref_Y = -1;
		PGS_Header.Bobbin_Length = 0.0;
		PGS_Header.Sew_Line_Num = 0;
		PGS_Header.Rotation_180 = 0;
		PGS_Header.Speed_Index = 0;
		PGS_Header.Sew_Stitch_size = 0 ;
		PGS_Header.Created_By[0] = '\0';
		X_Offset = 0. ;
		Y_Offset = 0. ;
		
		N_Sewlines = 0;
		
	}
	~PGS_Program()
	{
		int i;
		for (i = 0; i < N_Stitches; i++)
		{
			if (Stitches[i].Control_Def != NULL)
			{
				delete []Stitches[i].Control_Def;
				//delete Stitches[i].Control_Def;
				Stitches[i].Control_Def = NULL;

			}
		}
		for (i = 0; i < N_Lead_Points; i++)
		{
			if (Stitches[i].Control_Def != NULL)
			{
				delete []Lead_Points[i].Control_Def;
				//delete Lead_Points[i].Control_Def;
				Lead_Points[i].Control_Def = NULL;

			}
		}
	}

	int Load_PGS_File(FILE * PGS_File );

	int Load_PGS_Header(FILE * PGS_File, PGS_Header_t & PGS_Header );	// loading only header
	
	int Save_PGS_File(FILE * PGS_File );

	int Scan_Sewlines ();

	int Get_N_Sewlines() { return N_Sewlines; }

	int Get_Sewline_Length(int Sewline_No, long int & Length, int & N_Stitches);

	
};






class PGS_Control_Point
{

public: // temp
	Control_Point_Values_t Control_Point; // single control point

	int Check_Control(char * STR , int * pN_Control); // parse into inner value
	int Build_Control_Str ( char * STR, int * pN_Control );

	int Is_SS_Control(int * pSS)	{ *pSS = Control_Point.SS; 	return (Control_Point.SS != -1); }
	int Is_SE_Control(int * pSE)	{ *pSE = Control_Point.SE; 	return (Control_Point.SE != -1); }
	int Is_BK_Control(int * pBK)	{ *pBK = Control_Point.BK;  return (Control_Point.BK != -1); }
	int Is_SK_Control(void)			{ return (Control_Point.SK != -1); }
	int Is_SGZ_Control(int * pSGZ)  { *pSGZ = Control_Point.SGZ; return(Control_Point.SGZ != -1); }
	int Is_SZ_Control(int * pSZ)	{ *pSZ = Control_Point.SZ; 	return (Control_Point.SZ != -1); }
	int Is_RPM_Control(int * pRPM)	{ *pRPM = Control_Point.RPM; return(Control_Point.RPM != -1); }
	int Is_SP_Control(void)			{ return (Control_Point.SP != -1); }
	int Is_ST_Control(int * pST)	{ *pST = Control_Point.ST; 	return (Control_Point.ST != -1); }
	int Is_WT_Control(int * pWT)    { *pWT = Control_Point.WT;  return (Control_Point.WT != -1); }
	int Is_WTB_Control(int *pWTB)   { *pWTB = Control_Point.WTB; return(Control_Point.WTB != -1); }
	int Is_CR_Control(void)			{ return (Control_Point.CR != -1); }
	int Is_NT_Control(void)			{ return (Control_Point.NT != -1); }
	int Is_NS_Control(void)			{ return (Control_Point.NS != -1); }
	int Is_NU_Control(void)			{ return (Control_Point.NU != -1); }
	int Is_ND_Control(void)			{ return (Control_Point.ND != -1); }
	int Is_OPX_Control(int *pOPX, int *pOPXB)	{ *pOPX = Control_Point.OPX;  *pOPXB = Control_Point.OPXB ;  return(Control_Point.OPX != -1); }
	int Is_PF_Control(int * pPF)	{ *pPF = Control_Point.PF; 	return (Control_Point.PF != -1); }
	int Is_TT_Control(int * pTT)	{ *pTT = Control_Point.TT; 	return (Control_Point.TT != -1); }
	int Is_PH_Control(int * pPH)	{ *pPH = Control_Point.PH;	return (Control_Point.PH != -1); }
	int Is_NR_Control(void)			{ return (Control_Point.NR != -1); }
	int Is_RR_Control(void)			{ return (Control_Point.RR != -1); }
	int Is_EY_Control(void)			{ return (Control_Point.EY != -1); }
	int Is_CL_Control(void)			{ return (Control_Point.CL != -1); }
	int Is_RE_Control(int * pRPM)   { *pRPM = Control_Point.RE; return(Control_Point.RE != -1); }
	int Is_SB_Control(void)			{ return (Control_Point.SB != -1); }
	int Is_UW_Control(int * pUW)	{ *pUW = Control_Point.UW; return(Control_Point.UW != -1); }
	int Is_SM_Control(int * pSM)	{ *pSM = Control_Point.SM; return(Control_Point.SM != -1); }
	
};






class Stitch_List
{
public:
	int				Sew_Line_No ;	 // current sewline
	int				N_Stitches ;
	int			    Fix_APF_Shift_For_Up; // flag for APF shifts
	RTC_Stitch_t	RTC_Stitches[ MAX_RTC_STITCHES];
	
	
	
	void Clear_Stitch_List(void)			// TBD to move to cpp
	{
		static  RTC_Stitch_t	RTC_Stitch_Clear = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
		int		i;
		N_Stitches = 0 ;
		for (i = 0; i < MAX_RTC_STITCHES ; i++ )
			RTC_Stitches[i] = RTC_Stitch_Clear ;
	}
	int Prepare_Stitch_List( SewLine_Def_t & SewLine_Def );  // TBD - to check how ptr to stitch list

	int Handle_Stitch_Mode(PGS_Stitch_t Stitches[], SewLine_Def_t & SewLine_Def);

	int Find_SM_Section (PGS_Stitch_t Stitches[], int & Scan_from, int & Scan_to, int & Start, int & End);

	int Process_Stitches(PGS_Stitch_t Stitches[], SewLine_Def_t & SewLine_Def, double X_Offset, double Y_Offset , PGS_Settings_t & PGS_Setting );

	int Calc_Stitches_RPM(PGS_Stitch_t Stitches[], int First_Stitch, int Last_Stitch, int Zero_RPM, int DryRun , PGS_Settings_t & PGS_Setting ) ;
	
	void  Handle_Stop_Before(SewLine_Def_t & SewLine_Def);
	

};



#endif