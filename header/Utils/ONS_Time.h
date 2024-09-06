// ---------------------------------------------------------------------------
//  FileName: ONS_Time.h
//  Created by: Nissim Avichail
//  Date:  06/04/2016
//  Orisol (c) 
// ---------------------------------------------------------------------------


#ifndef _ONS_TIME_H_
#define _ONS_TIME_H_

#define ONS_TIME_VERSION   "ONS_Time 0.1.0"



#define ONS_TIME_OUT		777

class ONS_Time_Moudle
{
	Time_Unit_t	ONS_Base_Time_Sec;
	Time_Unit_t	ONS_Base_Time_mSec;
	int			Max_Time_For_Operation;

	public:
	ONS_Time_Moudle()
	{
		Max_Time_For_Operation = 30000;
	}
	void  Set_Base_Time(void);
	void  Get_ONS_Current_System_Time_In_Millisec(Time_Unit_t & Current_Time);     //(Time_Unit_t) (current sec-base sec)*1000 + (current usec-base usec)/1000
	char * Get_Version(void)
	{
		static char Version[] = ONS_TIME_VERSION ;
		return Version;
	}
	int  Check_Time_Out_Condition(Time_Unit_t Start_Time, Time_Unit_t Time_Out);
	int	 Get_Max_Time_For_Operation(void) { return Max_Time_For_Operation; };
	void Set_Max_Time_For_Operation(int Time) { Max_Time_For_Operation = Time; };

};

extern class ONS_Time_Moudle ONS_Time ;

#endif
