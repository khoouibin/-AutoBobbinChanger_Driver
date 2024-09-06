#ifndef _RTC_DRIVER_APITEST_H_
#define _RTC_DRIVER_APITEST_H_
#include <iostream>
#include <string>
#include "Msg_Prot.h"
#include "json.hpp"
using json = nlohmann::json;

typedef struct
{ // id - 12
	bool req;
	int mode;
	unsigned int delay;
	int dir;
	int apf_mov;
}run_data;


//-----------------------------------------------------------------------------
// function.
	//-----------------------------------------------------------------------------
	//--stitch control
	int stitch(int _mode);
	int run_ctrl(int _mode, int _delay);
	int dryrun_onestep(void);
	int dryrun_conti(void);
	int stitch_conti(void);

	int tension_set(int _value);
	int apfpos_set(bool _in, int _value);
	int apf_up(void);
	int apf_dn(void);
	int apf_updown(int _wait, int _act);

	int cut(void);

	int lock_sh_set(int _value);
	int lock_sh_set3020(int _state, int _value);

	//-----------------------------------------------------------------------------
	int drivXY_predictpos(void); // test... unuse.
	int XY_Value_predict(void);	 // test... unuse.

	void XSTEP_toggle(void);
	void YSTEP_toggle(void);

#endif //_GLOBAL_H_
