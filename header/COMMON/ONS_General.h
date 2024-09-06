// ---------------------------------------------------------------------------
//  Filename: ONS_General.h
//  Created by: Nissim Avichail
//  Date:  08/07/15
//  Orisol (c)
// ---------------------------------------------------------------------------


#ifndef _ONS_GEN_
#define _ONS_GEN_



#define ONS_TRUE   1		 
#define ONS_FALSE  0		 

#ifdef ONS_WIN
#define __FILENAME__   __FILE__//(strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#endif
#ifdef ONS_LINUX
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif
typedef   unsigned short UINT_16;
typedef   int16_t  INT_16;
typedef   unsigned int UINT_32;
typedef   int32_t  INT_32;



typedef unsigned char ONS_BYTE;    
typedef unsigned long Time_Unit_t;	//unsigned long


#endif
