#ifndef _BLCTRL_H_
#define _BLCTRL_H_

// send_profile.h
#include "Poco/Task.h"
#include "Poco/TaskManager.h"
#include "Poco/TaskNotification.h"
#include "Poco/NotificationCenter.h"
#include "Poco/NotificationQueue.h"
#include "Poco/Observer.h"
#include "Poco/Exception.h"
#include "notification_pkg.h"
#include <stdio.h>
#include "global.h"

#include "Poco/Timer.h"
#include "Poco/Thread.h"
#include "Poco/Stopwatch.h"
#include "Poco/Mutex.h"

using Poco::Stopwatch;
using Poco::Thread;
using Poco::Timer;
using Poco::TimerCallback;

typedef struct
{
	std::vector<int> data_addr;
	std::vector<vector<int>> row_data;
	int rec_type[5];
	char hex_filename[48];
} hex_sourcefile_t;

typedef struct
{
	std::vector<int> data_addr;
	std::vector<vector<int>> row_data;
	std::vector<unsigned char> row_chksum;
	int gen_seg_size;
	unsigned short chksum_gen; // not yet implement.
	unsigned short chksum_aux; // not yet implement.
} h32_sourcefile_t;

typedef struct
{
	int SST_sect;
	int pic_seg;
	int pthread_stop;
} h32_rdwr_para_t;

typedef struct
{
	string bl_title;
	string bl_description;
	string bl_date;
	string bl_version;
	string bl_filesize;
	string bl_h32_path;
	string bl_ini_path;
} ini_sourcefile_t;

typedef struct
{
	int i_total_msg;
	std::vector<int> i_curr_msg; // current message number.
	std::vector<int> i_tx_res;
	int i_send_ui;
} tx_h32_status_to_ui_t;

enum bl_cmd_define
{
	_Null = 0xff,
	_NOP = 0x00,	 // implement
	_BL_INIT = 0x01, // implement
	_ResetEn = 0x02,
	_Reset = 0x03,			 // implement
	_Read_Status_Reg = 0x04, // implement
	_Write_Status_Reg = 0x05,
	_Read_Configuration_Reg = 0x06,
	_Read = 0x07,
	_HighSpeed_Read = 0x08,
	_Read_Cont = 0x09,
	_Jedec_ID_Read = 0x0a, // implement
	_SFDP_Read = 0x0b,
	_WREN_SST26V = 0x0c, // implement
	_WRDI = 0x0d,		 // implement
	_Sector_Erase = 0x0e,
	_Block_Erase = 0x0f, // implement
	_Chip_Erase = 0x10,
	_Page_Program = 0x11,
	_Write_Suspend = 0x12,
	_Write_Resume = 0x13,
	_ReadBlockProtection = 0x14,  // implement
	_WriteBlockProtection = 0x15, // implement
	_LockBlockProtection = 0x16,
	_NonVolWriteLockProtection = 0x17,
	_Global_Block_Protection_Unlock = 0x18,
	_ReadSID = 0x19,
	_ProgSID = 0x1a,
	_LockSID = 0x1b,
	_RESET_PIC = 0x1c,			 // implement
	_RESET_SST26V = 0x1d,		 // implement
	_Read_Chksum_PIC = 0x1e,	 // 0: GenSeg, 1: AuxSeg.
	_Read_Chksum_SSTsect = 0x1f, // 0~10.
	_Resp_Chksum_PIC = 0x20,	 // opposite: Read_Chksum_PIC
	_Resp_Chksum_SSTsect = 0x21, // opposite: _Resp_Chksum_SSTsect
	_ConfigBit_RSTPRI = 0x22,
	_gotoAUX = 0x23,
	_nop_24 = 0x24,
	_dsPIC_AuxSegFlash = 0x25,
	_dsPIC_Erase_AuxSeg = 0x26,
	_Backup_GenSeg_SSTsect = 0x27,		// backup PIC-general segment to SST-sectN.
	_Backup_GenSeg_SSTsect_Resp = 0x28, // response.
	_Read_Version_PIC = 0x29,
	_Read_Version_SSTsect = 0x2a,
	_Read_Note_PIC = 0x2b,
	_Read_Note_SSTsect = 0x2c,
	_Read_SectInfo_n = 0x2d,
	_Write_SectInfo_n = 0x2e,
	_Read_SectBoot = 0x2f,
	_Write_SectBoot = 0x30,
}; //_READ_RTC_HEXVER

enum bl_respCode_define
{
	_positivResp = 0x00,
	_generalReject = 0x10,
	_servNotSupp = 0x11,
	_busyResp = 0x21,
};

typedef enum
{
	_Null_MemCtrl = 0x00,
	_HOST_READ_PIC16 = 0x01, // host request
	_HOST_READ_PIC32 = 0x02,
	_HOST_READ_SST16 = 0x03,
	_HOST_READ_SST32 = 0x04,
	_HOST_WRIT_SST16 = 0x05,
	_HOST_WRIT_SST32 = 0x06,
	_HOST_WRIT_SST48 = 0x07, // not impliment.
	_RTCr_READ_PIC16 = 0x11, // rtc relply
	_RTCr_READ_PIC32 = 0x12,
	_RTCr_READ_SST16 = 0x13,
	_RTCr_READ_SST32 = 0x14,
	_RTCr_WRIT_SST16 = 0x15, // 16bytes
	_RTCr_WRIT_SST32 = 0x16, // 32bytes
	_RTCr_WRIT_SST48 = 0x17, // not impliment.
} memCtrl_enum_t;

//-----------------------------------------------------------------------------
// Message Identification definition
//-----------------------------------------------------------------------------
#define MSG_PROT_BL_CmdReq 0xa0	 // host to rtc command
#define MSG_PORT_BL_CmdFbk 0xe0	 // rtc to host feedback
#define MSG_PROT_BL_DataReq 0xa1 // host to rtc data
#define MSG_PORT_BL_DataFbk 0xe1 // rtc to host feedback
//-----------------------------------------------------------------------------
#define BUSY_RespTime 100 //

#define SectInfo_addr 0
#define pSectInfo_chksumL 0
#define pSectInfo_chksumH 1
#define pSectInfo_rtcBase 2
#define pSectInfo_rtcMajo 3
#define pSectInfo_rtcMino 4
#define pSectInfo_protBase 5
#define pSectInfo_protMajo 6
#define pSectInfo_protMino 7
#define pSectInfo_blBase 8
#define pSectInfo_blMajo 9
#define pSectInfo_blMino 10
#define pSectInfo_null01 11
#define pSectInfo_null02 12
#define pSectInfo_null03 13
#define pSectInfo_null04 14
#define pSectInfo_null05 15
#define pSectInfo_blNote 16
#define pSectInfo_offset0 0
#define pSectInfo_offset1 64
#define pSectInfo_offset2 128
#define pSectInfo_offset3 192
#define pSectInfo_offset4 256
#define pSectInfo_offset5 320
#define pSectInfo_offset6 384
#define pSectInfo_offset7 448
#define pSectInfo_offset8 512
#define pSectInfo_offset9 576
#define pSectInfo_offset10 640

#define pSectBoot_addr 0x1000
#define pSectBoot_SectNo 0
#define pSectBoot_chksum 1

typedef struct
{
	int RTC_Base;
	int RTC_Major;
	int RTC_Minor;
	int MsgProt_Base;
	int MsgProt_Major;
	int MsgProt_Minor;
	int BL_Base;
	int BL_Major;
	int BL_Minor;
} VersionInfo_t;

typedef struct
{
	unsigned short sect_chksum;
	VersionInfo_t sect_verinfo;
	char sect_msg[48];
} SectInfo_n_t; // n:0~10.

typedef struct
{
	unsigned char sect_num;
	unsigned char s_chksum; // =0xff - sect_num + 1;
} SectBootInfo_t;

typedef struct
{
	int segment; // 0:general, 1: aux.
	unsigned short chksum;
} Read_segchksum_t;

typedef struct
{
	unsigned char manufa_id;
	unsigned char device_typ;
	unsigned char device_id;
} Jedec_ID_t;

typedef struct
{
	int BUSY; // 1:in process,                        0:not in process.
	int WEL;  // 1:write-enable,                      0:not write-enable.
	int WSE;  // 1:erase-suspend,                     0:not erase-suspend.
	int WSP;  // 1:program-suspend,                   0:not program-suspend.
	int WPLD; // 1:write-protection lock-down enable, 0:disable.
	int SEC;  // 1:security-locked,                   0:not security-locked.
	int RES;  //  reserved.
} SST26_status_t;

// Message structure
typedef struct
{ // encode message to RTC
	unsigned short Id;
	unsigned short null_size; // size.
	unsigned int null_sn;	  // serial number.
	unsigned int null_ts;	  // time stamp.
	unsigned char bl_cmd;
	unsigned char cmd_data[3]; // cmd_data[3], address[3]
	unsigned char ext_data[20];
} Msg_Prot_BL_ToRTC_CMD_t;

typedef struct
{ // decode message from RTC.
	unsigned char bl_cmd;
	unsigned char cmd_data[3]; // cmd_data[3], address[3].
	unsigned char ext_data[20];
} Msg_Prot_BL_fromRTC_CMD_t;

typedef struct
{ // from RTC.
	bool req;
	Poco::Event wait_cmdfbk;
	Msg_Prot_BL_fromRTC_CMD_t MSG_BL_RTC_cmd;
} bl_RTC_cmd_msg_t;
//-----------------------------------------------------------------------------

typedef union
{
	int int_addr;
	struct
	{
		unsigned char LLB;
		unsigned char LHB;
		unsigned char HLB;
		unsigned char HHB;
	} bytes;
} sst_addr_t;

typedef struct
{ // encode message from RTC
	unsigned char type;
	unsigned char addr[3];	// cmd_data[3], address[3]
	unsigned char data[32]; // cmd_data[3], address[3]
	unsigned char chksum;
	unsigned char chksumH;
} Msg_Prot_BL_fromRTC_MEM_t;

typedef struct
{ // encode message from RTC
	unsigned char type;
} Msg_Prot_BL_fromRTC_MEMtype_t;

typedef struct
{ // encode message from RTC: require mem then rtc return mem data.
	unsigned char type;
	unsigned char addr[3];	// cmd_data[3], address[3]
	unsigned char data[32]; // cmd_data[3], address[3]
} Msg_Prot_BL_fromRTC_MEMread_t;

typedef struct
{ // encode message from RTC: send mem data then rtc response checksum.
	unsigned char type;
	unsigned char chksum;
	unsigned char chksumH;
} Msg_Prot_BL_fromRTC_MEMecho_t;

typedef struct
{ // encode message to RTC
	unsigned short Id;
	unsigned short null_size; // size.
	unsigned int null_sn;	  // serial number.
	unsigned int null_ts;	  // time stamp.
	unsigned char type;
	unsigned char addr[3];	// cmd_data[3], address[3], reply checksum.
	unsigned char data[32]; // cmd_data[3], address[3]
} Msg_Prot_BL_ToRTC_MEM_t;

typedef struct
{ // from RTC.
	Poco::Event wait_mem16fbk;
	Msg_Prot_BL_fromRTC_MEMread_t MSG_BL_RTC_mem;
} bl_RTC_mem_16_t;

typedef struct
{ // from RTC.
	Poco::Event wait_mem32fbk;
	Msg_Prot_BL_fromRTC_MEMread_t MSG_BL_RTC_mem;
} bl_RTC_mem_32_t;
typedef struct
{ // from RTC.
	Poco::Event wait_memechofbk;
	Msg_Prot_BL_fromRTC_MEMecho_t MSG_BL_RTC_mem;
} bl_RTC_mem_echo_t;

typedef struct
{
	Poco::Event g_WriteHexRowService_idle;
	hex_sourcefile_t hex_sour;
	int WriteHexRow_start;
	int WriteHexRow_abort_trigger;
	int WriteHexRow_rtc_response_timeout;
	bool WriteHex_tx_info_enable;
} bl_WriteHexRowService_t;

typedef enum
{
	DEVICE_NULL = -1,
	DEVICE_ONS = 1,
	DEVICE_BL = 2
} Device_State_t;

typedef enum
{
	DevNull_0 = 0,
} DevNull_Flow_t;

typedef enum
{
	ONS_DEFAULT = 0,
	ONS_READ_VER_NUM = 1,
	ONS_SWITCH_TO_BL = 2,
	ONS_SWITCH_SUCCESS = 3,
	ONS_READ_VER_NUM_FAULT = 11,
	ONS_SWITCH_TO_BL_FAULT = 12,
	ONS_MSG_TIMEOUT = 20,
} DevONS_Flow_t;

typedef enum
{
	BL_DEFAULT = 0,
	BL_LOAD_HEXFILE = 1,
	BL_SEND_HEXFILE = 2,
	BL_WAIT_WRHEX = 3,
	BL_COMP_CHECKSUM = 4,
	BL_SWITCH_TO_ONS = 5,
	BL_SWITCH_SUCCESS = 6,
	BL_LOAD_HEXFILE_FAULT = 11,
	BL_SEND_HEXFILE_FAULT = 12,
	BL_WAIT_WRHEX_FAULT = 13,
	BL_COMP_CHECKSUM_FAULT = 14,
	BL_SWITCH_TO_ONS_FAULT = 15,
	BL_MSG_TIMEOUT = 20,
} DevBL_Flow_t;

typedef struct
{
	Poco::Event g_Endurance_Test_idle;
	int Endurance_Test_cycle;
	int Endurance_Test_start;
	int Endurance_Test_abort_trigger;
	int Endurance_Test_state_error;
	Device_State_t Endurance_DeviceState;
	DevNull_Flow_t Endurance_NULL_State;
	DevONS_Flow_t Endurance_ONS_State;
	DevBL_Flow_t Endurance_BL_State;
	unsigned short hexfile_gen_chksum;
	char hexfile1_path[48];
	char hexfile2_path[48];
	char hexfile3_path[48];
	hex_sourcefile_t hexfile_hexsour;
} bl_Endurance_Test_t;

// function.
int time_tag(timetagsel_t _typ, timetag_t *t_tag);
vector<string> bl_helper_doc(void);
int bl_ReadHexSource(char *file_name);
int bl_ExportHex32(char *new_name_h32);
int bl_ReadHex32(char *file_name_h32);
int bl_ExportHex16(char *new_name_h16);
int bl_ReadHex16(char *file_name_h16);

void bl_txH32_Stop(void);
int bl_Wr_Sect_by_H32(int SST_sect);
void *writeSSTsect_Hex32(void *ptr);
int bl_Wr_Sect_by_H16(int SST_sect);
void *writeSSTsect_Hex16(void *ptr);

unsigned int HexStringToInt(std::string &input);
string int2str(int &i);
int bl_sstWREN(void);
int bl_sstWRDI(void);

int bl_nop(void);
int bl_init(void);
int bl_Jedec_ID_Read(Jedec_ID_t *id);

int bl_sstWriteBP(vector<unsigned char> bp_reg);
int bl_sstWriteBP_section(int sectN, bool mask); // replace: bl_sstWritBP_11bit
int bl_sstWriteBP_SectInfo(bool mask);
int bl_sstReadBP(unsigned char *bp_reg);
int bl_sstReadStatus(SST26_status_t *status);

int bl_block_erase(int addr);
int bl_block_erase_section(int sectN);
int bl_sector_erase(int addr);
int bl_chip_erase(void);

int bl_dsPIC_EraseAux(void);
int bl_dsPIC_AuxSegFlash(void);

int bl_read_sstMEM16(int addr, int *read_addr, unsigned char *byte_16);
int bl_write_sstMEM16(int addr, vector<unsigned char> write16, unsigned char *chksum, unsigned char *chksumH);

int bl_read_sstMEM32(int addr, int *read_addr, unsigned char *byte_32); // size:32bytes, fixed.
int bl_write_sstMEM32(int addr, vector<unsigned char> write32, unsigned char *chksum, unsigned char *chksumH);

int bl_read_picMEM32(int addr, int *read_addr, unsigned char *byte_32);
int bl_read_picMEM16(int addr, int *read_addr, unsigned char *byte_16);

void bl_sstRESET(void);
void bl_picRESET(void);
void bl_gotoAUX(void);
int bl_set_ConfigBit_RSTPRI(int seg_sel, unsigned char *ConfigBit);

//---------------mar 08. 2021.
// pic/sst.
int bl_read_chksum_PIC(int seg_sel, Read_segchksum_t *seg_checksum);
int bl_read_chksum_SST(int sectN, int seg_sel, Read_segchksum_t *seg_checksum);
//  only GenSeg chksum-read.
// inner H32/H16.
int bl_Read_GenSegChksum_Hex32(unsigned short *chksum);
int bl_Read_AuxSegChksum_Hex32(unsigned short *chksum);
int bl_Read_GenSegChksum_Hex16(unsigned short *chksum);
int bl_Read_AuxSegChksum_Hex16(unsigned short *chksum);

int bl_backup_GenSeg_SSTsection(int sectN);

// encode/decode protocol.
int Msg_BL_fromRTC_CMD(ONS_BYTE *Msg_Buff);
int Msg_Prot_BL_Host_to_RTC_encodeCmd(unsigned char bl_cmd, unsigned char *cmd_data, unsigned char *ext_data);
int Msg_Prot_BL_fromRTC_Command_decode(ONS_BYTE *Msg_Buff, unsigned char *bl_cmd, unsigned char *cmd_data, unsigned char *ext_data);
int Msg_Prot_BL_Host_to_RTC_encodeMEMRead(memCtrl_enum_t mem_ctrl, int addr);

int Msg_BL_fromRTC_MEM(ONS_BYTE *Msg_Buff);
int Msg_Prot_BL_fromRTC_MEM16read_decode(ONS_BYTE *Msg_Buff, bl_RTC_mem_16_t *memread16);
int Msg_Prot_BL_fromRTC_MEM32read_decode(ONS_BYTE *Msg_Buff, bl_RTC_mem_32_t *memread32);
int Msg_Prot_BL_fromRTC_MEMecho_decode(ONS_BYTE *Msg_Buff, bl_RTC_mem_echo_t *memecho);

int Msg_Prot_BL_Host_to_RTC_encodeMEMWrite(unsigned char mem_ctrl, int addr, unsigned char *data);

int Msg_BL_fromRTC_DATA(ONS_BYTE *Msg_Buff);

string get_tnow(void);
unsigned char hex_chksum_cal(string row_data);
unsigned char hex32_chksum_gen(string row_data);
unsigned char hex16_chksum_gen(string row_data);

int bl_read_h32_VersionNum(VersionInfo_t *ver_fbk);
int bl_read_pic_VersionNum(VersionInfo_t *ver_fbk);
int bl_read_SST_VersionNum(int sectN, VersionInfo_t *ver_fbk);

int bl_read_h32_BLmessage(char *bl_msg);
int bl_read_pic_BLmessage(char *bl_msg);
int bl_read_sst_BLmessage(int sectN, char *bl_msg);

int bl_read_SectInfo_n(int sectN, SectInfo_n_t *fbk_info_n);
int bl_write_SectInfo_n(int sectN, string sour_sel);

int bl_read_SectInfo_All(void);
int bl_show_SectInfo_All(void);

int bl_read_SectBoot(SectBootInfo_t *fbk_sectboot);
int bl_write_SectBoot(int sectN);

int rc5_key_setting(char *key_in);
int rc5_crypt(vector<unsigned char> data_in, vector<unsigned char> *data_crypt);
int rc5_uncrypt(vector<unsigned char> data_in, vector<unsigned char> *data_crypt);
int rc5_encryptfile(char *file_name);
// int rc5_decryptfile(char *file_name);
int rc5_decryptfile(char *file_name, char **decrypt_filename);

int bl_readme_inifile(char *file_name, ini_sourcefile_t *readme_info);
int bl_read_filesize(char *file_name, string *str_size);

int shellCmd(const string &cmd, string &result);
double rounding(double num, int index);

int poco_periodictimer(int peridoic_ms);
int poco_periodictimerStop(void);

class PeriodicTimer
{
public:
	PeriodicTimer();
	void PeriodicTimer_trig(Timer &timer);
	void PeriodicTimer_callback();
	void PeriodicTimer_setting(int t0, int tn);
	void PeriodicTimer_stop();
	void PeriodicTimer_zero();
	Timer timer;

private:
	Stopwatch _sw;
	int cnt;
};

int bl_LoadHexSour(char *file_name, hex_sourcefile_t *hexsour);
int bl_Read_HexFile_Range_Checksum(hex_sourcefile_t *hex_sour, int StartAddr, int End_Addr, unsigned short *fbk_chksum);

int bl_WrHexRow_Thread_Init(void);
void *bl_WriteHexRowThread_Main_32(void *p);
int bl_WrHexRow_Thread_Terminate(void);
int bl_WrHexRow_Wakeup(hex_sourcefile_t *hex_sour, bool tx_info_enable);
int bl_WrHexRow_Abort();

int bl_EnduranceTest_Thread_Init(void);
void *bl_EnduranceTest_Main(void *p);
int bl_EnduranceTest_Thread_Terminate(void);
int bl_EnduranceTest_Wakeup(int cycle);
int bl_EnduranceTest_Abort();

string var_to_string(int var, int setw_size);

int RTC_Device_Switch(string switch_cmd);

void TxMsg_Info_percentage(int num, int den);
int TxMsg_WriteHexThread_Info(string info, int status);

void Set_BL_Driver_Run_Terminate(void);
int Get_BL_Driver_Run_Terminate_Status(void);
#endif //_HOMEMODE_H_
