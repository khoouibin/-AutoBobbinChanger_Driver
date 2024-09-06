// ReceivMsg.h

#ifndef _RS485_COMM_H_
#define _RS485_COMM_H_

// #ifdef __cplusplus
// extern "C" {
// #endif
//	A6 modbus function define.
//	slave address.
#define _SlaveAddr_Xaxis 2
#define _SlaveAddr_Yaxis 1
#define _SlaveAddr_Zaxis 3
//  function code define
#define _func_code_ReadCoil 0x01
#define _func_code_ReadReg 0x03
#define _func_code_WriteCoil 0x05
#define _func_code_WriteReg 0x06
#define _func_code_CommDiag 0x08
#define _func_code_WrMulCoil 0x0f
#define _func_code_WrMulReg 0x10
//	Abnormal_response
#define _abnormal_READCOIL 0x81
#define _abnormal_READREG 0x83
#define _abnormal_WRITECOIL 0x85
#define _abnormal_WRITEREG 0x86
#define _abnormal_COMMDIAG 0x88
#define _abnormal_WRITEMULCOL 0x8F
#define _abnormal_WRITEMULREG 0x90

#define _except_AbnFuncCode 0x01
#define _except_AbnDataAddr 0x02
#define _except_AbnData 0x03
#define _except_AbnRespProc 0x04

//	data function code define
#define _error_code_READ 0x4001

#define _multiturnData_CLEAR 0x4100
#define _CLEAR_cmd_H 0x61
#define _CLEAR_cmd_L 0x65

#define TripResetFunc_CLEAR 0x4102
#define _TripResCmd_H 0x72
#define _TripResCmd_L 0x74

#define _singleturnData_READ 0x4202
#define _multiturnData_READ 0x4204
#define _servocoditionData_READ 0x0020
#define _servoInput_READ 0x0020
#define _servoReady_READ 0x00a0
#define _SF1_READ 0x003b
#define _SF2_READ 0x003c

#define _velocityValue_READ 0x4025

#include <stdio.h>
/*
typedef struct{
	char *array;
	size_t used;
	size_t size;
}array_t;
*/
typedef struct
{
	bool req;
	bool occupy; // if start = 1, other function cannot use rs485.
	std::vector<char> Tx;
	std::vector<char> Rx;
	Poco::Event g_Wakeup;
} rs485_Comm;

typedef struct
{
	int main_code;
	string strFuncName; // 要上傳給Host的字串名
	int clear_en;		// 0: clear-able. -1:cannot clear.
	string strSolution;
} errcode_data_t;

typedef struct
{
	int init_success;
	char gszTtyForModbusRtu[64];
} modbus_init_t;

//  function.
int modbusComm_Init();
void *receiv_modbusmsg(void *ptr);
// int     modbus_TxFunc( int _SlaveAddr, int _FuncCode, int _FuncAddr, char _data[], int _data_leng );
int modbusComm_checksum_cal(char *str_in, int input_len, unsigned char *ret_Hbyte, unsigned char *ret_Lbyte);
// int 	modbus_RxMerge( char in_str[], int in_leng );

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// int 	modbus_WarningReq( int _Driv_addr, unsigned char* _mainErr, unsigned char* _subErr );
// int 	modbus_WarningClrReq(int _Driv_addr);
// int 	modbus_MultiTurnClrReq(int _Driv_addr);
// int 	modbus_SingleTurnReq( int _Driv_addr, unsigned long* _singleTurn );
// int 	modbus_MultiTurnReq( int _Driv_addr, int* _multiTurn );

int modbusComm_WarningClr(int _Driv_addr);
int modbusComm_WarningReq(int _Driv_addr, unsigned char *_mainErr, unsigned char *_subErr);
int modbusComm_MotorMTurnsClr(int _Driv_addr);
int modbusComm_MotorMTurnsReq(int _Driv_addr, int *_multiTurn);
int modbusComm_MotorSTurnsReq(int _Driv_addr, int *_singleTurn);

int modbusComm_SRVON_read20h(int _Driv_addr, char *_servo_codition);
int modbusComm_SRDY_reada0h(int _Driv_addr, char *_SRDY_codition);
int modbusComm_SF1_read3Bh(int _Driv_addr, char *_SF1_codition);
int modbusComm_SF2_read3Ch(int _Driv_addr, char *_SF2_codition);
int modbusComm_VelocityValue_read4025h(int _Driv_addr, int *VelocityVal);
int modbus_Tx(vector<char> data);

int modbusComm_SetTty(char *pszTtyName);
int Get_modbusComm_Init_Res(void);
int Get_modbusComm_Occupy_Res(void);

int modbusComm_SRDY_sendWR(int _Driv_addr);
int modbusComm_SRDY_readVal(char *_SRDY_codition);
int Get_modbusComm_trig_SRDY(void);

// #ifdef __cplusplus
// }
// #endif

#endif //_RS485_COMM_H_
