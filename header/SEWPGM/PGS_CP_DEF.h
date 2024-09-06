//----------------------------------------------------------------------------
//  Filename: PGS_CP_DEF.h
//  Created by: Shmuel Hass
//  Date:  28/06/16  updated 14/10/18, 6/12/18
//  Orisol (c)
// ---------------------------------------------------------------------------
#ifndef _PGS_CP_DEF_H_
#define _PGS_CP_DEF_H_


// Section Names
#define  PGS_SEW_PORGRAM_SECTION	"[Sew_Program]"		// Section of Sew Prgram Header
#define  PGS_STITCH_POINTS_SECTION   "[Stitch_Points]"   // Section of Stitch list
#define  PGS_END_SECTION			"[End]"				// Section of PGS END
#define  PGS_LEAD_POINTS_SECTION	"[Lead_Points]"	// Section of Lead points

#if(0)		
//Lead points format
Jump_1=n,6.1,73.1 
	sp : The system will stop at this lead point.
	ph(n) : The system will active the puncher for n times and punch a hole at this point.
	ey : The machine with Eyelet will active the eyelet and put an eyelet at this point.
	op(xxxx) : An I / O output point is defined in this point(xxxx - means status of each I / O numbers, 1 is ON, 0 is OFF and total I / O outputs are 4).
#endif

// Control points definition 



// SS  - Sewline start - ss(n_sbt) - number of backtach at start (0-9) - only valid at first stitch of SL

#define CP_SS_STR		"ss"
#define CP_SS_STR_LEN	2
#define CP_SS_MIN		0
#define CP_SS_MAX		9

// SE  - Sewline end - se(n_ebt) - number of backtach at end (0-9) - only valid at end stitch of SL

#define CP_SE_STR		"se"
#define CP_SE_STR_LEN	2
#define CP_SE_MIN		0
#define CP_SE_MAX		9

// BK -  Backtach mode - bk(m) - mode 0-close, 1-one pass, 2-two pass, 3-repated - only valid at first stitch of SL

#define CP_BK_STR		"bk"
#define CP_BK_STR_LEN	2
#define CP_BK_MIN		0
#define CP_BK_MAX		3
#define CP_BK_DEFAULT	1


// SK - sewline to be skipped	- only valid at first stitch of SL. 

#define CP_SK_STR		"sk"
#define CP_SK_STR_LEN	2


// SGZ - global stitch size  -  sgz(m.m) - size in mm - Editor ONLY

#define CP_SGZ_STR		"sgz"
#define CP_SGZ_STR_LEN  3
#define CP_SGZ_FACTOR	10
#define CP_SGZ_MIN		1
#define CP_SGZ_MAX		120


// SZ - stitch size -  sz(m.m) size of stitch size in mm - Editor Only

#define CP_SZ_STR		"sz"
#define CP_SZ_STR_LEN	2
#define CP_SZ_FACTOR	10
#define CP_SZ_MIN		1
#define CP_SZ_MAX		120


// RPM - RPM limit - rpm(r) - rpm limit 120 - in affect all program from that point

#define CP_RPM_STR		"rpm"
#define CP_RPM_STR_LEN  3
#define CP_RPM_MIN		120
#define CP_RPM_MAX		3000

// SP - stop point.    also at lead points

#define CP_SP_STR		"sp"
#define CP_SP_STR_LEN	2


// ST - stop point with time limit.   st(tttt) - tttt - delay time in 0.01 sec

#define CP_ST_STR		"st"
#define CP_ST_STR_LEN	2
#define CP_ST_MIN		0
#define CP_ST_MAX		9999


// WT - stop and wait for input. wt(ii-0) or wt(ii-1), wait for port ii to become 0 or 1

#define CP_WT_STR		"wt"
#define CP_WT_STR_LEN	2
#define CP_WTI_MIN		0
#define CP_WTI_MAX		20


// CR - corner point ( editor only )

#define CP_CR_STR		"cr"
#define CP_CR_STR_LEN	2


// NT -  NO trimming with stop at end of sewline - in affect only on current SL - (last NT/NS is in effect) - now on first stitch only

#define CP_NT_STR		"nt"
#define CP_NT_STR_LEN	2


// NS - No trimming without stop at end of sewline - in affect only on current SL - (last NT/NS is in effect) - now on first stitch only

#define CP_NS_STR		"ns"
#define CP_NS_STR_LEN	2


// NU - Stop and keep needle up

#define CP_NU_STR		"nu"
#define CP_NU_STR_LEN	2

// ND - Stop and keep needle down

#define CP_ND_STR		"nd"
#define CP_ND_STR_LEN	2


// OP - output to port - op(xxxx) - 4 bit values- also Lead points

#define CP_OP_STR		"op"
#define CP_OP_STR_LEN	2
#define CP_OP_NBITS		4


// OPX - output to port - opx(xxxxxxxxxxxxxxxx) - 16 bit values -0/1/X - also lead points

#define CP_OPX_STR		"opx"
#define CP_OPX_STR_LEN  3
#define CP_OPX_NBITS	16

// PF - press Foot height - pf(n.n) height in  mm - in affect all program from that point

#define CP_PF_STR		"pf"
#define CP_PF_STR_LEN	2
#define CP_PF_FACTOR	10
#define CP_PF_MIN		00
#define CP_PF_MAX		99


// TT - thread tension - tt(ttt) - tt value of thread tension - in affect all program from that point

#define CP_TT_STR		"tt"
#define CP_TT_STR_LEN	2
#define CP_TT_MIN		0
#define CP_TT_MAX		127


// PH - number of punching - ph(x) - x number of punches - relevant only to leads points ( defined in seperate section)

#define CP_PH_STR		"ph"
#define CP_PH_STR_LEN	2
#define CP_PH_MIN		1
#define CP_PH_MAX		3

// NR - start sewline without rotation (OSEW only) -  TBD  scope of affect

#define CP_NR_STR		"nr"
#define CP_NR_STR_LEN	2


// RR - end sewline without rotation (OSEW only) -  TBD  scope of affect

#define CP_RR_STR		"rr"
#define CP_RR_STR_LEN	2

// EY - activate Eyelet - - relevant only to leads points ( defined in seperate section)

#define CP_EY_STR		"ey"
#define CP_EY_STR_LEN	2

// CL - Open clamp - relevant only for Stop point ( SP, ND, NU ). If exist it means to open Clamp after stop

#define CP_CL_STR		"cl"
#define CP_CL_STR_LEN	2

// RE - Reduced RPM - define that current stitch is Reduced speed point.

#define CP_RE_STR		"re"
#define CP_RE_STR_LEN	2
#define CP_RE_MIN		0
#define CP_RE_MAX		3000


// SB - Stop Before - define that current sewline should be stopped before first stitch - only valid at first stitch of SL. 

#define CP_SB_STR		"sb"
#define CP_SB_STR_LEN	2

// UW - UT Welder Action - UW(level) define welder action operation with action level translated to time period - only valid at lead point

#define CP_UW_STR	   "uw"
#define CP_UW_STR_LEN   2
#define CP_UW_MIN       1
#define CP_UW_MAX       25

// SM - Stitchig mode - sm(mode) support new modes of stitching for a range of stitches ( in single sewline). mode 1 is FBF-FBF, mode 2 is reserved, mode 0 is normal

#define CP_SM_STR	   "sm"
#define CP_SM_STR_LEN   2
#define CP_SM_MIN       0
#define CP_SM_MAX       2


#endif