#ifndef _RTC_IOPORTS_H_
#define _RTC_IOPORTS_H_

// 這份文件裡的IO_Port_Id_t應該與RTC的RTC_IOports.h一致
typedef enum
{
	IOP_UNDEFINED                   = -1 , 	// when no IOP defined
	
	IOP_YA_PISTON_ID                = 0,
	IOP_YA1_PISTON_ID               = 0,	//Orisew_Loader
	IOP_LDR_T_PISTON_DN_ID          = 1,
	IOP_YA2_PISTON_ID               = 1,	//Orisew_Loader, additional output for the 5-way 3-position valve
	IOP_CL_PISTON_ID                = 2,
	IOP_LDR_CL_PISTON_ID            = 2,	//Orisew_Loader
	IOP_XA_PISTON_ID                = 3,
	IOP_XA1_PISTON_ID               = 3,	//Orisew_Loader
	IOP_LDR_X_PISTON_RIGHT_ID       = 4,
	IOP_XA2_PISTON_ID               = 4,	//Orisew_Loader, additional output for the 5-way 3-position valve
	IOP_ZP_PISTON_ID                = 5,
	IOP_LDR_ZP_PISTON_ID            = 5,	//Orisew_Loader
	IOP_RIVET_MAGNET_ID             = 6,
    IOP_LOADER_SAFETY_DISABLE_ID    = 6,    //58 safety - changed for Israel testing = 6,	//IOP_ENABLE_LIGHT_CURTAIN_ID = 6,	//Orisew_Safeties
	IOP_RIVET_STOP_ID               = 7,
	IOP_LOADER_RED_LAMP_ID          = 7,	//Orisew_Loader
	IOP_XP_PISTON_ID                = 8,
    IOP_LDR_XP_PISTON_ID            = 8,        
	IOP_ZA_PISTON_ID                = 9,
	IOP_ZA1_PISTON_ID               = 9,
	IOP_THREAD_CONSUMPTION_ID       = 10,	//Spool_Counters
	IOP_LOCATING_SENSOR_ID          = 10,
	IOP_PALLET_COVER_OPEN_SENSOR_ID = 11,	//Orisew_Safeties
	IOP_BC_YO_SENSOR_ID             = 12,	//Bobbin_Changer
	

    IOP_STOP_SEW_BUTTON_ID          = 76,
    IOP_CUTTER_OUT_SENSOR_ID        = 12,
    IOP_SEW_PEDAL_ID                = 82,
    IOP_RIVET_ALARM_ID              = 35,
    IOP_LEFT_LOCK_PEDAL_ID          = 84,
    IOP_RIVET_IN_POSITION_ID        = 36,
    IOP_RIGHT_LOCK_PEDAL_ID         = 83,
    IOP_RIVET_READY_ID              = 71,
    IOP_R_HOME_SENSOR_ID            = 66,
    IOP_PF_HOME_SENSOR_ID           = 85,
    IOP_OLD_PF_HOME_SENSOR_ID       = 85,
    IOP_CLAMP_PF_DIRECTION_ID       = 57,
    IOP_CLAMP_PF_STEP_ID            = 58,
    IOP_CLAMP_PF_MOTOR_ON_ID        = 88,
    IOP_R_SERVO_ON_ID               = 59,
    IOP_R_STEP_ID                   = 60,
    IOP_R_DIRECTION_ID              = 61,
    IOP_THREAD_BREAK_INPUT_ID       = 96,
    IOP_BC_BD_SENSOR_ID             = 97,              
	IOP_PF_UP_SENSOR_ID             = 13,
	IOP_BOBBIN_COUNTER_ID           = 13,	//Spool_Counters
	IOP_WINDER_COUNTER_ID           = 14,	//Spool_Counters
	IOP_PUNCHER_DOWN_SENSOR_ID      = 15,	//Puncher
    IOP_COVER_SAFETY_KEY_ID         = 15,   // 14 safety - changed for Israel testing = 15,   //Orisew_Safeties
    IOP_UT_WELDER_DOWN_SENSOR_ID    = 15,   //Welder
    IOP_UT_WELDER_UP_SENSOR_ID      = 16,   //Welder
	IOP_SERVOS_EDM_ID               = 16,   // 63 safety - changed for Israel testing = 16,	//Orisew_Safeties
	IOP_PUNCHER_UP_SENSOR_ID        = 16,	//Puncher
    IOP_TABLE_DOWN_SENSOR_ID        = 16,   //TABLE_DOWN_SENSOR
	IOP_Z_INDEX_ID                  = 17,	//Z_Servo
	IOP_Z_CH_B_ID                   = 18,	//Z_Servo
	IOP_Z_CH_A_ID                   = 19,	//Z_Servo
	IOP_TENSION_CURRENT_POLARITY_ID = 20,	//Tension
	IOP_Z_SERVO_ON_ID               = 21,	//Z_Servo
	IOP_Z_DIRECTION_ID              = 22,	//Z_Servo
	IOP_Z_STEP_ID                   = 23,	//Z_Servo
	IOP_X_SERVO_ON_ID               = 24,	//Z_Servo
	IOP_Z_ALARM_ID                  = 25,	//Z_Servo
	IOP_X_STEP_ID                   = 26,	//X_Servo
	IOP_X_DIRECTION_ID              = 27,	//X_Servo
	IOP_X_ALARM_ID                  = 28,	//X_Servo
	IOP_LDR_ZU_SENSOR_ID            = 29,	//Orisew_Loader
	IOP_LDR_ZD_SENSOR_ID            = 30,	//Orisew_Loader
	IOP_LDR_XN_SENSOR_ID            = 31,	//Orisew_Loader
	IOP_LDR_XF_SENSOR_ID            = 32,	//Orisew_Loader
	IOP_PALLET_SENSOR_ID            = 33,	//Orisew_Loader
    	IOP_AUTO_SEW_SENSOR_ID          = 33,
	IOP_LOADER_SAFETY_KEY_ID        = 34,   // 29 safety - changed for Israel testing = 34,	//IOP_TECHNICIAN_KEY_INPUT_ID = 34,	//Orisew_Safeties
	IOP_BC_XC_SENSOR_ID             = 35,	//Bobbin_Changer
	IOP_BC_XH_SENSOR_ID             = 36,	//Bobbin_Changer
    	IOP_READY_LAMP_ID               = 37,	//Orisew_Loader
	IOP_SAFETY_LAMP_ID              = 38,
	IOP_ZA2_PISTON_ID               = 38,	//Orisew_Loader, additional output for the 5-way 3-position valve
	IOP_MOTOR_SSR_ON_ID             = 39,
	IOP_CLAMP_LEFT_LOCK_ID          = 40,
	IOP_COVER_SAFETY_DISABLE_ID     = 40,   // 38 safety - changed for Israel testing = 40, 	//IOP_ENABLE_STO = 40,		//Orisew_Safeties
	IOP_CLAMP_RIGHT_LOCK_ID         = 41,
	IOP_R_CLAMP_LOCK_ID             = 42,
	IOP_RESET_LOADER_ALARM_ID       = 42,   // 2 safety - changed for Israel testing  = 42, 	//Orisew_Safeties
	IOP_FLIP_FLOP_ID                = 43,
	IOP_RESET_SAFETY_RELAY_ID       = 43,   // 0 safety - changed for Israel testing = 43,	//IOP_RELEASE_SAFE_RELAY_ID	= 43,	//Orisew_Safeties
	IOP_BC_CX_CYLINDER_ID           = 44,	//Bobbin_Changer, the old name is IOP_BC_ARM_TURN_CYLINDER_ID
	//IOP_PRESSER_FOOT_ID           = 44,
	IOP_PF_LEVEL_HIGH_ID            = 45,
	IOP_BC_CY_CYLINDER_ID           = 45, 	//Bobbin_Changer
	IOP_PINCHER_ID                  = 46,
	IOP_RESET_EDM_ALARM_ID          = 46,   // IOP_RESET_EDM_ALARM_ID = 46,	//Orisew_Safeties
	IOP_PULLER_ID                   = 47,
	IOP_BC_JAW_CYLINDER_ID          = 47,	//Bobbin_Changer
	IOP_WIPER_ID                    = 48,
	IOP_CUTTER_ID                   = 49,
	IOP_NEEDLE_COOLER_ID            = 50,
	IOP_BC_CA_CYLINDER_ID           = 51,	//Bobbin_Changer, The old name is IOP_BC_CASSETE_CYLINDER_ID.
	IOP_PUNCHER_PISTON_UP_ID        = 52,
	IOP_LOADER_GREEN_LAMP_ID        = 52,   // 53 safety - changed for Israel testing = 52,	//Orisew_Loader
    	IOP_UT_WELDER_DEVICE_ON_ID      = 52,   //Welder
	IOP_RIVET_PISTON_DN_ID          = 53,	//Rivet
	IOP_Y_SERVO_ON_ID               = 54,	//Y_Servo
	IOP_Y_STEP_ID                   = 55,	//Y_Servo
	IOP_Y_DIRECTION_ID              = 56,	//Y_Servo
	//IOP_CLAMP_PF_DIRECTION_ID 	= 57,	// 160301 remarked
	//IOP_CLAMP_PF_STEP_ID          = 58,	// 160301 remarked	
	//IOP_R_SERVO_ON_ID             = 59,	// 160301 remarked
	//IOP_R_STEP_ID                 = 60,	// 160301 remarked
	//IOP_R_DIRECTION_ID            = 61,	// 160301 remarked
	IOP_PUNCHER_PISTON_DN_ID        = 62,	//Puncher	////should be removed after correct the  Activator_Def[] in Activator.c
	IOP_PUNCHER_PISTON_DOWN_ID      = 62,	//Puncher
	IOP_OPEN_COVER_PISTON_ID        = 62,	// 1 safety - changed for Israel testing = 62,	//Orisew_Safeties
    	IOP_UT_WELDER_PISTON_ID         = 62,   //Welder    
	IOP_R_ALARM_ID                  = 63,
	IOP_BC_CF_SENSOR_ID             = 63,	//Bobbin_Changer
	IOP_Y_ALARM_ID                  = 64,	//Y_Servo
	IOP_BOOST_BUTTON_ID             = 65,	//Boot Loader button
	IOP_BC_CB_SENSOR_ID             = 66,	//Bobbin_Changer
	//IOP_R_HOME_SENSOR_ID          = 66,	// 160301 remarked change to IOP_PF_HOME_SENSOR_ID for the Israel test
	IOP_Y_HOME_SENSOR_ID            = 67,	//Y_Servo
	IOP_X_HOME_SENSOR_ID            = 68,	//X_Servo
	IOP_AIR_FAN_SENSOR_ID           = 69,
	IOP_AIR_SENSOR_ID               = 70,
	IOP_BC_YI_SENSOR_ID             = 71,	//Bobbin_Changer
	IOP_YN_SENSOR_ID                = 72,	
	IOP_LDR_YN_SENSOR_ID            = 72,	//Orisew_Loader
	IOP_YF_SENSOR_ID                = 73,
	IOP_LDR_YF_SENSOR_ID            = 73,	//Orisew_Loader
	IOP_XL_SENSOR_ID                = 74,
	IOP_LDR_XL_SENSOR_ID            = 74,	//Orisew_Loader
	IOP_ZL_SENSOR_ID                = 75,
	IOP_LDR_ZL_SENSOR_ID            = 75,	//Orisew_Loader
	//IOP_STOP_SEW_BUTTON_ID        = 76,
	IOP_R_INDEX_ID                  = 77,
	IOP_Y_INDEX_ID                  = 78,	//Y_Servo
	IOP_LOADER_SAFETY_ALARM_ID      = 79,   // 74 safety - changed for Israel testing = 79, 	//Orisew_Safeties
	IOP_SAFETY_BUTTON_ID            = 79,
	IOP_X_INDEX_ID                  = 80,	//X_Servo
	IOP_READY_BUTTON_ID             = 81,	//Orisew
	//IOP_SEW_PEDAL_ID              = 82,
	//IOP_RIGHT_LOCK_PEDAL_ID       = 83,
	//IOP_LEFT_LOCK_PEDAL_ID        = 84
	//IOP_PF_HOME_SENSOR_ID         = 85
	IOP_TENSION_PWM_ID              = 86,	//Tension
	IOP_AIR_VALVE_ON_ID             = 87,
	//IOP_CLAMP_PF_MOTOR_ON_ID      = 88,
    IOP_LEVEL_SHIFT_IC_ID           = 89,
	IOP_RESET_THREAD_BREAK_ID       = 90,	//Thread_Break_Detect
    IOP_LOWER_COVER_CLOSED_ID       = 91, 	// 64 safety - changed for Israel testing = 91,  //Orisew_Safeties
    IOP_UPPER_COVER_CLOSED_ID       = 92, 	// 33 safety - changed for Israel testing = 92,  //Orisew_Safeties    
    IOP_UPPER_COVER_OPENED_ID       = 93, 	// 28 safety - changed for Israel testing = 93,  //Orisew_Safeties    
    IOP_COVER_OPEN_ALARM_ID         = 94, 	// 31 safety - changed for Israel testing = 94,  //Orisew_Safeties
	IOP_RESET_RFID_ID               = 95	//RFID
}
    IO_Port_Id_t;

#endif