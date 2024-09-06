#ifndef _HOMEMODE_H_
#define _HOMEMODE_H_

// send_profile.h
#include "Poco/Task.h"
#include "Poco/TaskManager.h"
#include "Poco/TaskNotification.h"
#include "Poco/NotificationCenter.h"
#include "Poco/NotificationQueue.h"
#include "Poco/Observer.h"
#include "Poco/Exception.h"
#include "notification_pkg.h"

typedef enum
{
	STATE_NULL    = 0,
	STATE_X_PHASE = 2,
	STATE_Y_PHASE = 3,
} DiagAct_MotorSel_t;

typedef struct
{
	bool test_start;
	bool test_break;
	int test_seq;
} home_testing_t;

typedef enum
{
	HOME_STATE_READY = 0,
	HOME_STATE_Z = 1,
	HOME_STATE_MANUAL_Z = 2,
	HOME_STATE_PFS = 3,
	HOME_STATE_XY_PHASE_1 = 4,
	HOME_STATE_XY_PHASE_2 = 5,
	HOME_STATE_INIT = 6,
	HOME_STATE_EXIT = 7,
	HOME_STATE_ABORT = 8
} Home_State_t;

typedef enum
{
	DIAG_ACT_STATE_READY = 0,
	DIAG_ACT_LOCK_AT_INDEX = 1,
	DIAG_ACT_STATE_X_PHASE = 2,
	DIAG_ACT_STATE_Y_PHASE = 3,
	DIAG_ACT_STATE_INIT = 4,
	DIAG_ACT_STATE_EXIT = 5,
	DIAG_ACT_STATE_ABORT = 6
} Diag_Act_State_t;

typedef struct
{
	Poco::Event g_home_DiagAct_idle;
	Poco::Event g_home_xy_idle;
	Poco::Event g_home_xyz_idle;
	Poco::Event g_home_msg_idle;
	Poco::Event g_diagact_msg_idle;
	Poco::Event g_read_stmode_idle;
	int g_homeprocess_timeout_ms;
	int g_homemsgfbk_timeout_ms;
	Home_State_t g_driver_StateCmd;				// send command.
	Home_State_t g_rtc_StateFbk;				// rtc state feedback.
	Diag_Act_State_t g_driver_DiagAct_StateCmd; // send command.
	Diag_Act_State_t g_rtc_DiagAct_StateFbk;	// rtc state feedback.
	DiagAct_MotorSel_t g_diagact_motor_sel;
	int g_home_start;
	int g_home_abort_trigger;
	int g_start_read_stmode;
	int g_x_stmode_trigger;
	int g_y_stmode_trigger;
} HomeCtrl_t;

typedef struct
{
	RTC_State_t RTC_HomeMode_fbk;
	int Xpos_fbk;
	int Ypos_fbk;
	int XYmoving_fbk;
	int SewHead_Rotating_fbk;
	RTC_Home_Result_t Home_Res_fbk;
	int RTC_Err_fbk;
} Home_RTC_Status_t;

typedef struct
{
	RTC_State_t RTC_DiagAct_fbk;
	int Xpos_fbk;
	int Ypos_fbk;
	int XYmoving_fbk;
	int SewHead_Rotating_fbk;
	RTC_Diag_Act_Result_t DiagAct_Res_fbk;
	int RTC_Err_fbk;
} DiagAct_RTC_Status_t;

typedef struct
{
	int virtualRTC_sel; // 0:real, 1:virtual.
} Home_parameter_t;

// function.

//void *homing_process(void *ptr);
int home_abort(void);
int homing_test(int Inc);

void *home_xy(void *ptr);
void *home_xyz(void *ptr);
void *home_diagact(void *ptr);
void *read_stmode(void *ptr);

int home_xy_start(void);
int home_x_start(void);
int home_y_start(void);
int read_stmode_start(void);
void home_msgfbk_wakeup(void);
void diagact_msgfbk_wakeup(void);

int Read_RTC_Home_Status(ONS_BYTE *Msg_Buff);
int Read_RTC_DiagAct_Status(ONS_BYTE *Msg_Buff);

void Home_SetVirtualRTC(int var);

int Home_PthreadInit(void);
void home_xy_parameter_init(void);
void diagact_single_home_parameter_init(void);

#endif //_HOMEMODE_H_