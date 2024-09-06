// ---------------------------------------------------------------------------
//  Filename:  RPM.h
//  Created by: Shmulik Hass
//  Date:  19.07.15
//  Orisol (c) 
// ---------------------------------------------------------------------------




#ifndef _RPM_H_
#define _RPM_H_


#define MAX_PRECUT_STITCHES  4


typedef enum   { LONG_AXIS_MODE = 0 , STITCH_SIZE_MODE = 1 } RPM_Calc_Mode_t ;

typedef struct
{
	int		Min_Speed;		// minimum speed to use, except in stop stitches
	int		First_Stitch_Speed ;	// initial speed in first stitch when starting */
	int     Second_Stitch_Speed;
	int     Third_Stitch_Speed;
	int		Fourth_Stitch_Speed;
	int     Number_of_Slow_Stitches;
	int     Machine_RPM_Limit ;     // Max RPM for this machine
	int		Pre_Cut_Stitches ; // number of stitches before cut: rotary=1, shuttle=2
	int		Pre_Cut_RPM[MAX_PRECUT_STITCHES];  // The stitches before cut. the first is the last 
	int		Head_Acc ;	// to be multiplied by 2 * PI 
	int		Head_Deacc ; // to be multiplied by 2 * PI 
	int			XY_Speed ;
	RPM_Calc_Mode_t  RPM_Calc_Mode;
	int		Move_Section ; 
	int		Servo_Delay ;		// in miliseconds
	int     Min_Start_Angle ;
	int		Max_Start_Angle ;
    int     XY_Accelaration ;
	int     SewHead_Angle_Scale ;	 // one round of encoder
	int		Move_Section_Delta_per_Speed_Index ;
	int		Start_Angle_Delta_per_Speed_Index ;
	//int		XY_Steps_Per_mm ;	// ***>> XY_World
	int		Max_DT_Value ;
	//int		Global_X_Offset ;	// ***>> XY_World
	//int		Global_Y_Offset ;	// ***>> XY_World
		

}
RPM_Profile_t;


class  RPM_Calc_t
{
public:
	RPM_Profile_t   RPM_Profile;
	int				Current_Speed_Index ;
	double			Head_Acc;
	double			Head_Deacc;

	RPM_Calc_t()
	{
		// load default to profile 
		RPM_Profile.Min_Speed = 200;
		RPM_Profile.First_Stitch_Speed = 200;
		RPM_Profile.Second_Stitch_Speed = 200;
		RPM_Profile.Third_Stitch_Speed = 400;
		RPM_Profile.Fourth_Stitch_Speed = 600;
		RPM_Profile.Number_of_Slow_Stitches = 4;

		RPM_Profile.Machine_RPM_Limit = 2500;

		RPM_Profile.Pre_Cut_Stitches = 1 ;  
		RPM_Profile.Pre_Cut_RPM[0] = 400 ;
		RPM_Profile.Pre_Cut_RPM[1] = 0;
		RPM_Profile.Pre_Cut_RPM[2] = 0;		
		RPM_Profile.Pre_Cut_RPM[3] = 0;		

		RPM_Profile.Head_Acc = 150 ;
		RPM_Profile.Head_Deacc = 180 ;
		RPM_Profile.XY_Speed = 200 ;
		RPM_Profile.RPM_Calc_Mode = LONG_AXIS_MODE;
		RPM_Profile.Move_Section = 200 ;
		RPM_Profile.Servo_Delay = 5;

		RPM_Profile.Min_Start_Angle = 180 ;
		RPM_Profile.Max_Start_Angle = 359 ;
		RPM_Profile.XY_Accelaration = (int)(2.5 * 9800 +0.5) ; // 2.5g = 24500
		RPM_Profile.SewHead_Angle_Scale = 2000 ;  // 2000 pulses per round
		RPM_Profile.Move_Section_Delta_per_Speed_Index = 3 ;
		RPM_Profile.Start_Angle_Delta_per_Speed_Index = 3 ;
		//RPM_Profile.XY_Steps_Per_mm = 20 ;	// ***>> XY_World
		RPM_Profile.Max_DT_Value = 4000 ; // 4 msec

		//RPM_Profile.Global_X_Offset = 0 ;	// ***>> XY_World
		//RPM_Profile.Global_Y_Offset = 0 ;	// ***>> XY_World
		

		Current_Speed_Index = 24 ;

		Head_Acc = RPM_Profile.Head_Acc * M_PI * 2;
		Head_Deacc = RPM_Profile.Head_Deacc * M_PI * 2;
							
	}


	void Set_RPM_Profile(RPM_Profile_t  Profile) 
	{ 
		RPM_Profile = Profile; 
		Head_Acc = RPM_Profile.Head_Acc * M_PI * 2;
		Head_Deacc = RPM_Profile.Head_Deacc * M_PI * 2;
	}
	void Get_RPM_Profile(RPM_Profile_t & Profile) { Profile = RPM_Profile ; }

	void Set_RPM_Speed_Index(int Speed_Index) { Current_Speed_Index = Speed_Index; }
	int Get_RPM_Speed_Index(void) { return ( Current_Speed_Index);}


	int Get_Min_Speed() { return RPM_Profile.Min_Speed; }
	int Get_First_Stitch_Speed() { return RPM_Profile.First_Stitch_Speed; }
	int Get_Second_Stitch_Speed() { return RPM_Profile.Second_Stitch_Speed; }
	int Get_Third_Stitch_Speed() { return RPM_Profile.Third_Stitch_Speed; }
	int Get_Fourth_Stitch_Speed() { return RPM_Profile.Fourth_Stitch_Speed; }
	int Get_Number_Of_Slow_Stitches() { return RPM_Profile.Number_of_Slow_Stitches; }

	int Get_Pre_Cut_Stithces() { return RPM_Profile.Pre_Cut_Stitches; }
	int Get_Pre_Cut_RPM (int N_Stitch) { if (N_Stitch < 0 || N_Stitch >= RPM_Profile.Pre_Cut_Stitches)return (0);  return RPM_Profile.Pre_Cut_RPM[N_Stitch]; }

	int Get_Machine_RPM_Limit() { return RPM_Profile.Machine_RPM_Limit; }
	
	double Get_Head_Acc() { return (Head_Acc); }
	double Get_Head_Deacc() { return (Head_Deacc); }
	RPM_Calc_Mode_t Get_RPM_Calc_Mode()  { return (RPM_Profile.RPM_Calc_Mode); }

	int Calc_Max_Increase_RPM(int Current_RPM, int & Next_RPM);
	int Calc_Max_Increase_RPM_Rev(int Current_RPM, int & Next_RPM);
	int Calc_Max_Decrease_RPM(int Current_RPM, int & Next_RPM);
	int Max_RPM_Per_XY_Move(int dx, int dy);	
	int Calc_RPM_Params(int RPM, int dx, int dy, int & Start_Angle, int & Dt_Vx, int & Dt_Vy);
	int Calc_XY_Dt(int dx, int dy, int & Dt_Vx, int & Dt_Vy);
	double  Calc_Stitches_Time ( int RPM , int Dt );
	
	//int X_Convert_to_MU(double X_Cord);		// ***>> XY_World
	//int Y_Convert_to_MU(double Y_Cord);		// ***>> XY_World

	//double MU_Convert_to_X ( int X_Position );	// ***>> XY_World
	//double MU_Convert_to_Y  (int Y_Position);	// ***>> XY_World

	//double MU_Convert_to_MM(int XY_Position);	// ***>> XY_World

}  ;

extern class RPM_Calc_t   RPM_Calc ;				// temp - global one // TBD - to change



#endif


