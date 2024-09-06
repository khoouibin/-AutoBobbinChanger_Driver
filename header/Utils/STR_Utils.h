// ---------------------------------------------------------------------------
//  Filename:  STR_Utils.h
//  Created by: Shmulik Hass
//  Date:  19.11.15
//  Orisol (c) 
// ---------------------------------------------------------------------------

#ifndef _STR_UTILS_H_
#define _STR_UTILS_H_


#define COMA_DELIMITER  ','
#define SPACE_DELIMITER  ' '
#define COLON_DELIMITER ':'



int Pharse_INI_Line(char*  line, char ** pName, char ** pValue);
int Pharse_Next_Element(char ** ppLine, char ** pElement, char Delimiter);
int Check_Str_Match(const char * Str1, const char * Str2, int Count);


int Check_Whitespace(char ch);
void Cut_End_WhiteSpaces(char * Str);
void Remove_End_LF(char *line);

int  Get_Braces_Value(char * Str, int * pInt_Value, int Min_Val, int Max_Val);
int  Get_Braces_FValue(char * Str, int * pInt_Value, int Min_Val, int Max_Val, int Factor);
int  Get_Braces_BMValue(char * Str, int * pInt_Value, int * pMask_Value, int NBits);
int  Get_Braces_BValue(char * Str, int * pInt_Value, int NBits);


int  Check_No_Postfix(char * Str);

#endif

