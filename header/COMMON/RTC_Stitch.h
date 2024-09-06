// ---------------------------------------------------------------------------
//  Filename: RTC_Stitch.h
//  Created by: Nissim Avichail
//  Date:  06/08/15
//  Update: Shmulik 18/2/19
//  Orisol (c)
// ---------------------------------------------------------------------------

#ifndef _RTC_STITCH_H_
#define _RTC_STITCH_H_
#include "ONS_General.h"

typedef		INT_16	XYCord_t;
typedef		INT_16	RPM_t;
typedef		INT_16	Stitch_ID_t;
typedef		INT_16	PGM_SID_t;
typedef		INT_16	XY_Vel_t;
typedef		INT_16	Z_Angle_Pos_t;
typedef		INT_16	APF_H_t;
typedef		INT_16	Tension_t;
typedef    	INT_16  Stop_t;
typedef		INT_16	Stop_Type_t;
typedef		INT_16	Trim_Type_t;
typedef		INT_16	Out_Port_t;
typedef		INT_16	Out_Port_Mask_t;




typedef enum		// bitwise of values - to define STOP type.
{
	CMD_BIT_STOP_REG = 1,	// bit 0
	CMD_BIT_NDL_DWN = 2,	// bit 1
	CMD_BIT_NDL_TOP = 4	,	// bit 2
	CMD_BIT_WAIT_TIME = 8,  // bit 3
	CMD_BIT_WAIT_IO = 16 ,	// bit 4
	CMD_BIT_CONT = 32,		// bit 5
	
	CMD_BIT_IMM_STOP = 256  // Bit 7	-To be raised if only stop
		
}
Stop_Codes_t;

typedef enum
{
	CMD_BIT_TRIM_REG = 1 , // Bit 0
	CMD_BIT_TRIM_NO_WIPER = 2 // Bit 1 - to indicate no wiper
}
Trim_Codes_t ;



typedef struct
{
	Stitch_ID_t		SID;			// sequential stitch index in SL
	PGM_SID_t		PGM_SID;		// Original stitch Id
	XYCord_t		XPos;			// Position in XY carriage
	XYCord_t		YPos;			// Position in XY carriage
	RPM_t			RPM;			// Final RPM value
	XY_Vel_t		Dt_Vx;			// Time between pulses (usec)
	XY_Vel_t		Dt_Vy;			// Time between pulses (usec)
	Z_Angle_Pos_t	Start_Angle;	// Start angle for XY move
	APF_H_t			APF_H;			// APF Height in steps
	Tension_t		Tension;		// Thread Tension index
	Stop_t			Stop ;			// flag to stop in this stitch
	Stop_Type_t		Stop_Type;		// stop type: per Stop_Codes_t  
	Trim_Type_t		Trim_Type;		// trim type: per Trim_Codes_t
	Out_Port_t		Out_Port;		// bit def of out control & OP/OPX
	Out_Port_Mask_t Out_Port_Mask;  // bit mask of out controls

}
RTC_Stitch_t;



#endif // _RTC_STITCH_H_

