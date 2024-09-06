// ---------------------------------------------------------------------------
//  Filename:  Tension_Def.h
//  Created by: Shmulik Hass
//  Date:  28.12.15
//  Orisol (c) 
// ---------------------------------------------------------------------------

#ifndef _TENSION_DEF_H_
#define _TENSION_DEF_H_


#define TT_DEFAULT_VAL  5

class Thread_Tension
{
public:
	struct
	{
		int		TT_Default;
	} Profile ;

	Thread_Tension()
	{
		Profile.TT_Default = TT_DEFAULT_VAL;
	}

	static Thread_Tension *GetInstance();
	
	int Get_TT_Default() { return (Profile.TT_Default); }
	void Set_TT_Default(int TT_Default) { Profile.TT_Default = TT_Default ; }

} ;

// extern class Thread_Tension	Thread_Tension;

#endif