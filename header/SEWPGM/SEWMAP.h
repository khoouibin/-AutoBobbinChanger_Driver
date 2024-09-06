// ---------------------------------------------------------------------------
//  Filename:  SEWMAP.h
//  Created by: Shmulik Hass
//  Date:  16.08.15
//  Orisol (c) 
// ---------------------------------------------------------------------------

#ifndef _SEWMAP_H_
#define _SEWMAP_H_


#define SEWMAP_VERSION   "SEWMAP 0.0.8"



#define SEWMAP_ERROR_NO_PROGRAMS		 39
#define SEWMAP_ERROR_NOT_ENOUGH_MEMORY   40
#define SEWMAP_ERROR_ILLEGAL_PROG_ID	 41
#define SEWMAP_ERROR_TOO_MANY_PROGRAMS         42
#define SEWMAP_ERROR_TOO_MANY_SEWLINES   43
#define SEWMAP_ERROR_TOO_MANY_STITCHES   44
#define SEWMAP_ERROR_TOO_MANY_LEADPOINTS 45


#ifdef  ONS_ERRORS

static Error_Rec_t  SEWMAP_Errors[] =
{
	ERROR_MSG_REC(SEWMAP_ERROR_NO_PROGRAMS),
	ERROR_MSG_REC(SEWMAP_ERROR_NOT_ENOUGH_MEMORY),
	ERROR_MSG_REC(SEWMAP_ERROR_ILLEGAL_PROG_ID),
	ERROR_MSG_REC(SEWMAP_ERROR_TOO_MANY_PROGRAMS),
	ERROR_MSG_REC(SEWMAP_ERROR_TOO_MANY_SEWLINES),
	ERROR_MSG_REC(SEWMAP_ERROR_TOO_MANY_STITCHES),
	ERROR_MSG_REC(SEWMAP_ERROR_TOO_MANY_LEADPOINTS),
		
	ERROR_MSG_REC(0)

};

#endif




typedef struct 
{
	int  Map_Width;		// X Axis
	int  Map_Length ;	// Y axis
	int  Factor ;		// multiplier factor from float to int

	// sewline map profile

	double  Min_D_Threshold;		// Minimal D which means corner can be ignored
	double  Min_D_Straight;		// Minimal D considered as straight
								//double  Min_Len_Diff_Threshold; // Minimal difference in length which should be ignored
	int  Max_Succssive_Removal; // maximal number of sucessive points removal
	int  Max_Stitches;

}
SEWMAP_Profile_t ;


class SewMap
{
public:
	SewMap()
	{
			// default profile of Orisew

		SEWMAP_Profile.Map_Width = 500 * 20 ;
		SEWMAP_Profile.Map_Length = 350 * 20 ;
		SEWMAP_Profile.Factor = 20 ; 
		SEWMAP_Profile.Min_D_Threshold = 2.0 ;		
		SEWMAP_Profile.Min_D_Straight = 0.2;
		SEWMAP_Profile.Max_Succssive_Removal = 2;
		SEWMAP_Profile.Max_Stitches = 100;

			
	}
	SEWMAP_Profile_t SEWMAP_Profile ;

	void SEWMAP_Set_Profile(SEWMAP_Profile_t & SEWMAP_Profile_New)
	{
		SEWMAP_Profile = SEWMAP_Profile_New;
	}
	void SEWMAP_Get_Profile(SEWMAP_Profile_t & SEWMAP_Profile_New)
	{
		SEWMAP_Profile_New = SEWMAP_Profile ;
	}

	int SEWMAP_Load_Map(class Sew_PGM  * p_Sew_PGM, void * Memory_Ptr, long Memory_Size, int Single_Prog, int & Partial_Load);
	char * Get_Version(void)
	{
		static char Version[] = SEWMAP_VERSION;
		return Version;
	}
	void SEWMAP_Convert_XY(double X, double Y, int & XPos, int & YPos);

	//private:
		void Convert_XY ( double X , double Y, int & XPos, int & YPos );
		void NConvert_XY(int X, int Y, double & XPos, double & YPos);
		
	
};

#endif
