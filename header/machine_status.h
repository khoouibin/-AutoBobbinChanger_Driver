#ifndef _MACHINE_STATUS_H_
#define _MACHINE_STATUS_H_

// 這個部分還沒定案.

typedef enum
{
	Ready				= 0,
    NotReady      		= 1,
	FeedHold   			= 2,
	CycleStart			= 3
}
Machine_Status;

int GetMachineStatus();
int SetMachineStatus(int iStatus);

#endif //_MACHINE_STATUS_H_