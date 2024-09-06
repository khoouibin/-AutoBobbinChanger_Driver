// ---------------------------------------------------------------------------
//  FileName: ONS_Trace.h
//  Created by: Shmulik Hass
//  Date:  27/12/2015
//  Orisol (c) 
// ---------------------------------------------------------------------------


#ifndef _ONS_TRACE_H_
#define _ONS_TRACE_H_

#define ONSTRC_VERSION   "ONS_Trace 0.1.6"

#ifdef _MSC_VER 

#define _CRT_SECURE_NO_WARNINGS

#endif

#include <string.h>
#include <stdio.h>
#include <stdint.h>

#define ONS_TRACE(msg) if(ONS_Trace_Is_Active())ONS_Trace(msg,(char*)"")
#define ONS_TRACE1(msg,var1) if(ONS_Trace_Is_Active())ONS_Trace(msg,(strcpy(ONS_Trace_Buf,ONS_Trace_Str(#var1,var1))))
#define ONS_TRACE2(msg,var1,var2) if(ONS_Trace_Is_Active())ONS_Trace(msg,(strcpy(ONS_Trace_Buf,ONS_Trace_Str(#var1,var1)),strcat(ONS_Trace_Buf,ONS_Trace_Str(#var2,var2))))
#define ONS_TRACE4(msg,var1,var2,var3,var4) if(ONS_Trace_Is_Active())ONS_Trace(msg,(strcpy(ONS_Trace_Buf,ONS_Trace_Str(#var1,var1)),strcat(ONS_Trace_Buf,ONS_Trace_Str(#var2,var2)),strcat(ONS_Trace_Buf,ONS_Trace_Str(#var3,var3)),strcat(ONS_Trace_Buf,ONS_Trace_Str(#var4,var4))))
#define ONS_TRACE3(msg,var1,var2,var3) if(ONS_Trace_Is_Active())ONS_Trace(msg,(strcpy(ONS_Trace_Buf,ONS_Trace_Str(#var1,var1)),strcat(ONS_Trace_Buf,ONS_Trace_Str(#var2,var2)),strcat(ONS_Trace_Buf,ONS_Trace_Str(#var3,var3))))
#define ONS_TRACE5(msg,var1,var2,var3,var4,var5) if(ONS_Trace_Is_Active())ONS_Trace(msg, (strcpy(ONS_Trace_Buf,ONS_Trace_Str(#var1,var1)),strcat(ONS_Trace_Buf,ONS_Trace_Str(#var2,var2)),strcat(ONS_Trace_Buf,ONS_Trace_Str(#var3,var3)),strcat(ONS_Trace_Buf,ONS_Trace_Str(#var4,var4)),strcat(ONS_Trace_Buf,ONS_Trace_Str(#var5,var5))))
#define ONS_TRACE_FLUSH  ONS_Trace_Flush()

#ifdef ONS_TRACE_OFF

#define ONS_TRACE(msg)
#define ONS_TRACE1(msg,var1)
#define ONS_TRACE2(msg,var1,var2)
#define ONS_TRACE3(msg,var1,var2,var3)
#define ONS_TRACE4(msg,var1,var2,var3,var4)
#define ONS_TRACE5(msg,var1,var2,var3,var4,var5)
#define ONS_TRACE_FLUSH  

#endif

// Prototypes

char * ONS_Trace_Str(const char * Name, int Var);
char * ONS_Trace_Str(const char * Name, unsigned int Var);
char * ONS_Trace_Str(const char * Name, char * Var);
char * ONS_Trace_Str(const char * Name, const char * Var);
char * ONS_Trace_Str(const char * Name, short int Var );
char * ONS_Trace_Str(const char * Name, unsigned short int Var);
char * ONS_Trace_Str(const char *Name, char Var );
char * ONS_Trace_Str(const char *Name, float Var);
char * ONS_Trace_Str(const char *Name, double Var);
char * ONS_Trace_Str(const char *Name, long Var);
char * ONS_Trace_Str(const char *Name, unsigned long Var);
char * ONS_Trace_Str(const char *Name, long long Var);
char * ONS_Trace_Str(const char *Name, unsigned long long Var);



void ONS_Trace(const char * msg, char * ONS_Trace_Buf);

void ONS_Trace_Enable(void);
void ONS_Trace_Disable(void);
int  ONS_Trace_Is_Active(void);

void ONS_Trace_Set_OUT(FILE * Trace_Out);

char * ONS_Trace_Get_Version(void);

void ONS_Trace_Flush(void);

// buffer used by the macro

extern char  ONS_Trace_Buf[];

#endif

