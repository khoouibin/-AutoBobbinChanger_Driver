// ---------------------------------------------------------------------------
//  Filename: time_func.h
//  Created by: Jack.ccy
//  Date:  22/10/2020
//  Orisol (c) 
// ---------------------------------------------------------------------------

#ifndef _TIMER_FUNC_H_
#define	_TIMER_FUNC_H_

#include<sys/time.h>

struct timeval GetTimeStamp(void);

long DiffTimeMs(struct timeval tOld, struct timeval tNew);

long DiffTimeUs(struct timeval tOld, struct timeval tNew);

#endif	// _TIMER_FUNC_H_ 

