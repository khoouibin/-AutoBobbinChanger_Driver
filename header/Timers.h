// ---------------------------------------------------------------------------
//  Filename: Timers.h
//  Created by: Roey Huberman
//  Date:  07/07/15
//  Orisol (c) 
// ---------------------------------------------------------------------------

#ifndef _TIMERS_H_
#define	_TIMERS_H_

#define Fcy 60000000L
#define Delay_1uS_Cnt (Fcy /1000000)
#define Delay_1mS_Cnt (Fcy/1000)
#define TCY_CNT_PER_LOOP    15 //insted of 6 fix by adam
#define Timer9TogglesPerSec 1000
#define Timer9Tick ( ( Fcy / 8 ) / Timer9TogglesPerSec )

typedef enum
{
	USBTimer_Interrupt_Fast_Speed			=	0,
	USBTimer_Interrupt_Slow_Speed           =   1,
    USBTimer_Interrupt_Delayed_Speed        =   2
}
	USBTimer_Interrupt_Mode_State_t;
    
typedef enum
{
	USBTimer_Rx					=	0,
    USBTimer_Tx        			=	1
}
	USBTimer_Direction_t;
 
    
void Init_Timers( void );
void USBTimer_SetInterruptTimeMode ( USBTimer_Interrupt_Mode_State_t Mode , USBTimer_Direction_t Direction ); 
unsigned long SysTimer_GetTimeInMiliSeconds ( void );
void SysTimer_SetTimerInMiliSeconds ( int Timer_Handle , unsigned long Delta_Time );
int SysTimer_IsTimerExpiered ( int Timer_Handle , unsigned long Delta_Time );
void Delay1usX ( unsigned int count );
void ClearIntrflags( void );

#endif	// _TIMERS_H_ 

