// ---------------------------------------------------------------------------
//  Filename:  SEWMAPDef.h
//  Created by: Shmulik Hass
//  Date:  16.08.15
//  Orisol (c) 
// ---------------------------------------------------------------------------

#ifndef _SEWMAPDEF_H_
#define _SEWMAPDEF_H_


#define SEWMAPDEF_VERSION   "SEWMAPDEF 0.0.4"

#define SEWMAP_MAX_PROG					8
#define SEWMAP_MAX_SEWLINES				256
#define SEWMAP_MAX_STITCHES_IN_PROG		9999
#define SEWMAP_MAX_LEADPOINTS_IN_PROG   100

/****************************************************
Please note the following definitions in SEWPGS.h & SEWPGM.h
( current values per today )

		#define MAX_PGS_STITCHES	9999
		#define MAX_SEWLINES		 256
		#define MAX_LEAD_POINTS		 100

		#define MAX_PROGRAM_LOADED     8

*****************************************************/



typedef struct 
{
		int X_Pos ;
		int Y_Pos ;
		int State ;

} 
SEWMAP_Stitch_t ;

typedef struct
{
	int N_Stitches ;			// number of stitches
	int Start_Index ;			// index to first stitch in array
	int End_Index ;				// index to last stitch in array 
	int Skipped ;				// flag to indicate skipped line
	int N_Pre_Lead_Points ;		// Leads points BEFORE the sewline
	int Pre_LeadPoint_Index ;	// index of first pre LP
	int N_Post_Lead_Points ;	// ONLY FOR LAST SEWLINE - Leads points after the SL
	int Post_LeadPoint_Index ;  // index of first post LP. only for last SL !!
	int State ;					// state TBD

} 
SEWMAP_Sewline_Def_t ;



typedef struct
{
	int Map_Exist ;  // if 0 - then no map
	int Prog_Id ; // ID according to sewpgm
	int N_SewLines ;  // number of sewlines. if 0 then 
	int N_Stitches ;
	int N_Leadpoints ;
	int Min_X, Max_X, Min_Y, Max_Y;
	SEWMAP_Sewline_Def_t  Sewline_Def[SEWMAP_MAX_SEWLINES ];
	SEWMAP_Stitch_t       Sewmap_Stitches [SEWMAP_MAX_STITCHES_IN_PROG];
	SEWMAP_Stitch_t       Sewmap_LeadPoints[SEWMAP_MAX_LEADPOINTS_IN_PROG];

} 
SEWMAP_Prog_Def_t ;

typedef struct
{
	int N_Progs ;
	SEWMAP_Prog_Def_t   SewMap_Prog [ SEWMAP_MAX_PROG ] ;
}
SEWMAP_Def_t ;


#endif
