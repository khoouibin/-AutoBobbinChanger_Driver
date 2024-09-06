#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdint.h>
#include <iostream>
#include <string>
#include <vector>
#include <fcntl.h> // serial needs.
#include <termios.h>
#include <errno.h>

#include "Poco/Task.h"
#include "Poco/TaskManager.h"
#include "Poco/TaskNotification.h"
#include "Poco/NotificationCenter.h"
#include "Poco/NotificationQueue.h"
#include "Poco/Exception.h"
#include "Poco/Mutex.h"
#include "Poco/Condition.h"
#include "Poco/Event.h"
#include "Poco/Timestamp.h"

#include "interface_to_host.h"
#include "global.h"
#include "IO_Entity.h"

#include "SEWPGM.h"
#include "RPM.h"
#include "APF_Def.h"

using namespace std;
using json = nlohmann::json;


enum UHF6040EntityTable
{
// safety sensor
    IO_UHF6040_RIGHT_SAFETY_DOOR_SENSOR_ENTITY = IO_ZU_SENSOR_ENTITY,
    IO_UHF6040_RIGHT_PALLET_SENSOR_ENTITY = IO_PALLET_SENSOR_ENTITY,
    IO_UHF6040_MIDDLE_PALLET_SENSOR_ENTITY = IO_PALLET_COVER_OPEN_SENSOR_ENTITY,
    IO_UHF6040_LEFT_PALLET_SENSOR_ENTITY = IO_LOCATING_SENSOR_ENTITY,
    IO_UHF6040_LEFT_SAFETY_DOOR_SENSOR_ENTITY = IO_STOP_SEW_BUTTON_ENTITY,

    // stopper up sensor
    IO_UHF6040_STOPPER_UP_SENSOR_ENTITY = IO_CUTTER_OUT_SENSOR_ENTITY,
    // stopper control
    IO_UHF6040_STOPPER_UP_CTL_ENTITY = IO_PUNCHER_PISTON_UP_ENTITY,
    IO_UHF6040_STOPPER_DN_CTL_ENTITY = IO_PUNCHER_PISTON_DN_ENTITY,

    // load-piston left/right sensor
    IO_UHF6040_LOAD_PISTON_RIGHT_SENSOR_ENTITY = IO_PF_HOME_SENSOR_ENTITY,
    IO_UHF6040_LOAD_PISTON_LEFT_SENSOR_ENTITY = IO_PF_UP_SENSOR_ENTITY,
    // load-piston left/right control
    IO_UHF6040_LOAD_PISTON_RIGHT_CTL_ENTITY = IO_YA2_PISTON_ENTITY,
    IO_UHF6040_LOAD_PISTON_LEFT_CTL_ENTITY = IO_YA1_PISTON_ENTITY,

    // welder-piston up/down sensor
    IO_UHF6040_WELDER_PISTON_UP_SENSOR_ENTITY = IO_PUNCHER_UP_SENSOR_ENTITY,
    IO_UHF6040_WELDER_PISTON_DN_SENSOR_ENTITY = IO_PUNCHER_DN_SENSOR_ENTITY,
    // welder-piston up/down control
    IO_UHF6040_WELDER_PISTON_UP_CTL_ENTITY = IO_ZA1_PISTON_ENTITY,
    IO_UHF6040_WELDER_PISTON_DN_CTL_ENTITY = IO_ZA2_PISTON_ENTITY,
    // welder power control
    IO_UHF6040_WELDER_POWER_CTL_ENTITY = IO_RIVET_STOP_ENTITY,
    // welder power sensor
    IO_UHF6040_WELDER_POWER_SENSOR_ENTITY = IO_RIVET_READY_ENTITY,

    // xy limit sensor
    IO_UHF6040_X_LEFT_LIMIT_SENSOR_ENTITY = IO_X_HOME_SENSOR_ENTITY,
    IO_UHF6040_X_RIGHT_LIMIT_SENSOR_ENTITY = IO_R_ALARM_ENTITY,
    IO_UHF6040_Y_FRONT_LIMIT_SENSOR_ENTITY = IO_R_HOME_SENSOR_ENTITY,
    IO_UHF6040_Y_BACK_LIMIT_SENSOR_ENTITY = IO_Y_HOME_SENSOR_ENTITY,
    IO_UHF6040_X_HOME_SENSOR_ENTITY = IO_X_HOME_SENSOR_ENTITY,
    IO_UHF6040_Y_HOME_SENSOR_ENTITY = IO_Y_HOME_SENSOR_ENTITY,

    // clamp lock sensor
    IO_UHF6040_CLAMP_LOCK_SENSOR_ENTITY = IO_WINDER_COUNTER_ENTITY,
    // clamp lock/release control
    IO_UHF6040_CLAMP_LOCK_CTL_ENTITY = IO_R_CLAMP_LOCK_ENTITY,
    IO_UHF6040_CLAMP_RELEASE_CTL_ENTITY = IO_CLAMP_LEFT_LOCK_ENTITY,

    // led
    IO_UHF6040_LEFT_SIGNAL_LED_WHITE_CTL_ENTITY = IO_NEEDLE_COOLER_ENTITY,
    IO_UHF6040_RIGHT_SIGNAL_LED_WHITE_CTL_ENTITY = IO_PINCHER_ENTITY,
    IO_UHF6040_SIGNAL_LED_GREEN_CTL_ENTITY = IO_BC_CX_CYLINDER_ENTITY,
    IO_UHF6040_SIGNAL_LED_YELLOW_CTL_ENTITY = IO_PF_LEVEL_HIGH_ENTITY,
    IO_UHF6040_SIGNAL_LED_RED_CTL_ENTITY = IO_BC_CA_CYLINDER_ENTITY,

    // air
    IO_UHF6040_AIR_SENSOR_ENTITY = IO_AIR_SENSOR_ENTITY,
    IO_UHF6040_AIR_FAN_SENSOR_ENTITY = IO_AIR_FAN_SENSOR_ENTITY,

    // buzzer
    IO_UHF6040_BUZZER_CTL_ENTITY = IO_PULLER_ENTITY,

    // sto-reset
    IO_UHF6040_STO_RESET = IO_RIVET_MAGNET_ENTITY
};

class RTCSimulator : public Poco::Task
{
public:
    enum LoaderMode
    {
        LeftToRight = 0,
        RightToLeft = 1,
        LeftToLeft = 2,
        RightToRight = 3,
        LeftToRightDryRun = 10,
        RightToLeftDryRun = 11,
        LeftToLeftDryRun = 12,
        RightToRightDryRun = 13
    };

    enum LoaderStatus
    {
        Unknown = -1,
        PrepareMoveInReadyAtLeft = 0,
        PrepareMoveInReadyAtRight = 1,
        ReadyForProcessing = 2,
        PrepareTakeOutReadyAtLeft = 3,
        PrepareTakeOutReadyAtRight = 4
    };

    enum UserOperstion
    {
        PutHandIn = 0,
        PutHandOut = 1,
        GetHandIn = 2,
        GetHandOut = 3,
    };

    enum RunningMode
    {
        SewDryContinueForward = 0,
        SewDryContinueBackward = 1,
        SewDryLineForward = 2,
        SewDryLineBackward = 3,
        SewDryStepForward = 4,
        SewDryStepBackward = 5,
        SewNormalContinue = 6,
        SewProgStop = 7,
        SewProgAbort = 8,
    };

private:
    const int m_c_xy_motor_left_limit = 0;
    const int m_c_xy_motor_right_limit = 600 * 20;
    const int m_c_xy_motor_back_limit = 400 * 20;
    const int m_c_xy_motor_front_limit = 0;

    int m_i_motor_x, m_i_motor_y;
    int m_curr_io_table[IO_TABLE_BOUNDARY];
    int m_prev_io_table[IO_TABLE_BOUNDARY];
    int m_io_tracking_list[18] = {
        IO_UHF6040_RIGHT_SAFETY_DOOR_SENSOR_ENTITY,  // 0
        IO_UHF6040_RIGHT_PALLET_SENSOR_ENTITY,       // 1
        IO_UHF6040_MIDDLE_PALLET_SENSOR_ENTITY,      // 2
        IO_UHF6040_LEFT_PALLET_SENSOR_ENTITY,        // 3
        IO_UHF6040_LEFT_SAFETY_DOOR_SENSOR_ENTITY,   // 4
        IO_UHF6040_STOPPER_UP_SENSOR_ENTITY,         // 5,
        IO_UHF6040_LOAD_PISTON_RIGHT_SENSOR_ENTITY,  // 6
        IO_UHF6040_LOAD_PISTON_LEFT_SENSOR_ENTITY,   // 7
        IO_UHF6040_WELDER_PISTON_UP_SENSOR_ENTITY,   // 8
        IO_UHF6040_WELDER_PISTON_DN_SENSOR_ENTITY,   // 9
        IO_UHF6040_WELDER_POWER_CTL_ENTITY,          // 10
        IO_UHF6040_CLAMP_LOCK_SENSOR_ENTITY,         // 11
        IO_UHF6040_LEFT_SIGNAL_LED_WHITE_CTL_ENTITY,  // 12
        IO_UHF6040_RIGHT_SIGNAL_LED_WHITE_CTL_ENTITY, // 13
        IO_UHF6040_SIGNAL_LED_GREEN_CTL_ENTITY,      // 14
        IO_UHF6040_SIGNAL_LED_YELLOW_CTL_ENTITY,     // 15
        IO_UHF6040_SIGNAL_LED_RED_CTL_ENTITY,        // 16
        IO_UHF6040_BUZZER_CTL_ENTITY                 // 17
    };

    bool m_b_is_welder_power_on;
    bool m_b_running;
    bool m_b_is_ready_stop;
    bool m_b_is_homing_aborted;

    int m_i_welder_position;
    LoaderMode m_loader_mode;

    Sew_PGM *m_sew_pgm;
    Poco::Event m_event;

    int m_i_sewline_index;
    int m_i_lead_point_index;
    int m_i_stitch_point_index;
    int m_i_sewline_n;
    bool m_b_is_continue_run;
    RunningMode m_running_mode;
    SEWPGM_MODES_t m_sew_pgm_mode;
    Poco::Timestamp m_time_now;
    RPM_Profile_t m_rpm_profile;
    LoaderStatus m_loader_status;

public:
    RTCSimulator(const std::string &name);
    static RTCSimulator *GetInstance();

    bool IsNeedUpdateStitch();
    void Init();
    void SetLoaderMode(LoaderMode mode);
    bool IsTrackIoChanged();
    void SetPGS(std::string pgs_path, json rpm_profile);
    int ExecuteProg(RunningMode mode);
    void NotifyHostStitchPointChange();
    void NotifyHostSewLineChange();
    int TaskGetReady(int iMode);
    int TaskBeforeJob(int iMode);
    int TaskAfterJob(int iMode);
    int GetTaskStatus();

    void LoaderPistonOp(int id, int value);
    void StopperPistonOp(int id, int value);
    void WelderPistonOp(int id, int value);
    void WelderPowerOp(int id, int value);
    void ClampOp(int id, int value);
    void XyMotorOp();
    void MoveSpeedTest();

    void SetEntity(int id, int value);
    int GetEntity(int id);

    void LoginProcess();
    void Homing();
    void AbortHoming();

    void LockXyMotor();
    void ReleaseXyMotor();
    int ClampAct(int op);

    int MovXY(int _Xin, int _Yin, int _apfAct, int _boundaryChk);
    int XyReset();
    int XyServoSwitch(UHF_xyServoSWitch_t servo_switch);
    int WaitXyIdle(void);

    int MoveUwPiston(int op);
    void WaitUwPistonUp();
    void WaitUwPistonDn();
    int UwPower(int op);

    int GetXyStepFromModbus(int *_x_val, int *_y_val);
    int home_x_start(void);
    int home_y_start(void);
    int GetHomeSensorValue(ONS_xyHomeSensor_t _sensor);

    void runTask();
};
