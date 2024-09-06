#ifndef _SEWCTRL_H_
#define _SEWCTRL_H_

// send_profile.h
#include "Poco/Task.h"
#include "Poco/TaskManager.h"
#include "Poco/TaskNotification.h"
#include "Poco/NotificationCenter.h"
#include "Poco/NotificationQueue.h"
#include "Poco/Observer.h"
#include "Poco/Exception.h"
#include "notification_pkg.h"
#include <stdio.h>

#include "SEWPGM.h"
#include "json.hpp"

using json = nlohmann::json;

//#define HOME_FINISH 30
//#define HOME_FAIL   29

#define Continue_SewLine_SIM_T        20 //unit: ms.
#define Continue_LoadingPoint_SIM_T  100 //unit: ms.
#define OneStep_SIM_T                 50 //unit: ms.
#define JumpLine_SIM_T               100 //unit: ms.

/*
typedef enum{
	level_1 	= 1200,	//.
	level_2 	= 640,	// 
	level_3 	= 320,	// 
	level_4 	= 160,	//.
	level_5 	= 80,
	level_6 	= 40,
	level_7 	= 20,
	level_8 	= 10,
} dry_spd_t;
*/

typedef enum{
	spd_null= 0,
	spd_up 	= 1,	//.
	spd_dn 	= 2,	// 
	dryspd  = 3,
	sewspd  = 4,
} spdIndexSel_t;

typedef enum{
	sel_SewLine      = 0,
	sel_LoadingPoint = 1,
} pgm_sel_t;

typedef struct
{
	int 	dryrun_usrIndex;	// user SpdIndex: 1~8.
	int 	sewing_usrIndex;	// user SpdIndex: 1~24.
	int 	dryrun_stiRUN;		// sti-RUN, chage spdIndex available.
	int 	sewing_stiRUN;
} spdIndex_t;

//-----------------------------------------
typedef enum{
	_null_ 		= 0,
	_pause_ 	= 1,
	_abort_ 	= 2,
	_next_line  = 3,
	_forw_step  = 4,
	_forw_conti = 5,
	_back_conti = 6,
	_back_step  = 7,
	_prev_line  = 8,
	_sewing     = 9,
} dryrun_ctrl_t;

//---------------------------
typedef struct stiPoint
{
	std::vector<int> SID;
	std::vector<int> PSID;
	std::vector<int> X;
	std::vector<int> Y;
	std::vector<int> RPM;
	std::vector<int> PFH;	//height value of Presser foot.
} stiPoint_t;

typedef struct
{
	int 					line_n;
	std::vector<int> 		lines_size;
	stiPoint_t				sti_bulk;
} stiBulk_t;

typedef struct
{
	stiBulk_t 			bulk_buf;
	std::vector<int>	Start_index;
	std::vector<int>	End_index;
	int 				pre_LineIndex[2];	// [0]:index, [1]:point.
	int 				cur_LineIndex[2];	// index, point.
	int 				pre_Seq;
	int 				cur_Seq;
	int 				total_Seq;
	int 				cur_XY[2];			// x, y.
	dryrun_ctrl_t 		innerCmd;
	int 				fbk_sid;
	bool 				act_sim;
	bool 				sti_tag;
} stiInnerBuf_t;

typedef struct
{
	int 				line_n;
	std::vector<int> 	lines_size;
	std::vector<int>	Start_index;
	std::vector<int>	End_index;
	std::vector<int> 	Offset_index;
	int 				pre_LineIndex[2];	// [0]:index, [1]:point.
	int 				cur_LineIndex[2];
	int 				pre_Seq;
	int 				cur_Seq;
	int 				all_Seq;
	int 				lp_index;		// leading point index.
	int 				sl_index[2];	// sew line index.
	dryrun_ctrl_t 		innerCmd;
	bool 				act_sim;
	int 				fbk_sid;
	bool 				t_pause;
	int 				t_pgsSTOP;		// stop condition: needle up/down...
} SewPGM_InnerIndex_t;
//stiIndex_pgs_t;

typedef struct
{
	int 				pre_LineIndex[2];	// [0]:index, [1]:point.
	int 				pre_Seq;
	int 				cur_XY[2];			// x, y.
	dryrun_ctrl_t 		innerCmd;
} stiFbkHost_t;

typedef struct
{
	int 				usrIndex;			// user SpdIndex: 1~24.
	int 				x_k_1;				// x[k-1].
	int 				y_k_1;				// y[k-1].
	int 				x_k;				// x[k].
	int 				y_k;				// y[k].
	int 				x_k_delta;
	int 				y_k_delta;
	int 				Spd_byIndex;
	int 				Spd_byTrape;
} sewing_ZrpmCal_t;

typedef struct
{
	int 				rpmOutput;
	int 				x_pt;				//pitch time of x
	int 				y_pt;				//pitch time of y
	int 				startAng;
} sewing_paraCal_t;

typedef struct
{
	std::vector<int> 	MA_buf;
	int 				MA_bulk;
	int 				MA_index;
	int 				iir_y_k_1;
} movingAvg_t;

// function.
int StiPointBulk_Gen(void);
int copy_toStiInnerBuf( stiBulk_t* _sour, stiInnerBuf_t* _dest );
void clearStiBuf( stiInnerBuf_t* _dest );
int print_StiBuf(void);

int dryrun_CmdInput( dryrun_ctrl_t cmd_input, stiInnerBuf_t* _dest);
int check_InnerBuf_empty( stiInnerBuf_t* _dest );
int check_line_boundary( dryrun_ctrl_t cmd_input, stiInnerBuf_t* _dest );
int chk_ForBound_points(stiInnerBuf_t* _dest );
int chk_BackBound_points(stiInnerBuf_t* _dest );

int chk_ForBound_line(stiInnerBuf_t* _dest );
int chk_PrevBound_line(stiInnerBuf_t* _dest );

int curLinePointCal( dryrun_ctrl_t cmd_input, stiInnerBuf_t* _dest );
int mov_curLinePoint( dryrun_ctrl_t cmd_input, stiInnerBuf_t* _dest );

int dryrun_onestep_cmd( stiInnerBuf_t* _dest );
void *dryrun_forwconti( void *ptr );
void *dryrun_backconti( void *ptr );
void *sewing( void *ptr );

int dryrun_conti_cmd( int send_i, int* sti_mask, int* stop_tag, stiInnerBuf_t* _dest, sewing_paraCal_t* _sewPara );
int clr_parameter( int* i, int* j, int* stop_tag );
void LineIndexClr( stiInnerBuf_t* _dest );
void update_LineIndex( stiInnerBuf_t* _dest );
void update_LineIndex_Conti( stiInnerBuf_t* _dest );

int check_rtc_status_fbk( int* fbk_id, int* fbk_state );

int ZrpmCal( stiInnerBuf_t* _sour, sewing_ZrpmCal_t* _dest, int _sim );
float rpm2Xy_t( int _rpm );
int ptCal( sewing_ZrpmCal_t* _sour, sewing_paraCal_t* _dest);
int startAngCal( sewing_paraCal_t* _dest );

//-----------------------------------------------------------------------
int rand_gen( int mod_n );
int send_dryrun_info_to_host(stiInnerBuf_t* _sour );
int usrIndexChange( spdIndex_t* _sour, spdIndexSel_t target,spdIndexSel_t updn );

int maClear( movingAvg_t* _ma );
int maCal( movingAvg_t* _ma, int _input );
int rpmTrapezoid( stiInnerBuf_t* _sour, movingAvg_t* _ma, int _input );


// sep04, 2020.
int  SewPGM_ReadPGS( char* file_name );
void SewPGM_Para_printf( class Sew_PGM* sew_pgm );
void SewPGM_LeadPoint_printf(class PGS_Program *PGS_Pgm);

int  SewPGM_LineSel( class Sew_PGM* _sour, int line_num );
void SewPGM_ClrStiList( class Sew_PGM* _sour );
int  SewPGM_LineSizeCal( class Sew_PGM* _sour, SewPGM_InnerIndex_t* _dest);
void SewPGM_IndexClr(SewPGM_InnerIndex_t* _dest);
void SewPGM_SewParaGen( class Sew_PGM* sew_pgm, int ZeroRPM );

int  SewPGM_SIDupdate( int sti_num );
void SewPGM_ReadStiCtrl( class Sew_PGM* _sour, int SID );
int  SewPGM_ReadLeadPointCtrl( class Sew_PGM* _sour, int ld_index );

int  SewPGM_CmdInput( dryrun_ctrl_t cmd_input );
int  SewPGM_BufEmptyCheck( SewPGM_InnerIndex_t* _dest );
int  SewPGM_EdgeCheck( SewPGM_InnerIndex_t* _dest );
int  SewPGM_curLineIndexCal( SewPGM_InnerIndex_t* _dest );

void SewPGM_LineIndexClr( SewPGM_InnerIndex_t* _dest );
pgm_sel_t SewPGM_LP_SL_Selection( SewPGM_InnerIndex_t* _dest );

int  SewPGM_movXY_withAct( int Xpos_i, int Ypos_i, class Sew_PGM* _sour, int lp_index,SewPGM_InnerIndex_t* _dest );
int  SewPGM_movXY_withoutAct( int Xpos_i, int Ypos_i, bool control_bit );
void SewPGM_update_LineIndex( SewPGM_InnerIndex_t* _dest );
void SewPGM_update_LineIndex_Conti( SewPGM_InnerIndex_t* _dest );

int  SewPGM_cur_LineIndex_Execute( SewPGM_InnerIndex_t* _dest );

int  SewPGM_dryrunOneStep( SewPGM_InnerIndex_t* _dest );
void SewPGM_pgsSTOP_contiSEW( SewPGM_InnerIndex_t* _dest );

void *SewPGM_dryrunForwConti( void *ptr );
void *SewPGM_dryrunBackConti( void *ptr );
void *SewPGM_SewingForwConti( void *ptr );
int  SewPGM_dryrunConti( int send_i, int* stop_tag, SewPGM_InnerIndex_t* _dest );

int  SewPGM_dryrunSpeedAdj( spdIndex_t* _sour, spdIndexSel_t updn );
int  SewPGM_sewingSpeedAdj( spdIndex_t* _sour, spdIndexSel_t updn );
int  SewPGM_sewingSpeedChange( int SPDindex );
int  SewPGM_dryrunSpeedChange( int SPDindex );
int  SewPGM_send_info_to_host( SewPGM_InnerIndex_t* _dest, int thread_break );
void SewPGM_setRpmProfile( json profile );
void SewPGM_setXyProfile( json profile );
#endif //_HOMEMODE_H_