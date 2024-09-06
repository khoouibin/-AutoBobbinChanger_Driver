#ifndef _GLOBAL_H_
#define _GLOBAL_H_
#include <iostream>
#include <string>
#include "Msg_Prot.h"
#include "json.hpp"
#include "home_mode.h"
using json = nlohmann::json;

#define SuccessCode 0

typedef enum
{
	UHF = 0,
	SEW = 1,
} projectDef_t;

typedef enum
{
	SEW_1510 = 1510,
	SEW_3020 = 3020,
	SEW_3525 = 3525,
} SEWSIZE_Def_t;

enum Communication_define
{
	//	USBcommSW   = 1, // 1: turn on/ 0:off usb comm. for debug.
	//	RS485__SW   = 1, // 1: turn on/ 0:off 485 comm. for debug.
	//	RS485_debug = 0, // 1: tty_USB(VM), 0:ttyS10(real Single Board Computer)
	PROJ_DEF = UHF,
	SEWSIZE = SEW_3020,
};

// hardware init define.
#define apf_install 1
#define Zaxis_install 1

// move-XY parameter.& wait xy_idle
#define XYdiff_Threshold 900
#define XYratio_Min 60
#define XYratio_sync_Min 20
#define XYratio_orig_Min 10
#define XYidle_delay 270

enum OutputPin_define
{
	UW_Piston_O = 173, //ACT_UT_WELDER_ACT_ENTITY
	UW_Power_O = 171,  //IO_UT_WELDER_DEVICE_ON_ENTITY
	DRIVE_SSR_O = 122, //ACT_MOTOR_SSR_ON_ACT_ENTITY, (UHF useless)
	CLAMP_O = 136,	   //ACT_CLAMP1_3S_ACT_ENTITY
	CLAMP_OPEN_UHF = 42,
	CLAMP_CLOS_UHF = 33,
	//CLAMP_ACT_UHF  =  42,
	UW_Z_UP_UHF = 25,
	UW_Z_DN_UHF = 153,
	UW_POW_UHF = 31,
};

enum DiagOutputPin_define
{
	SSR_O = 43, //IO_MOTOR_SSR_ON_ENTITY
	//ReadyLamp_O = , //
	CLAMP_L_O = 33, //IO_CLAMP_LEFT_LOCK_ENTITY
	CLAMP_R_O = 34, //IO_CLAMP_RIGHT_LOCK_ENTITY
	X_ON = 0,		//IO_X_SERVO_ON_ENTITY
	X_STEP = 1,		//IO_X_STEP_ENTITY
	X_DIR = 2,		//IO_X_DIRECTION_ENTITY
	Y_ON = 3,		//IO_Y_SERVO_ON_ENTITY
	Y_STEP = 4,		//IO_Y_STEP_ENTITY
	Y_DIR = 5,		//IO_Y_DIRECTION_ENTITY
};

enum SSR_define
{
	SSR_ON = 1,
	SSR_OFF = 0,
};

enum CLAMP_RL_define
{
	CLAMP_L_CLOSE = 1, //light up.
	CLAMP_L_OPEN = 0,  //light off.
	CLAMP_R_CLOSE = 0, //loght up.
	CLAMP_R_OPEN = 1,  //light off.
};

enum XY_define
{
	XY_ON_ON = 1,	 //light up.
	XY_ON_OFF = 0,	 //light off.
	XY_STEP_ON = 1,	 //loght up.
	XY_STEP_OFF = 0, //light off.
	XY_DIR_ON = 0,	 //loght up.
	XY_DIR_OFF = 1,	 //light off.
};

enum UW_define
{
	UW_UP = 1,
	UW_DOWN = 0,
	UW_pos_check = 0,
	UW_TOPsen_MASK = 1024,
	UW_BOTsen_MASK = 2048,
	UW_PWRoff = 0,
	UW_PWRon = 1,
	UW_Z_TIMEOUT = 500,
	UW_PWRsen_MASK = 2048,
};

enum CLAMP_define
{
	CLOSE = 0,
	OPEN = 1,
	CLAMP_pos_check = 1,
	CLAMP_MASK = 0x0040,
	CLAMP_TIMEOUT = 500,
	CLAMP_MASK_UHF = 0x0008,
};

// clamp open/close, mar27,2020.
//#define CLAMP_OP 1
//#define CLAMP_CL 0
//#define CLAMP_MASK 0x0040

//protocol handshake define.
#define sh_ACT1_seq_1 0x0010 // there has some risks, needs to modify.
#define sh_ACT1_seq_2 0x0308 // there has some risks, needs to modify.
#define sh_ACT1_seq_3 0xc808 // there has some risks, needs to modify.

#define table_SERVO_seq_1 0x0013 // there has some risks, needs to modify.(cancelled)
#define table_SERVO_seq_2 0x001b // there has some risks, needs to modify.(cancelled)
#define table_lockMASK 0x0008
//x-y table boudary.
#define Xpos_MAX 13900
#define Ypos_MAX 8175
#define Xpos_MIN 0
#define Ypos_MIN 0
#define xy_scal_min 1
#define xy_scal_max 100
#define XYIDLE_timeout 1000 //unit: ms.

// mechanical movement area:
// x = 680mm. ( 13840 - 240 = 13600 / 20 = 680mm )
// y = 405mm. ( 8150  - 50  = 8100  / 20 = 405mm )
// working area:
// x = 670mm.
// y = 400mm.
// calbration range:
// calibration x = 10mm = 200cnt.
// calibration y =  5mm = 100cnt.

#define Xcali_MAX 200
#define Ycali_MAX 100
#define Xwork_MAX 13600 // 13600 = 680mm * 20.
#define Ywork_MAX 8000	//  8000 = 400mm * 20.

#define Xuncal_CEN 1600
#define Yuncal_CEN 4100

// RTC alarm message: Msg_Prot_Unpack_Alarm_Info.
#define _airpre_ALARM_mask 0X01
#define _xMotor_ALARM_mask 0X02
#define _yMotor_ALARM_mask 0X04
#define _zMotor_ALARM_mask 0X08
#define _LoadSaf_ALARM_mask 0X10

typedef struct
{
	int checking_usb_state;
} comm_data;

typedef struct
{ // id - 10
	bool req;
	RTC_Stitch_t stitchData;
	int flush_buf;
} next_stitch_data;

typedef struct
{ // id - 11
	bool req;
	int Stitch_ID;
	int Current_RPM;
	int Current_X_Pos;
	int Current_Y_Pos;
	int Target_RPM;
	int Target_X_Pos;
	int Target_Y_Pos;
	int XY_Moving;
	int Sew_Head_Rotating;
	int RTC_State;
	int Buff_Stitches_Num;
	int HW_State;
	int Reason_Code;
	RTC_Action_Start_Report_t Action_Started;
	ONS_RTC_Status_Error_type_t Error_Code;
} rtc_status_data;

typedef struct
{ // id - 11
	int Stitch_ID;
	int Current_RPM;
	int Current_X_Pos;
	int Current_Y_Pos;
	int Target_RPM;
	int Target_X_Pos;
	int Target_Y_Pos;
	int XY_Moving;
	int Sew_Head_Rotating;
	int RTC_State;
	int Buff_Stitches_Num;
	int HW_State;
	int Reason_Code;
	RTC_Action_Start_Report_t Action_Started;
	ONS_RTC_Status_Error_type_t Error_Code;
} RTCStatus_11;

typedef struct
{ // id - 11
	unsigned int msg_num;
	std::vector<int> StiID;
	std::vector<int> CurrRPM;
	std::vector<int> CurrX;
	std::vector<int> CurrY;
	std::vector<int> TargRPM;
	std::vector<int> TargX;
	std::vector<int> TargY;
	std::vector<int> XYMoving;
	std::vector<int> SHRoting;
	std::vector<int> RTCState;
	std::vector<int> BufStiNum;
	std::vector<int> HW_State;
	std::vector<int> ReasCode;
	std::vector<RTC_Action_Start_Report_t> ActStart;
	std::vector<ONS_RTC_Status_Error_type_t> ErrCode;
	std::vector<int> HWState_XYjump;
} RTCStatus_vec;

typedef struct
{ // id - 11
	bool req;
	int Current_X_Pos;
	int Current_Y_Pos;
	int Target_RPM;
	int Target_X_Pos;
	int Target_Y_Pos;
} rtc_status_xy;

typedef struct
{ // id - 13
	int ErrorCode;
	char ErrorData[30];
} error_message_data;

typedef struct
{ // id_14
	int X_poscmd;
	int Y_poscmd;
	int X_scale;
	int Y_scale;
	int X_poscmd_previous;
	int Y_poscmd_previous;
} movXY_data;

typedef struct
{ // id - 15
	int HomeProcedure;
	int readySend;
	Msg_Prot_Home_Mode_t Home_Mode;
	Msg_Prot_Home_Z_Direction_t Home_Zdir;
	int movZdistance;
	int Home_2;
} home_mode_data;

typedef struct
{ // id - 16
	bool req;
	int Xposition;
	int Yposition;
	int XYmoving;
	int SewHeadRotating;
	int ErrorCode;
	RTC_State_t RTC_homemode;
	RTC_Home_Result_t HomeRes;
} RTC_home_status_data;

typedef struct
{ // id - 26
	SH_UP_Pos_t state;
	int value;
} locksh_data;

typedef struct
{ // id - 25
	bool req;
	INT_16 RTC_init_status;
} init_data;

typedef struct
{
	Status_Reply_Mode_t reply_mode;
	int time_period;
	unsigned int ID;
	bool trig;
} machine_req;

typedef struct
{ // id - 51
	bool req;
	int M_S_Msg_ID;
	int Act_1;
	int Act_2;
	int Act_3;
	int Act_4;
	int Act_5;
	int IO_1;
	int IO_2;
	int IO_3;
	int Servo_Ind;
} machine_status_data;

typedef struct
{ // id - 51
	unsigned int msg_num;
	std::vector<int> ACT1_PF;
	std::vector<int> ACT1_APF;
	std::vector<int> ACT1_APF_POS;
	std::vector<int> SerInd_SHLOCK;
	std::vector<int> SerInd_SHROTATE;
	std::vector<int> SerInd_XYLOCK;
	std::vector<int> SerInd_XYMOVING;
} MachineStat_vec;

typedef struct
{ // id - 52
	ONS_Up_Down_State_t pf_pos;
	int apf_val;
} apf_setposition_data;

typedef struct
{ // id - 58
	Msg_Prot_Entity_Data_t Entity[8];
	Msg_Prot_Entity_List_t EntityAll;
} entity_data;

typedef struct
{ // id - 60
	bool resp;
	int EntityGet_num;
	Msg_Prot_Entity_Data_t EntityGet[8];
} entityget_data;

typedef struct
{ // id - 61
	int tension_val;
} tensionset_data;

typedef struct
{ // id - 62, 63
	bool req;
	INT_16 xy_position_loading;
} xypos_response;

typedef struct
{ // id - 65
	bool tb_en;
	bool pallet_en;
	bool air_press_en;
	bool wiper_en;
	bool apf_chk_en;
} config_cmd_data;

typedef struct
{ // id - 67
	bool req;
	UINT_16 IO_List1;
	UINT_16 IO_List2;
	UINT_16 IO_List3;
	UINT_16 IO_List4;
	UINT_16 IO_List5;
	UINT_16 IO_List6;
	UINT_16 IO_List7;
	UINT_16 IO_List8;
} iolist_data;

typedef struct
{ // id - 70
	int Alarm_Mask;
	int Enalbe_State;
} alarm_data;

typedef struct
{ // id - 51
	int M_S_Msg_ID;
	int Act_1;
	int Act_2;
	int Act_3;
	int Act_4;
	int Act_5;
	int IO_1;
	int IO_2;
	int IO_3;
	int Servo_Ind;
} MachineStatus_51;

typedef struct
{ // id - 76
	bool req;
	int M_S_Msg_ID;
	int SpoolCounters_Status;
	int BobbinCounter_Value;
	int WinderCounter_Value;
	int ThreadCounter_Value;
	int Tension_Value;
	int BobbinChanger_Status;
	int BobinChanger_ErrorCode;
	int BobbinChanger_Step;
	int Zservo_Pos;
} machine_info_data;

typedef struct
{ // id - 76
	int M_S_Msg_ID;
	int SpoolCounters_Status;
	int BobbinCounter_Value;
	int WinderCounter_Value;
	int ThreadCounter_Value;
	int Tension_Value;
	int BobbinChanger_Status;
	int BobinChanger_ErrorCode;
	int BobbinChanger_Step;
	int Zservo_Pos;
} MachineInfo_76;

typedef struct
{ // id - 76
	unsigned int msg_num;
	std::vector<int> SpoolCounters_Status;
	std::vector<int> BobbinCounter_Value;
	std::vector<int> WinderCounter_Value;
	std::vector<int> ThreadCounter_Value;
	std::vector<int> Tension_Value;
	std::vector<int> BobbinChanger_Status;
	std::vector<int> BobinChanger_ErrorCode;
	std::vector<int> BobbinChanger_Step;
	std::vector<int> Zservo_Pos;
} MachineInfo_vec;

typedef struct
{
	bool req;
	RTC_State_t RTC_Status;
	RTC_Diag_Act_Result_t DiagActRes;
	int x_pos;
	int y_pos;
	int xy_moving;
	int sewhead_moving;
	int error_code;
} DiagAct_t;

typedef struct
{
	bool home_ready;
} ctrl_movXY_data;

typedef struct
{
	int time_pre;
	int time_now;
	int time_diff;
} timestamp_t;

typedef struct
{
	unsigned int t_n[2];   // t[n-1], t[n].
	unsigned int delta_tn; // delta_t[n] = t[n] - t[n-1];
} timetag_t;

typedef struct
{
	unsigned int t_n[2];   // t[n-1], t[n].
	unsigned int delta_tn; // delta_t[n] = t[n] - t[n-1];
} utimetag_t;

typedef enum
{
	tag_all = 0,   //time tag t[0] & t[1].
	tag_now = 1,   //time tag t[1].
	tag_delta = 2, //delta time calculation.
} timetagsel_t;

/*
typedef struct
{
	int x_welding_points[1024];
	int y_welding_points[1024];
	int x_loading_point;
	int y_loading_point;
	int point_num;
} welding_pkg_t;
*/

// enum parameters.
typedef enum
{
	latch_on = 1, //diag in.
	latch_off = 0 //diag exit.
} ONS_Diag_LatchCmd_t;

typedef enum
{
	homesensor_x = 0,
	homesensor_y = 1
} ONS_xyHomeSensor_t;
typedef enum
{
	Motor_Y = 1,
	Motor_X = 2,
	Motor_Z = 3,
} MotorNumber_t;

//-----------------------------------------------------------------------------
// xy-table position infomation.
typedef struct
{
	int singleTurn;
	int multi_Turn;
} motor_turn;

typedef struct
{
	motor_turn Xturns;
	motor_turn Yturns;
} XY_positiondata;

typedef struct
{ // calculate how far from previous point.
	int X_prev;
	int Y_prev;
	int X_pres;
	int Y_pres;
	int XY_absdist;
} XY_dist;

//-----------------------------------------------------------------------------
//	rs485 communication
#define POS_REQ_FINISH 10

/*
typedef struct
{
	bool req;
	bool occupy;		// if start = 1, other function cannot use rs485.
	int  rs485_seq;		//tx and rx procedure sequence.
	int  receiv_seq;	//
	char read_buf[32];	//buf to merge.
	char read_buf_idx;	//buf index.(length of merged buffer)
	char write_chk[2];	//check tx,rx error code.
}rs485_reciv_data;
*/

typedef struct
{
	unsigned char singleL;
	unsigned char singleM;
	unsigned char singleH;
	unsigned char multi_L;
	unsigned char multi_H;
	int s_turn;
	int m_turn;
} turns_t;

typedef struct
{ // error message from motor driver through rs485.
	unsigned char main_code;
	unsigned char sub_code;
} errorcode_t;

//-----------------------------------------------------------------------------
// login process
typedef struct
{
	int readySend;
	int process_N;
	int login_seq;
} login_process_t;

//-----------------------------------------------------------------------------
// External Input Emergency
typedef struct
{
	int stop_button;
	int air_pressure;
	int fan_machinebox;
} external_input;

//-----------------------------------------------------------------------------
// dryrun speed
typedef enum
{
	spd_1 = 1200,
	spd_2 = 640,
	spd_3 = 320,
	spd_4 = 160,
	spd_5 = 80,
	spd_6 = 40,
	spd_7 = 20,
	spd_8 = 10,
} dryrun_speed_t;

typedef struct
{
	bool dryrun_pthreadON;
	int dryrun_id11_dispOFF;
	int alarm__id56_interrupt;
	int error__id13_interrupt;
} dryrun_control_t;

typedef struct
{
	bool req;
	int x_k_1;
	int x_k;
	int y_k_1;
	int y_k;
	int xy_k;
	int d_x;
	int d_y;
	int z_rpm;
	int x_pulse_interval;
	int y_pulse_interval;
	int startAng;
} sti_control_t;

typedef struct
{ // uhf movement test.
	int cycle;
	int points;
	std::vector<int> Xpos;
	std::vector<int> Ypos;
	int bk;
} uhf_test;

typedef struct
{ // uhf xy movement coordination.
	int X_cmd;
	int Y_cmd;
	int X_delta;
	int Y_delta;
	int X_rtc_pos; // to rtc value.
	int Y_rtc_pos;
} uhf_calib_t;

typedef enum // 5/3_SOLENOID_POWER_VALVES(五口三位電磁閥)
{			 // 無桿缸
	ACT_RIGHT = 0,
	ACT_LEFT = 1,
	CYLIN_Out1 = 22,
	CYLIN_Out2 = 152,
} UHF_Cylinder_t;

typedef enum
{
	ACT_UP = 0,
	ACT_DOWN = 1,
	PALLET_Out3 = 20,
	PALLET_Out4 = 21,
} UHF_Pallet_t;

typedef enum
{
	ACT_CLOSE = 0,
	ACT_OPEN = 1,
	CLAMP_Out1 = 33,
	CLAMP_Out2 = 42,
} UHF_Clamp_t;

typedef enum // 5/3_SOLENOID_POWER_VALVES(五口三位電磁閥)
{
	ACT_UWUP = 0,
	ACT_UWDN = 1,
	WELD_Out1 = 25,
	WELD_Out2 = 153,
} UHF_WeldingZ_t;

typedef enum
{
	ACT_UWPWROFF = 0,
	ACT_UWPWRON = 1,
	PWR_Out1 = 31,
} UHF_WeldingPWR_t;

typedef enum
{
	ACT_LightOFF = 0,
	ACT_LightON = 1,
	LightR_sel = 40,
	LightL_sel = 37,
} UHF_Lighting_t;

typedef enum
{
	Servo_OFF = 0,
	Servo_ON = 1,
	Xservo_Out = 0,
	Yservo_Out = 3,
} UHF_xyServoSWitch_t;

typedef struct
{
	int air_pres;
	int motor_x;
	int motor_y;
	int motor_z;
	int loader_safety;
} rtc_AlarmPart_t;

typedef struct
{
	Poco::Event   g_oneshotfbk;
	char status;
}entity_oneshot_t;

//-----------------------------------------------------------------------------
// function.
//int rtc_driver_exit( void );
int time_stamp(void);
void timestamp_delay(int _delay_time);
int timestamp_diff(int _now, int _pre);

int time_tag(timetagsel_t _typ, timetag_t *t_tag);
int utime_tag(timetagsel_t _typ, utimetag_t *t_tag);

int Read_RTC_Status(ONS_BYTE *Msg_Buff);
int Read_RTC_MachineStatus(ONS_BYTE *Msg_Buff);
int Read_RTC_MachineInfo(ONS_BYTE *Msg_Buff);
void Disp_RTC_Status(void);

//-----------------------------------------------------------------------------
int Init_request(int _req, int _id);
int rtc_status_check(int _req, int _id);

int Read_RTC_MachineStatus(void);
int Machine_info_check(int _req, int _mode, int _period, int _id);
int usbbuf_status_check();
void set_cli_usb_check(int var);
int read_cli_usb_check(void);
//-----------------------------------------------------------------------------
int entity_ctrl(int _IO, int _CMD);
int entity_get_check(int _req, int _entityNum, int _id);
//-----------------------------------------------------------------------------
int alarm_enable(int alarm_target_bit);
int alarm_reset(int alarm_target_bit);
int config_set(bool _tb, bool _pal, bool _air, bool _wip, bool _apfchk);
//-----------------------------------------------------------------------------
int error_disp(int _req, int _id);
int Read_RTC_AlarmMessage(ONS_BYTE *Msg_Buff);
int read_AlarmFlag(void);
int MotorDrive_ClrAlarmAll(void);
//-----------------------------------------------------------------------------
motor_turn xyTab_OrginTurnReq(int _Driv_addr);
int xyTab_PresentTurnReq(int _Driv_addr, motor_turn *pTurn);
int xyTab_SingleTurnReq(int _Driv_addr, motor_turn *pTurn);
int xyTab_MultiTurnReq(int _Driv_addr, motor_turn *pTurn);
int xyTab_MultiTurnClrReq(int _Driv_addr);
//-----------------------------------------------------------------------------
int xyTab_WarningClrReq(int _Driv_addr);
int xyTab_WarningReq(int _Driv_addr, errorcode_t *errcode);
int xyTab_rs485Receiv(char _str[]);
//-----------------------------------------------------------------------------
int XY_LockRelease(int _req);
int xypostion_response(int _id);
int XY_Value_predict2RTC(int *_x, int *_y);
int signExten(int instr);
int Z_ValInvPredict(int z_enco_val, int *z_dist, int *z_dir);
//-----------------------------------------------------------------------------
int uw_piston(int _act);
int uw_power(int _act);
int uw_powerToggle(int delay);
int uw_pistonToggle(void);

//-----------------------------------------------------------------------------
int wait_act_down(int _timeout);
int wait_act_up(int _timeout);

//int uw_mov_w0_Act(int _x, int _y, int _uw_power);
//int uw_mov_w0_Act_updn_delay( int _x, int _y, int _uw_power, int _uw_dn_delay, int _uw_up_delay );
//int uw_mov_w1_Act(int _x, int _y, int _w_delay, int _xy_limit);
//int uw_mov_w2_Act(int _x, int _y, int _w_delay, int _xy_limit);
//int uw_mov_w1_ActTest(int _x, int _y, int _uw_power, int _xy_limit);
//int uw_mov_w2_ActTest(int _x, int _y, int _uw_power, int _xy_limit);
//int uw_demo_test(int _uw_power, int _xy_threshold);
//int uw_demo2_test(int _uw_power, int _xy_threshold);
//int uw_demo3_test( int _uw_power );
//-----------------------------------------------------------------------------
int clamp_act(int _act);
//	int wait_xy_idle(void);

//	welding_pkg_t *get_welding_pkg();
//	void dump_welding_pkg();
//	void init_welding_pkg(std::string &str_pgs);

//	int IO_List_Req(int mode);
//	int iolist_disp(int _req, int _id);
//	int diag_LatchMode(ONS_Diag_LatchCmd_t _cmd);
//	int diag_singleHome(ONS_Diag_homingXY_t _cmd);
//	int diag_singleHomeExit(void);
//	int diag_singleHomeAbort(ONS_Diag_homingXY_t _cmd);

//	int diag_into( void );
//	int diag_out( void );
//	int diag_singleHomeY(void);
int HomeSensor_SignalGet(ONS_xyHomeSensor_t _sensor);
//int modbus_xyPosCal( void );
int modbus_xyPosCal(int *_x_val, int *_y_val);
int modbus_xyPos_UHFmirror(int *_x_val, int *_y_val);

int SetRtcIoStatusConfig();
int SetRtcIoStatusReplyMode(Status_Reply_Mode_t eReplyMode, int iPeriodMs);

int External_InputCheck(machine_status_data _input);

//int APF_up(int _apf_timeout);
//int APF_down(int _apf_timeout);
//int movXY_apf( int _Xin, int _Yin);
int wait_xy_idleVar(int _m_timeout);

int dryrun_demo(dryrun_speed_t _dryspeed, int _num);
int stitch_demo(int _num);
int hypotenuse_cal(int x_k_1, int x_k, int y_k_1, int y_k);
int z_rpm_hypo_cal(int hypo_val);
int pulse_interval_cal(int z_rpm, int d_x, int d_y, int *x_pulse_int, int *y_pulse_int);
int staAng_cal(int z_rpm);

int test(void);
timespec diff(timespec start, timespec end);

int xy_ServoSwitch(UHF_xyServoSWitch_t switch_input);
void clr_screen(void);

int st_reset(void);
int xy_reset(void);

int cylinder_RightLeftSwitch(UHF_Cylinder_t _in);
int pallet_UpDnSwitch(UHF_Pallet_t _in);
int clamp_ClosOpenSwitch(UHF_Clamp_t _in);
//int  piston_WeldSwitch( UHF_WeldingZ_t _in );
int power_WeldSwitch(UHF_WeldingPWR_t _in);
int piston_WeldSwitch(UHF_WeldingZ_t _in, int UWZ_timeout);
int light_Switch(UHF_Lighting_t _sel, UHF_Lighting_t _ONOFF);
int MotorReadyCheck(void);

//int moving_test( int Inc, int m_sync, int pis_act );
int moving_test(int Inc, int pis_act, int x_offset, int y_offset);
int moving_test2(int Inc, int pis_act);
int moving_test3(int Inc, int pis_act, int sync);
int moving_test4(int Inc, int pis_act);
int movSpd_test(int Inc);

//	int  movXY_value(int _Xval, int _Yval, int _Xscal, int _Yscal);          // x-y original.

int xy_boundary_check(int xin, int yin);
void xy_scale_check(void);

int moving_st_test(int Inc);

int movXY_original(int xpos, int ypos, int x_scal, int y_scal);
int movXY_spd_100(int xpos, int ypos);
int movXY_spd_ratio(int xpos, int ypos, int ratio);

int movXY_sync(int _Xin, int _Yin); // x-y sync.
int movXY_syncSpdratio(int xpos, int ypos, int ratio_in);

int moveXY_mirror(int x_in, int y_in, int x_mirror, int y_mirror, int spd_ratio);
int moveXY_mirrorSync(int x_in, int y_in, int x_mirror, int y_mirror, int spd_ratio);

void xyTab_Clear_OrginMultiTurn(MotorNumber_t motor);
int xyTab_Set_OrginSingleTurn(MotorNumber_t motor, motor_turn present);

int GetDriverVersion(char *cpBuffer, int iByteOfBuf);

int OneShot_Control(int entity_num, int millisec);
int Read_RTC_EntityOneShotStatus(ONS_BYTE *Msg_Buff);
void entity_oneshot_wakeup(void);


int unsignedtest(void);

#endif //_GLOBAL_H_
