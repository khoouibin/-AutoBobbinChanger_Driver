// ---------------------------------------------------------------------------
//  Filename: RTC_Definition.h
//  Created by: Roey Huberman
//  Date:  07/01/16
//  Orisol (c)
// ---------------------------------------------------------------------------

#ifndef _RTC_DEFINITION_H_
#define	_RTC_DEFINITION_H_

typedef enum
{
    MEC_RTC_CONTROL                 = 0,
    MEC_RTC_STCMGR                  = 100,
    MEC_RTC_DRMGR                   = 200,
    MEC_RTC_STPMGR                  = 300,
    MEC_RTC_HOMEMGR                 = 400,
    MEC_RTC_STCBUFF                 = 500,
    MEC_RTC_ZSERVO                  = 600,
    MEC_RTC_XYSERVO                 = 700,
    MEC_RTC_XYJUMP                  = 800,
    MEC_RTC_TB                      = 900,
    MEC_RTC_TT                      = 1000,
    MEC_RTC_CUT                     = 1100,
    MEC_IO_ENTITY                   = 1200        
}
    Modules_Error_Code;

#define GET_MEC(a,b)	( a + b )   


#endif	// _RTC_DEFINITION_H_

