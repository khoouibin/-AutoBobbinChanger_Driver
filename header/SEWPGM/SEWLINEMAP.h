// ---------------------------------------------------------------------------
//  Filename:  SEWLINEMAP.h
//  Created by: Shmulik Hass
//  Date:  28.12.16
//  Orisol (c) 
// ---------------------------------------------------------------------------

#ifndef _SEWLINEMAP_H_
#define _SEWLINEMAP_H_




typedef struct
{
	double  Min_D_Threshold;		// Minimal D which means corner can be ignored
	double  Min_D_Straight ;		// Minimal D considered as straight
	//double  Min_Len_Diff_Threshold; // Minimal difference in length which should be ignored
	int  Max_Succssive_Removal ; // maximal number of sucessive points removal
	int  Max_Stitches ;			// Maximal number of stitches required
}
SEWLINEMAP_Profile_t ;


class SewLineMap
{
public:
	SewLineMap()
	{
			// default profile of Orisew

		SEWLINEMAP_Profile.Min_D_Threshold = 2. ; // 0.1 mm
		SEWLINEMAP_Profile.Min_D_Straight = 0.2 ; // 0.01 mm
		//SEWLINEMAP_Profile.Min_Len_Diff_Threshold = 0.04 ; // 0.002 mm difference in length
		SEWLINEMAP_Profile.Max_Succssive_Removal = 2 ; // successiev points can be removed, then the next must stay
		SEWLINEMAP_Profile.Max_Stitches = 100 ; 
	
	}
	SEWLINEMAP_Profile_t SEWLINEMAP_Profile ;

	void SEWLINEMAP_Set_Profile(SEWLINEMAP_Profile_t & SEWLINEMAP_Profile_New)
	{
		SEWLINEMAP_Profile  = SEWLINEMAP_Profile_New;
	}
	void SEWLINEMAP_Get_Profile(SEWLINEMAP_Profile_t & SEWLINEMAP_Profile_New)
	{
		SEWLINEMAP_Profile_New = SEWLINEMAP_Profile ;
	}

	int SEWLINEMAP_Process_Sewline(SEWMAP_Stitch_t Stitches[], int First , int Last );

};

#endif
