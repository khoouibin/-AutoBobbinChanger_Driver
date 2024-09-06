#ifndef _RTC_DRIVER_APITEST_H_
#define _RTC_DRIVER_APITEST_H_
#include <iostream>
#include <string>
#include "Msg_Prot.h"
#include "json.hpp"
using json = nlohmann::json;


typedef struct
{
	bool          trig;
	Poco::Event   g_tensionfbk;
}TensionCtrl_t;

typedef struct
{
	bool          trig;
	int           seq;
	Poco::Event   g_apffbk;
}APFCtrl_t;

typedef struct
{
	bool          trig;
	int           seq;
	Poco::Event   g_shfbk;
}SewHeadCtrl_t;

typedef struct
{
	bool          trig;
	Poco::Event   g_SHrofbk;			 //sew head rorating wait to stop.
	std::vector<int>MachineInfo_Zservo_Pos;        //id76
}SewHeadPos_t;

typedef struct
{
	bool          trig;
	int           seq;
	Poco::Event   g_xyfbk;
}XYCtrl_t;

typedef struct
{
	bool          trig;
	int           seq;
	Poco::Event   g_xymovfbk;
	std::vector<int>RTCStatusXYMoving;        //id11
	std::vector<int>RTCStatusHWState_XYjump;  //id11
	std::vector<int>MacStatusSerInd_XYMOVING; //id51.
}XYMove_t;


typedef struct
{
	bool          trig;
	Poco::Event   g_SHrofbk;			 //sew head rorating wait to stop.
	std::vector<int>MachineInfo_Zservo_Pos;        //id76
}TrimAct_t;


//-----------------------------------------------------------------------------
// function.
	int  Tension_Setting(int val);
	void Tension_wakeup(void);

	int  APF_Setting( int val );
	void APF_wakeup(void);

	int  SewHead_Lock(SH_UP_Pos_t pos);
	int  SewHead_PostionCmd( int rpm, int pos_cmd );
	void SewHead_PostionCmd_sh_wakeup(void);

	int SewHead_Unlock(void);
	int SewHead_LockCurrentPosition(void);
	int SewHead_LockUpPosition(void);
	void SewHead_wakeup(void);

	int cut(void);
	void cut_sh_wakeup(void);

	void XYTable_wakeup(void);

	int XYTable_Lock( void );
	int XYTable_Unlock(void);
	int XYTable_UpdatePosition( void );
	int XYTable_Lock_Update( void );

	int XY_MoveFullSpd( int xpos, int ypos );
	void XYMoveFullSpd_wakeup(void);

	int XY_MoveRatioSpd( int xpos, int ypos, int spd_ratio );
	int XY_MoveSync( int xpos, int ypos );

	void XSTEP_toggle(void);
	void YSTEP_toggle(void);

	int mov_APF_test( int loop );

	int XY_Move_APF( int xpos, int ypos, int final_tension, int final_apfpos );

	void XY_Boundary( int x, int y, int* x_res, int* y_res );
#endif //_GLOBAL_H_
