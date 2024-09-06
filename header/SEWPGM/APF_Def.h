// ---------------------------------------------------------------------------
//  Filename:  APF_Def.h
//  Created by: Shmulik Hass
//  Date:  13.06.17
//  Orisol (c)
// ---------------------------------------------------------------------------

#ifndef _APF_DEF_H_
#define _APF_DEF_H_

#define PF_DEFAULT_VAL 12
#define PF_DEFAULT_STEP_RATIO (230. / 180.) // 18.0 mm per 230 steps
#define PF_DEFAULT_SPEED 1000				// time in usec per step

#define SCLAMP_MIN_HEIGHT 0.5
#define SCLAMP_MAX_HEIGHT 20.00
#define SCLAMP_MAX_LEVELS 200

class PF
{
public:
	struct
	{
		int PF_Default;
		double Step_Ratio; // conversion from mm to steps
		int PF_Speed;
		double SClamp_Min_Height;
		double SClamp_Max_Height;
		int APF_Height_To_Disable_Wiper;

	} Profile;

	PF()
	{
		Profile.PF_Default = PF_DEFAULT_VAL;
		Profile.Step_Ratio = PF_DEFAULT_STEP_RATIO;
		Profile.PF_Speed = PF_DEFAULT_SPEED;
		Profile.SClamp_Min_Height = SCLAMP_MIN_HEIGHT;
		Profile.SClamp_Max_Height = SCLAMP_MAX_HEIGHT;
		Profile.APF_Height_To_Disable_Wiper = 0; // disable by default
	}

	static PF *GetInstance();

	void Set_APF_Height_To_Disable_Wiper(int APF_Height_To_Disable_Wiper)
	{
		Profile.APF_Height_To_Disable_Wiper = APF_Height_To_Disable_Wiper;
	}
	int Get_APF_Height_To_Disable_Wiper()
	{
		return (Profile.APF_Height_To_Disable_Wiper);
	}
	void Set_PF_Default(int PF_Default)
	{
		Profile.PF_Default = PF_Default;
	}
	int Get_PF_Default() { return (Profile.PF_Default); }

	void Set_PF_Step_Ratio(double Step_Ratio)
	{
		Profile.Step_Ratio = Step_Ratio;
	}
	void Set_PF_Speed(int PF_Speed)
	{
		Profile.PF_Speed = PF_Speed;
	}
	void Set_SClamp_Heights(double SClamp_Min_Height, double SClamp_Max_Height)
	{
		Profile.SClamp_Min_Height = SClamp_Min_Height;
		Profile.SClamp_Max_Height = SClamp_Max_Height;
	}

	int Get_PF_Move_Time(int PF_Steps)
	{
		return (Profile.PF_Speed * PF_Steps);
	}
	int PF_Convert_Height_to_Steps(int PF_Height)
	{
		return ((int)(PF_Height * Profile.Step_Ratio + 0.5));
	}
	int PF_Convert_Steps_to_Height(int PF_Steps)
	{
		return ((int)(PF_Steps / Profile.Step_Ratio + 0.5));
		//return ((int)((double)PF_Steps / Profile.Step_Ratio + 0.5) );
	}
	int SClamp_Convert_Height_to_Level(double SClamp_Height)
	{
		int temp;

		temp = (int)((SClamp_Height - Profile.SClamp_Min_Height) / ((Profile.SClamp_Max_Height - Profile.SClamp_Min_Height) / SCLAMP_MAX_LEVELS));
		temp = SCLAMP_MAX_LEVELS - temp; // different direction
		if (temp < 0)
			temp = 0;
		else if (temp >= SCLAMP_MAX_LEVELS)
			temp = SCLAMP_MAX_LEVELS;
		return (temp);
	}
	double SClamp_Convert_Level_to_Height(int SClamp_Level)
	{
		double temp;

		temp = (SCLAMP_MAX_LEVELS - SClamp_Level) * ((Profile.SClamp_Max_Height - Profile.SClamp_Min_Height) / SCLAMP_MAX_LEVELS) + Profile.SClamp_Min_Height;
		if (temp < Profile.SClamp_Min_Height)
			temp = Profile.SClamp_Min_Height;
		else if (temp >= Profile.SClamp_Max_Height)
			temp = Profile.SClamp_Max_Height;
		return (temp);
	}
};

// extern class PF PF;

#endif