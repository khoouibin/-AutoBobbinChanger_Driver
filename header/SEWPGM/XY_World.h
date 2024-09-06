// ---------------------------------------------------------------------------
//  Filename:  XY_World.h
//  Created by: Shmulik Hass
//  Date:  31/05/16
//  Orisol (c) 
// ---------------------------------------------------------------------------


#ifndef _XY_WORLD_H_
#define _XY_WORLD_H_

typedef struct
{
	int		XY_Steps_Per_mm;
	int		Global_X_Offset;
	int		Global_Y_Offset;
	int		Calib_X_Offset ;
	int		Calib_Y_Offset ;
	int		Min_X_Axis ;
	int		Max_X_Axis ;
	int		Min_Y_Axis ;
	int		Max_Y_Axis ;
	int	    Y_Axis_Reverse ;
	double	Sewing_Area_X_Size;
	double	Sewing_Area_Y_Size;
	double	Extended_Sewing_Area_X_Min;
	double	Extended_Sewing_Area_X_Max;
	double	Extended_Sewing_Area_Y_Min;
	double	Extended_Sewing_Area_Y_Max;


} XY_World_Profile_t;



class XY_World_t
{
public:	// temp
	XY_World_Profile_t	Profile ;
	int Calib_X_Offset ;
	int Calib_Y_Offset ;


	XY_World_t()
	{
		
		Profile.XY_Steps_Per_mm = 20 ;
		Profile.Global_X_Offset = 0 ;
		Profile.Global_Y_Offset = 0 ;
		Profile.Min_X_Axis = 0 ;
		Profile.Max_X_Axis = 16000;
		Profile.Min_Y_Axis = 0 ;
		Profile.Max_Y_Axis = 9000;	
		Profile.Y_Axis_Reverse = 0 ;
		Profile.Sewing_Area_X_Size = 500.;
		Profile.Sewing_Area_Y_Size = 350.;
		Profile.Extended_Sewing_Area_X_Min = -10.;
		Profile.Extended_Sewing_Area_X_Max = 510.;
		Profile.Extended_Sewing_Area_Y_Min = -10.;
		Profile.Extended_Sewing_Area_Y_Max = 360;

		Calib_X_Offset = 0;
		Calib_Y_Offset = 0;
		
	};
public:

	void Get_Profile( XY_World_Profile_t & Prof) { Prof = Profile; }
	void Set_Profile( XY_World_Profile_t & Prof) { Profile = Prof; }

	void Set_Calibration_Offset(int Offset_x, int Offset_y) { Calib_X_Offset = Offset_x; Calib_Y_Offset = Offset_y; }
	
	int X_Convert_to_MU(double X_Cord);		
	int Y_Convert_to_MU(double Y_Cord);		

	int Check_XY_Steps_In_Range ( int & XStep, int & YStep );

	double MU_Convert_to_X(int X_Position);	
	double MU_Convert_to_Y(int Y_Position);	

	double MU_Convert_to_MM(int XY_Position);	
	int	   MM_Convert_to_MU(double  XY_Cord);

	int		Check_XY_In_Sewing_Area(double X_Pos, double Y_Pos, int Ext);

};


extern class XY_World_t  XY_World ;


#endif

