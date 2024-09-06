#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>

#include "USBComm.h"
#include "USBComm_Driver.h"
#include "Msg_Prot.h"
#include "ReceivMsg.h"

#include "interface_to_host.h"
#include "logger_wrapper.h"
// #include "return_code.h"
#include "bl_ctrl.h"
#include <time.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <cstdio>
#include <iomanip>
#include <mutex>
#include <sstream>

#include "json.hpp"
#include <bits/stdc++.h>

#include "RC5Simple.h"
#include "iniparser.h"
#include <cmath>

#include "interface_driver_to_ui.h"
#include "BL_USBComm.h"

using namespace std;

using Poco::Stopwatch;
using Poco::Thread;
using Poco::Timer;
using Poco::TimerCallback;

Poco::Mutex poco_mutex;
PeriodicTimer k1;
tx_h32_status_to_ui_t downloading_h32_status;

// new.
hex_sourcefile_t hex_source;
// h32_sourcefile_t   h48_source;
h32_sourcefile_t h32_source;
h32_sourcefile_t h16_source;

ini_sourcefile_t h32_readme;

h32_rdwr_para_t wr_sect_by_h32; // wr_sect_by_h32;
h32_rdwr_para_t wr_sect_by_h16;
// h32_rdwr_para_t    list_sst_sect;
// h32_rdwr_para_t    list_pic;
pthread_t rdwr_h32pthread;

bl_RTC_cmd_msg_t BL_RTC_CmdMsg; // cmd  message response from rtc.
// bl_RTC_mem_msg_t  BL_RTC_MemMsg;	// memory message response from rtc.
bl_RTC_mem_16_t BL_RTC_MemRead16;
bl_RTC_mem_32_t BL_RTC_MemRead32;
bl_RTC_mem_echo_t BL_RTC_MemEcho;

std::mutex mutex_writHEX;
std::mutex mutex_CmdMsg;

unsigned char bl_msg_test1[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};			 // 16bytes.
unsigned char bl_msg_test2[] = {21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36}; // 16bytes.
const int SST26V_SECTBASE = 0x10000;
unsigned char SST_SectBase[11] = {0x1, 0xc, 0x17, 0x22, 0x2d, 0x38, 0x43, 0x4e, 0x59, 0x64, 0x6f};
unsigned char sst_SectInfo[704]; // 64*11=704.

bool bl_driver_run = true;

// rc5simple.
vector<unsigned char> v_rc5key{'o', 'r', 'i', 's', 'o', 'l', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

std::vector<SectInfo_n_t> Vec_SectInfo;

pthread_t WriteHexRow_Thread;
bl_WriteHexRowService_t WriteHex_32bytes;
bl_Endurance_Test_t BL_EnduranceTest;
bool g_b_WrHexRow_run = true;

pthread_t BLEndurance_Thread;
bool g_b_BLEndurance_run = true;

// Lily Ballard...
template <typename charT, typename traits = std::char_traits<charT>>
class center_helper
{
	std::basic_string<charT, traits> str_;

public:
	center_helper(std::basic_string<charT, traits> str) : str_(str) {}
	template <typename a, typename b>
	friend std::basic_ostream<a, b> &operator<<(std::basic_ostream<a, b> &s, const center_helper<a, b> &c);
};

template <typename charT, typename traits = std::char_traits<charT>>
center_helper<charT, traits> centered(std::basic_string<charT, traits> str)
{
	return center_helper<charT, traits>(str);
}

// redeclare for std::string directly so we can support anything that implicitly converts to std::string
center_helper<std::string::value_type, std::string::traits_type> centered(const std::string &str)
{
	return center_helper<std::string::value_type, std::string::traits_type>(str);
}

template <typename charT, typename traits>
std::basic_ostream<charT, traits> &operator<<(std::basic_ostream<charT, traits> &s, const center_helper<charT, traits> &c)
{
	std::streamsize w = s.width();
	if (w > c.str_.length())
	{
		std::streamsize left = (w + c.str_.length()) / 2;
		s.width(left);
		s << c.str_;
		s.width(w - left);
		s << "";
	}
	else
	{
		s << c.str_;
	}
	return s;
}

vector<string> bl_helper_doc(void)
{
	vector<string> doc;
	doc.push_back(" config command:");
	doc.push_back(" -----------------------------------------------------------------");
	doc.push_back(" 'bl nop'               : wake-up 7-seg disp for debug. ");
	doc.push_back(" 'bl init'              : init1: SPI( control SST26V ), init2: 7-seg-disp IO");
	doc.push_back(" 'bl wren'              : set SST26V-write en bit");
	doc.push_back(" 'bl wrdi'              : clr SST26V-write en bit");
	doc.push_back(" 'bl reset pic'         : reset RTC-PIC mcu");
	doc.push_back(" 'bl reset sst'         : reset RTC-SST26V");
	doc.push_back(" -----------------------------------------------------------------");
	doc.push_back(" READ out infomation.");
	doc.push_back(" 'bl rd sstid'          : return SST26V-ID values");
	doc.push_back(" 'bl rd bp              : return SST26V block-protection[0~17]");
	doc.push_back(" 'bl rd status          : return SST26V status register");
	doc.push_back(" 'bl rd sst32/sst16 addr: return SST-memory data( 32/16 bytes )");
	doc.push_back(" 'bl rd pic32/pic16 addr: return PIC-memory data( 32/16 bytes )");
	doc.push_back(" 'bl rd chksum pic 0/1' : return chksum, pic GenSeg/AuxSeg.");
	doc.push_back(" 'bl rd chksum h32 0/1' : return chksum, inner H32 GenSeg/AuxSeg.");
	doc.push_back(" 'bl rd chksum h16 0/1' : return chksum, inner H16 GenSeg/AuxSeg.");
	doc.push_back(" 'bl rd chksum sst 8 0' : return chksum, SST-sect general-segment.");
	doc.push_back(" 'bl rd version h32/pic': return version number from inner H32 / pic.");
	doc.push_back(" 'bl rd version sst N'  : return version number from sst32-N.");
	doc.push_back(" 'bl rd note h32/pic'   : return bl-message form inner H32 / pic.");
	doc.push_back(" 'bl rd note sst N'     : return bl-message form sst32-N.");
	doc.push_back(" 'bl rd part-info N'    : return part infomation from SST-sectinfo");
	doc.push_back(" 'bl rd all-info'       : return all  infomation from SST-sectinfo");
	doc.push_back(" 'bl rd sect-boot'      : return sect-boot variable value");
	doc.push_back(" -----------------------------------------------------------------");
	doc.push_back(" WRITE out infomation.");
	doc.push_back(" 'bl wr bp all-0/all-1/def': set SST26V BPREG all-0(write-able)/all-1(write-forbidden)/default");
	doc.push_back(" 'bl wr bp sect-info 0/1  : set SST26V block-protection (sect-list: 0x0000~0x1fff)");
	doc.push_back(" 'bl wr bp vec7'          : set SST26V block-protection vec7(bp vector function test)");
	doc.push_back(" 'bl wr bp sect sectN 1'  : set SST26V block-protection sectN(0-10) mask(1/0)");
	doc.push_back(" 'bl wr be ADDR'          : set SST26V block-erase ADDR( 0x0000 ~ 0x7f ffff )");
	doc.push_back(" 'bl wr be-sect sectN'    : set SST26V block-erase sectN(0-10)");
	doc.push_back(" 'bl wr se ADDR'          : set SST26V sector-erase(4kbytes) ADDR(0x0000 ~ 0x7f ffff)");
	doc.push_back(" 'bl wr ce'               : set SST26V chip-erase");
	doc.push_back(" 'bl wr sst32 ADDR ex1/2/3: write SST-memory data(32bytes) ex1/ex2/ex3 addr( 0x0000 ~ 0x7f ffff ), make sure mem-0xff & bp=0");
	doc.push_back(" 'bl wr sst32 h32 sectN   : set SST-memory data(32bytes) from h32-source(General-Segment) to sectN(0-10)");
	doc.push_back(" 'bl wr sst16 ex1 ADDR    : set SST-memory data(16bytes) ex1/ex2/ex3 addr( 0x0000 ~ 0x7f ffff )");
	doc.push_back(" 'bl wr part-info h32/clr N': write sect-info to SST-sectinfo");
	doc.push_back(" 'bl wr sect-boot'        : write sect-boot variable value");
	doc.push_back(" -----------------------------------------------------------------");
	doc.push_back(" LOAD with file.");
	doc.push_back(" 'bl load hex xxx.hex'  : load original RTC-hex file, prepare to gen h32/h16 file");
	doc.push_back(" 'bl load h32 xxx.h32'  : load h32 file 32byte-rowdata file, prepare to RTC");
	doc.push_back(" 'bl load h16 xxx.h16'  : load h16 file 16byte-rowdata file, prepare to RTC");
	doc.push_back(" -----------------------------------------------------------------");
	doc.push_back(" EXPORT to file.");
	doc.push_back(" 'bl export h32 xxx'    : export xxx.h32, source from original RTC-hex file");
	doc.push_back(" 'bl export h16 xxx'    : export xxx.h16, source from original RTC-hex file");
	doc.push_back(" -----------------------------------------------------------------");
	doc.push_back(" LIST to file (unimpliment).");
	doc.push_back(" 'bl list pic-gen'      : export pic-gen.list, source from pic-general segment memory");
	doc.push_back(" 'bl list pic-aux'      : export pic-aux.list, source from pic-auxiliary segment memory");
	doc.push_back(" 'bl list sect N(0~10)' : export sect-N.list, source from SST-sect general segment memory");
	doc.push_back(" 'bl list sect-info'    : show sect-info table");
	doc.push_back(" -----------------------------------------------------------------");
	doc.push_back(" BACKUP to SST-sect.");
	doc.push_back(" 'bl backup sectN'      : copy pic gen-segment to SST sectN(0-10)");
	doc.push_back(" -----------------------------------------------------------------");
	doc.push_back(" RC5 to decrypt/encrypt.");
	doc.push_back(" 'bl rc5 key xxx'                : gen string xxx to RC5key, xxx (max 16 chars, empty fill-up '0' automatical)");
	doc.push_back(" 'bl rc5 str_enco yyy'           : enco string 'yyy' with RC5key to    'v_rc5_uncrypt'(vector) ");
	doc.push_back(" 'bl rc5 str_deco'               : deco 'v_rc5_uncrypt' with RC5key to 'v_rc5_uncrypt'(vector) ");
	doc.push_back(" 'bl rc5 enfile zzz.xxx'         : enco file with RC5key to 'zzz.xxx-encrypt");
	doc.push_back(" 'bl rc5 defile zzz.xxx-encrypt' : deco en-file with RC5key to 'zzz.xxx-encrypt-decrypt ");
	doc.push_back(" -----------------------------------------------------------------");
	doc.push_back(" Bootloader procedure.");
	doc.push_back(" 1: bl load h32 file");
	doc.push_back(" 2: bl wr sst32 h32 1~10");
	doc.push_back(" 3: bl wr sect-boot 1~10");
	doc.push_back(" 4: bl rd all-info");
	doc.push_back(" 5: bl goaux");
	doc.push_back(" *: bl sp (STOP write sst32) ");
	for (int i = 0; i < doc.size(); i += 1)
		cout << doc[i] << endl;

	return doc;
}
int time_tag(timetagsel_t _typ, timetag_t *t_tag)
{
	// _typ = 0 : put tn to t_n[0],t_n[1] together.
	// _typ = 1 : put tn to t_n[1] only.
	// _typ = 2 : cal delta_t_n[1] - t_n[0].
	// constraint: delta_t must less TAG_MAX(16.6 minutes) nor wrong calculation.

	int res = 0;
	int const TAG_MAX = 1000000;

	struct timeval tv;
	unsigned int tn = 0;
	gettimeofday(&tv, NULL);

	tn = (((unsigned int)tv.tv_sec) % 1000) * 1000;
	tn = tn + ((unsigned int)tv.tv_usec / 1000);

	if (_typ == tag_all)
	{
		t_tag->t_n[0] = tn;
		t_tag->t_n[1] = tn;
	}
	else if (_typ == tag_now)
	{
		t_tag->t_n[1] = tn;
	}
	else if (_typ == tag_delta)
	{
		t_tag->t_n[1] = tn;

		if (t_tag->t_n[1] >= t_tag->t_n[0])
			t_tag->delta_tn = t_tag->t_n[1] - t_tag->t_n[0];
		else
		{
			t_tag->delta_tn = TAG_MAX - t_tag->t_n[0] + t_tag->t_n[1];
			res = 1;
		}
		usleep(10);
	}
	return res;
}

//-----------------------------------------------------------------------------
int bl_ReadHexSource(char *file_name)
{ // intel hex ref: https://en.wikipedia.org/wiki/Intel_HEX

	// res = -1: file not exist.
	// res = -2: file format fault. startcode not ":"
	// res = -3: file format fault. bytecount > 16.
	// res = -4: file format fault. record type > 5.
	int res = 0;
	int i_line = 0;
	int b_filevalid;
	int i_bytecount;
	int i_recordtyp;
	int i_addrH, i_addrL, i_addr;

	int i_bytecountMAX = 16;
	int i_rectyp_valid = 5; // valid: 0x00, 0x01, 0x02, 0x03, 0x04, 0x05.
	int i_rowchksum;
	int i_rowchksum_cal;
	int i_bytedata;

	fstream source_file;
	string file_data;
	string str_startcode;
	string str_bytecount;
	string str_recordtyp;
	string str_rowchksum;

	string str_addrH, str_addrL, str_addr;
	string str_rowdata, str_bytedata;

	b_filevalid = access(file_name, F_OK);
	if (b_filevalid == 0)
		res = 0;
	else if (b_filevalid == -1)
	{
		std::string log = file_name;
		log = "'" + log + "'" + " file is not exist.";
		goDriverLogger.Log("warning", log);
		res = -1;
	}

	if (res < 0)
		return res;

	source_file.open(string(file_name).c_str());

	//  1. check startcode ":"
	//  2. check bytecount > 16
	//  3. check record type: 00, 01, 02, 03, 04, 05.
	//  4. check row checksum match.
	while (source_file >> file_data)
	{
		str_startcode = str_startcode.assign(file_data, 0, 1); // start 0, size:1.
		str_bytecount = str_bytecount.assign(file_data, 1, 2); // start 1, size:2.
		i_bytecount = HexStringToInt(str_bytecount);

		str_recordtyp = str_recordtyp.assign(file_data, 7, 2); // start 7, size:2.
		i_recordtyp = HexStringToInt(str_recordtyp);

		// if( i_recordtyp == )
		str_rowdata = str_rowdata.assign(file_data, 9, i_bytecount * 2);

		str_addrH = str_addrH.assign(file_data, 3, 2);
		str_addrL = str_addrL.assign(file_data, 5, 2);
		i_addrH = HexStringToInt(str_addrH);
		i_addrL = HexStringToInt(str_addrL);

		str_rowchksum = str_rowchksum.assign(file_data, 9 + i_bytecount * 2, 2);
		i_rowchksum = HexStringToInt(str_rowchksum);
		i_line++;

		//------row checksum calculation.
		i_rowchksum_cal = hex_chksum_cal(file_data);
		// printf("row_checksum: 0x%02X\n",i_rowchksum_cal);

		if (str_startcode != ":")
		{
			goDriverLogger.Log("warning", "file format, start code fault");
			res = -2;
			break;
		}

		if (i_bytecount > i_bytecountMAX)
		{
			goDriverLogger.Log("warning", "file format, byte count fault");
			res = -3;
			break;
		}

		if (i_recordtyp > i_rectyp_valid)
		{
			goDriverLogger.Log("warning", "file format, record type fault");
			res = -4;
			break;
		}

		if (i_rowchksum_cal != i_rowchksum)
		{
			goDriverLogger.Log("warning", "file format, row checksum fault,line_num:" + std::to_string(i_line));
			res = -5;
			break;
		}
	}

	source_file.close();
	if (res == 0)
	{
		std::string log = file_name;
		log = "'" + log + "'" + " file format check successfully.";
		goDriverLogger.Log("info", log);
	}

	if (res < 0)
		return res;

	//  save data.
	vector<int> row_bytes;
	string str_onebyte;
	string str_addrshift;

	int i_onebyte;
	int i_addr_shift;
	int i_addr_offset = 0;
	source_file.open(string(file_name).c_str());

	hex_source.data_addr.clear();
	hex_source.row_data.clear();
	hex_source.rec_type[0] = 0;
	hex_source.rec_type[1] = 0;
	hex_source.rec_type[2] = 0;
	hex_source.rec_type[3] = 0;
	hex_source.rec_type[4] = 0;

	while (source_file >> file_data)
	{
		str_bytecount = str_bytecount.assign(file_data, 1, 2); // start 1, size:2.
		i_bytecount = HexStringToInt(str_bytecount);
		//		hex_source.bytecount.push_back(i_bytecount );

		str_recordtyp = str_recordtyp.assign(file_data, 7, 2); // start 7, size:2.
		i_recordtyp = HexStringToInt(str_recordtyp);
		if (i_recordtyp == 0)
			hex_source.rec_type[0] += 1;
		else if (i_recordtyp == 1)
			hex_source.rec_type[1] += 1;
		else if (i_recordtyp == 2)
			hex_source.rec_type[2] += 1;
		else if (i_recordtyp == 3)
			hex_source.rec_type[3] += 1;
		else if (i_recordtyp == 4)
			hex_source.rec_type[4] += 1;

		row_bytes.clear();
		if (i_recordtyp == 0)
		{
			str_rowdata = str_rowdata.assign(file_data, 9, i_bytecount * 2);
			for (int i = 0; i < i_bytecount; i++)
			{
				str_onebyte = str_onebyte.assign(str_rowdata, i * 2, 2);
				i_onebyte = HexStringToInt(str_onebyte);
				row_bytes.push_back(i_onebyte);
			}
			hex_source.row_data.push_back(row_bytes);

			str_addr = str_addrH.assign(file_data, 3, 4);
			i_addr = HexStringToInt(str_addr);
			i_addr |= i_addr_offset;
			// printf("i_addr: 0x%04x\n",i_addr );
			hex_source.data_addr.push_back(i_addr);
		}
		else if (i_recordtyp == 1)
		{
			// cout<<"end of hex"<<endl;
			break;
		}
		else if (i_recordtyp == 2)
		{
			cout << "xc16 without this type:0x02" << endl;
			continue;
		}
		else if (i_recordtyp == 3)
		{
			cout << "xc16 without this type:0x03" << endl;
			continue;
		}
		else if (i_recordtyp == 4)
		{
			str_addrshift = str_rowdata.assign(file_data, 9, 4);
			i_addr_shift = HexStringToInt(str_addrshift);
			// printf("addr_shift: 0x%04x\n",i_addr_shift );
			i_addr_offset = i_addr_shift;
			i_addr_offset = (i_addr_offset << 16) & 0xffff0000;
			continue;
		}
	}
	source_file.close();

	cout << "rec_type:[" << hex_source.rec_type[0] << ",";
	cout << hex_source.rec_type[1] << ",";
	cout << hex_source.rec_type[2] << ",";
	cout << hex_source.rec_type[3] << ",";
	cout << hex_source.rec_type[4] << "]" << endl;

	cout << "size:data_addr:" << hex_source.data_addr.size() << endl;
	cout << "size:row_data:" << hex_source.row_data.size() << endl;
	return res;
}

int bl_ExportHex32(char *new_name_h32)
{
	// res = -1: hex source is not exist.
	// res = -2: new_name_h32 size "0"
	// res = -3: file format fault. bytecount > 16.
	// res = -4: file format fault. record type > 5.
	int res = 0;
	if (hex_source.data_addr.empty())
	{
		goDriverLogger.Log("warning", "hex source.address are empty, load hex source file first");
		res = -1;
		return res;
	}
	if (hex_source.row_data.empty())
	{
		goDriverLogger.Log("warning", "hex source.row_data are empty, load hex source file first");
		res = -1;
		return res;
	}
	// create a hex file for 32bytes.
	string file_ext = ".h32";
	string str_now = get_tnow();
	string newfile_name = new_name_h32;
	cout << "str_size:" << newfile_name.size() << endl;
	if (newfile_name.size() == 0)
	{
		goDriverLogger.Log("warning", "input file_name size = 0");
		res = -2;
		return res;
	}

	string::size_type dot_pos;
	dot_pos = newfile_name.find(".");
	if (dot_pos != newfile_name.npos)
	{
		cout << "dot_pos:" << dot_pos << endl;
		if (dot_pos == 0)
		{
			goDriverLogger.Log("warning", "input file_name size = 0, dot in fornt.");
			res = -3;
			return res;
		}
		newfile_name.erase(newfile_name.begin() + dot_pos, newfile_name.end());
	}
	newfile_name = newfile_name.append(file_ext);
	cout << "newfile_name: " << newfile_name << endl;

	ofstream fileout(newfile_name.c_str(), ios::out | ios::trunc);
	fileout.close();
	chmod(newfile_name.c_str(), 0777);
	chown(newfile_name.c_str(), 1000, 1000);

	fstream file_h32;
	file_h32.open(newfile_name.c_str(), ios::app);

	int hexsour_size = hex_source.data_addr.size();
	int i_vect_ptr = 0;
	int i_curr_addr;
	int i_pred_next_addr;
	int i_real_next_addr;
	int i_data_size;
	unsigned char h32_row_chksum;

	string str_pushdata;
	string str_pushtemp;
	std::stringstream stream;

	file_h32 << "#< h32 file for bootloader >#" << endl;
	file_h32 << "#< Date of Export:" << str_now << ">#" << endl;

	while (i_vect_ptr < hexsour_size)
	{
		i_curr_addr = hex_source.data_addr[i_vect_ptr];
		i_pred_next_addr = i_curr_addr + 0x10;
		if (i_vect_ptr < hexsour_size)
			i_real_next_addr = hex_source.data_addr[i_vect_ptr + 1];
		else
			i_real_next_addr = -1;

		i_data_size = 0;
		str_pushdata.clear();
		if (i_pred_next_addr == i_real_next_addr)
		{
			i_data_size = hex_source.row_data[i_vect_ptr].size();
			i_data_size += hex_source.row_data[i_vect_ptr + 1].size();

			if (i_data_size == 32)
			{
				for (int i = 0; i < 2; i++)
				{
					for (int j = 0; j < 16; j++)
					{
						stream.str("");
						stream.clear();
						stream << std::hex << std::setw(2) << setfill('0') << hex_source.row_data[i_vect_ptr + i][j];
						stream >> str_pushtemp;
						str_pushdata += str_pushtemp;
					}
				}
			}
			else
			{
				if (i_data_size <= 16)
				{
					for (int j = 0; j < 16; j++)
					{
						if (j < i_data_size)
						{
							stream.str("");
							stream.clear();
							stream << std::hex << std::setw(2) << setfill('0') << hex_source.row_data[i_vect_ptr][j];
							stream >> str_pushtemp;
							str_pushdata += str_pushtemp;
						}
						else
						{
							stream.str("");
							stream.clear();

							if ((j + 1) % 4 == 0)
								stream << std::hex << std::setw(2) << setfill('0') << 0x00;
							else
								stream << std::hex << std::setw(2) << setfill('0') << 0xff;

							stream >> str_pushtemp;
							str_pushdata += str_pushtemp;
						}
					}
				}
				else
				{
					for (int j = 0; j < 16; j++)
					{
						stream.str("");
						stream.clear();
						stream << std::hex << std::setw(2) << setfill('0') << hex_source.row_data[i_vect_ptr][j];
						stream >> str_pushtemp;
						str_pushdata += str_pushtemp;
					}

					i_data_size -= 16;
					// cout<<"data_not_32_but_addr_serialized"<<endl;
					for (int j = 0; j < 16; j++)
					{
						if (j < i_data_size)
						{
							stream.str("");
							stream.clear();
							stream << std::hex << std::setw(2) << setfill('0') << hex_source.row_data[i_vect_ptr + 1][j];
							stream >> str_pushtemp;
							str_pushdata += str_pushtemp;
						}
						else
						{
							stream.str("");
							stream.clear();

							if ((j + 1) % 4 == 0)
								stream << std::hex << std::setw(2) << setfill('0') << 0x00;
							else
								stream << std::hex << std::setw(2) << setfill('0') << 0xff;

							stream >> str_pushtemp;
							str_pushdata += str_pushtemp;
						}
					}
				}
			}
			i_vect_ptr += 2;
		}
		else
		{
			i_data_size = hex_source.row_data[i_vect_ptr].size();

			for (int j = 0; j < 16; j++)
			{
				if (j < i_data_size)
				{
					stream.str("");
					stream.clear();
					stream << std::hex << std::setw(2) << setfill('0') << hex_source.row_data[i_vect_ptr][j];
					stream >> str_pushtemp;
					str_pushdata += str_pushtemp;
				}
				else
				{
					stream.str("");
					stream.clear();

					if ((j + 1) % 4 == 0)
						stream << std::hex << std::setw(2) << setfill('0') << 0x00;
					else
						stream << std::hex << std::setw(2) << setfill('0') << 0xff;

					stream >> str_pushtemp;
					str_pushdata += str_pushtemp;
				}
			}

			for (int j = 0; j < 16; j++)
			{
				stream.str("");
				stream.clear();

				if ((j + 1) % 4 == 0)
					stream << std::hex << std::setw(2) << setfill('0') << 0x00;
				else
					stream << std::hex << std::setw(2) << setfill('0') << 0xff;

				stream >> str_pushtemp;
				str_pushdata += str_pushtemp;
			}
			i_vect_ptr += 1;
		}
		h32_row_chksum = hex32_chksum_gen(str_pushdata);

		file_h32 << ":" << std::hex << std::setw(7) << setfill('0') << i_curr_addr;
		file_h32 << ":" << str_pushdata;
		file_h32 << ":" << std::hex << std::setw(2) << setfill('0') << (int)h32_row_chksum << endl;
	}

	file_h32.close();
	return 0;
}

int bl_ReadHex32(char *file_name_h32)
{
	int res = 0;
	int b_filevalid;
	int i_linenum = 0;

	int colon_res = 0;
	int colon_pos1 = 8;
	int colon_pos2 = 73;

	int rowdata_chksum_res = 0;
	int i_rowchksum;
	unsigned char cal_rowdata_chksum;
	int PIC_GenADDR_MAX = 0x55800 * 2;
	//  save data.
	vector<int> row_bytes;
	string str_onebyte;
	int i_onebyte;

	ifstream source_file;
	string file_data;
	string str_startcode;
	string::size_type position;

	string str_rowdata;
	string str_rowchksum;

	string str_row_addr;
	int i_row_addr;

	//	string  str_rowchksum;

	b_filevalid = access(file_name_h32, F_OK);
	if (b_filevalid == 0)
		res = 0;
	else if (b_filevalid == -1)
	{
		std::string log = file_name_h32;
		log = "'" + log + "'" + " file is not exist.";
		goDriverLogger.Log("warning", log);
		res = -1;
	}

	if (res < 0)
		return res;

	//  1. check startcode ":"
	//  2. check bytecount > 16
	//  3. check record type: 00, 01, 02, 03, 04, 05.
	//  4. check row checksum match.
	source_file.open(string(file_name_h32).c_str(), std::ifstream::in);
	while (std::getline(source_file, file_data)) // read line by line.
	{
		str_startcode.clear();
		str_startcode = str_startcode.assign(file_data, 0, 1); // start 0, size:1.
		// cout<<"str_startcode   "<<str_startcode<<endl;
		if (str_startcode != ":" && str_startcode != "#")
		{
			goDriverLogger.Log("warning", "file format, start code fault. line:" + std::to_string(i_linenum + 1));
			res = -2;
			break;
		}
		i_linenum++;
	}
	source_file.close();
	cout << "h32-startcode check finished." << endl;

	i_linenum = 0;
	source_file.open(string(file_name_h32).c_str(), std::ifstream::in);
	while (std::getline(source_file, file_data)) // read line by line.
	{
		str_startcode.clear();
		str_startcode = str_startcode.assign(file_data, 0, 1); // start 0, size:1.
		position = 0;
		if (str_startcode == ":")
		{
			// cout<<file_data<<endl;
			while (1)
			{
				position = file_data.find(":", position + 1);

				if (position == file_data.npos)
					break;
				else if (position != colon_pos1 && position != colon_pos2)
				{
					cout << "pos:" << position << endl;
					colon_res = -1;
				}
			}

			if (colon_res == -1)
			{
				goDriverLogger.Log("warning", "file format, colon position fault. line:" + std::to_string(i_linenum + 1));
				res = -3;
				break;
			}
		}
		i_linenum++;
	}
	source_file.close();
	cout << "h32-colon pos check finished." << endl;

	i_linenum = 0;
	source_file.open(string(file_name_h32).c_str(), std::ifstream::in);
	while (std::getline(source_file, file_data)) // read line by line.
	{
		str_startcode.clear();
		str_startcode = str_startcode.assign(file_data, 0, 1); // start 0, size:1.
		if (str_startcode == ":")
		{ // cout<<"str_size:"<<file_data.size()<<endl;

			if (file_data.size() != 76)
			{
				res = -5;
				goDriverLogger.Log("warning", "file format, row checksum-size fault. line:" + std::to_string(i_linenum + 1));
				break;
			}

			str_rowdata = str_rowdata.assign(file_data, colon_pos1 + 1, 64);
			str_rowchksum = str_rowchksum.assign(file_data, colon_pos2 + 1, 2);
			i_rowchksum = HexStringToInt(str_rowchksum);
			cal_rowdata_chksum = hex32_chksum_gen(str_rowdata);
			//			cout<<str_rowdata<<","<<str_rowchksum<<"----";
			//			printf("chksumCal: 0x%02x\n",str_rowdata_chksum );
			if (cal_rowdata_chksum != (unsigned char)i_rowchksum)
				rowdata_chksum_res = -1;
		}

		if (rowdata_chksum_res == -1)
		{
			goDriverLogger.Log("warning", "file format, row checksum fault. line:" + std::to_string(i_linenum + 1));
			res = -4;
			break;
		}
		i_linenum++;
	}
	source_file.close();
	cout << "h32-rowchksum check finished." << endl;

	if (res < 0)
		return res;

	//-----------------------------------------------------------------------------
	i_linenum = 0;
	h32_source.data_addr.clear();
	h32_source.row_data.clear();
	h32_source.row_chksum.clear();
	h32_source.gen_seg_size = 0;
	source_file.open(string(file_name_h32).c_str(), std::ifstream::in);
	while (std::getline(source_file, file_data)) // read line by line.
	{
		str_startcode.clear();
		str_startcode = str_startcode.assign(file_data, 0, 1); // start 0, size:1.
		if (str_startcode == ":")
		{
			str_row_addr = str_row_addr.assign(file_data, 1, 7);
			i_row_addr = HexStringToInt(str_row_addr);
			//			printf("addr: 0x%07x\n",i_row_addr );
			h32_source.data_addr.push_back(i_row_addr);

			row_bytes.clear();
			str_rowdata = str_rowdata.assign(file_data, colon_pos1 + 1, 64);
			//			cout<<str_rowdata<<endl;
			for (int i = 0; i < 32; i++)
			{
				str_onebyte = str_onebyte.assign(str_rowdata, i * 2, 2);
				i_onebyte = HexStringToInt(str_onebyte);
				row_bytes.push_back(i_onebyte);
			}
			h32_source.row_data.push_back(row_bytes);

			str_rowchksum = str_rowchksum.assign(file_data, colon_pos2 + 1, 2);
			i_rowchksum = HexStringToInt(str_rowchksum);
			h32_source.row_chksum.push_back((unsigned char)i_rowchksum);

			if (i_row_addr <= PIC_GenADDR_MAX)
				h32_source.gen_seg_size++;
		}
		i_linenum++;
	}
	source_file.close();

	cout << "h32-file load check ok." << endl;
	cout << "h32_sour_addr_size:" << std::dec << h32_source.data_addr.size() << endl;
	// cout<<"h32_sour_data_size:"<<std::dec<<h32_source.row_data.size()<<endl;
	cout << "gen_seg_size:" << std::dec << h32_source.gen_seg_size << endl;

	return res;
}

int bl_Read_GenSegChksum_Hex32(unsigned short *chksum)
{
	int res = 0;
	if (h32_source.data_addr.empty())
	{
		goDriverLogger.Log("warning", "inner h32_source is empty");
		res = -1;
		return res;
	}
	if (h32_source.row_data.empty())
	{
		goDriverLogger.Log("warning", "inner h32_source is empty");
		res = -1;
		return res;
	}

	int i_maxaddr_bk = 0;
	int i_vect_ptr = 0;
	int i_curr_addr;
	int i_pred_next_addr;
	int i_real_next_addr;
	int i_short_data_of_row; // 2bytes;
	int hexsour_size = h32_source.data_addr.size();
	int PIC_GenADDR_MAX = 0x55800 * 2;
	// int PIC_GenADDR_MAX = 0x1200 * 2;
	unsigned short sum = 0;

	while (i_vect_ptr < hexsour_size)
	{
		i_curr_addr = h32_source.data_addr[i_vect_ptr];
		if (i_curr_addr > PIC_GenADDR_MAX)
		{
			// cout<<"PIC_GenADDR_MAX reached."<<endl;
			break;
		}

		i_pred_next_addr = i_curr_addr + 0x20;
		if (i_vect_ptr < hexsour_size)
			i_real_next_addr = h32_source.data_addr[i_vect_ptr + 1];
		else
			i_real_next_addr = -1;

		if (i_pred_next_addr == i_real_next_addr)
		{
			// printf("i[0x%04X]: ",i_curr_addr);
			for (int i = 0; i < 32; i += 2)
			{
				i_short_data_of_row = h32_source.row_data[i_vect_ptr][i];
				i_short_data_of_row |= ((int)h32_source.row_data[i_vect_ptr][i + 1]) << 8;
				sum += i_short_data_of_row;
				// printf("%04X ",i_short_data_of_row);
			}
			// printf("\n");
		}
		else
		{ // printf("k[0x%04X]: ",i_curr_addr);
			for (int i = 0; i < 32; i += 2)
			{
				i_short_data_of_row = h32_source.row_data[i_vect_ptr][i];
				i_short_data_of_row |= ((int)h32_source.row_data[i_vect_ptr][i + 1]) << 8;
				sum += i_short_data_of_row;
				// printf("%04X ",i_short_data_of_row);
			}
			// printf("\n");

			for (int j = i_pred_next_addr; j < i_real_next_addr; j += 0x20)
			{
				// printf("J[0x%04X]: ",j);

				if (j >= PIC_GenADDR_MAX)
				{
					// cout<<"PIC_GenADDR_MAX reached000."<<endl;
					break;
				}

				for (int i = 0; i < 32; i += 2)
				{
					if (i % 4 == 0)
						i_short_data_of_row = 0xffff;
					else
						i_short_data_of_row = 0x00ff;
					sum += i_short_data_of_row;
					// printf("%04X ",i_short_data_of_row);
				}
				// printf("\n");
			}
		}

		i_vect_ptr++;
	}

	*chksum = 0xffff - sum + 1;
	return 0;
}

int bl_Read_AuxSegChksum_Hex32(unsigned short *chksum)
{
	int res = 0;
	if (h32_source.data_addr.empty())
	{
		goDriverLogger.Log("warning", "inner h32_source is empty");
		res = -1;
		return res;
	}
	if (h32_source.row_data.empty())
	{
		goDriverLogger.Log("warning", "inner h32_source is empty");
		res = -1;
		return res;
	}

	int i_maxaddr_bk = 0;
	int i_vect_ptr = 0;
	int i_curr_addr;
	int i_pred_next_addr;
	int i_real_next_addr;
	int i_short_data_of_row; // 2bytes;
	int hexsour_size = h32_source.data_addr.size();
	int PIC_AuxADDR_FINAL = 0x7ffff0 * 2;
	int PIC_AuxADDR_MAX = 0x7ffff8 * 2;
	int PIC_AuxADDR_MIN = 0x7fc000 * 2 + 0x800 * 2;
	unsigned short sum = 0;

	while (i_vect_ptr < hexsour_size)
	{
		i_curr_addr = h32_source.data_addr[i_vect_ptr];
		if (i_curr_addr > PIC_AuxADDR_MAX)
			break;

		if (i_curr_addr < PIC_AuxADDR_MIN)
		{
			i_vect_ptr++;
			continue;
		}

		i_pred_next_addr = i_curr_addr + 0x20;
		if (i_vect_ptr < hexsour_size)
			i_real_next_addr = h32_source.data_addr[i_vect_ptr + 1];
		else
			i_real_next_addr = -1;

		if (i_pred_next_addr == i_real_next_addr)
		{
			for (int i = 0; i < 32; i += 2)
			{
				i_short_data_of_row = h32_source.row_data[i_vect_ptr][i];
				i_short_data_of_row |= ((int)h32_source.row_data[i_vect_ptr][i + 1]) << 8;
				sum += i_short_data_of_row;
			}
		}
		else
		{

			if (i_curr_addr < PIC_AuxADDR_FINAL)
			{ // printf("k[0x%04X]: ",i_curr_addr);
				for (int i = 0; i < 32; i += 2)
				{
					i_short_data_of_row = h32_source.row_data[i_vect_ptr][i];
					i_short_data_of_row |= ((int)h32_source.row_data[i_vect_ptr][i + 1]) << 8;
					sum += i_short_data_of_row;
					// printf("%04X ",i_short_data_of_row);
				}
			}
			else
			{ // printf("m[0x%04X]: ",i_curr_addr);
				for (int i = 0; i < 16; i += 2)
				{
					i_short_data_of_row = h32_source.row_data[i_vect_ptr][i];
					i_short_data_of_row |= ((int)h32_source.row_data[i_vect_ptr][i + 1]) << 8;
					sum += i_short_data_of_row;
					// printf("%04X ",i_short_data_of_row);
				}
			}
			for (int j = i_pred_next_addr; j < i_real_next_addr; j += 0x20)
			{
				if (j >= PIC_AuxADDR_MAX)
					break;
				for (int i = 0; i < 32; i += 2)
				{
					if (i % 4 == 0)
						i_short_data_of_row = 0xffff;
					else
						i_short_data_of_row = 0x00ff;
					sum += i_short_data_of_row;
				}
			}
		}
		i_vect_ptr++;
	}

	*chksum = 0xffff - sum + 1;
	return 0;
}

int bl_Wr_Sect_by_H32(int SST_sect)
{
	if (h32_source.data_addr.empty())
	{
		cout << "inner h32_source is empty." << endl;
		return -1;
	}
	if (SST_sect < 0 || SST_sect > 10)
	{
		cout << "sect out of range." << endl;
		return -2;
	}
	int res;
	//	cout<<"test ok"<<endl;
	//  1. bl initial.
	//  2. bl sect wrbp-0.
	//  3. bl sect block erase.

	res = bl_init();
	if (res == -1)
	{
		goDriverLogger.Log("error", "bl init fault");
		return res;
	}

	res = bl_sstWriteBP_section(SST_sect, 0);
	if (res == -1)
	{
		goDriverLogger.Log("error", "bl WriteBP-clear fault");
		return res;
	}

	res = bl_block_erase_section(SST_sect);
	if (res == -1)
	{
		goDriverLogger.Log("error", "bl block-erase fault");
		return res;
	}

	wr_sect_by_h32.SST_sect = SST_sect;
	wr_sect_by_h32.pthread_stop = 0;
	pthread_create(&rdwr_h32pthread, NULL, writeSSTsect_Hex32, NULL);
	return 0;
}

void *writeSSTsect_Hex32(void *ptr)
{
	Poco::Event wait_50ms;
	// int wr_start_addr,wr_sst32_addr;
	// int SST26V_sectbase = 0x10000;
	int PIC_GenSegAddr_MAX = 0x55800;
	int h32_sour_addr_size = h32_source.data_addr.size();
	// int h32_genseg_max     = h32_source.gen_seg_size;
	int i = 0;
	int j = 0;
	int em_stop = 0;
	int h32_row_addr;
	int tx_h32_addr;
	unsigned char h32_row_chksum;
	vector<unsigned char> h32_row_data;

	unsigned char chksum_fbk, chksumH_fbk;
	int tx_res;

	sst_addr_t WrSST_Addr;
	WrSST_Addr.bytes.LLB = 0;
	WrSST_Addr.bytes.LHB = 0;
	WrSST_Addr.bytes.HLB = SST_SectBase[wr_sect_by_h32.SST_sect];
	WrSST_Addr.bytes.HHB = 0;

	// wr_start_addr = (( wr_sect_by_h32.SST_sect*11 )<<16 )&0x7fffff;
	// wr_start_addr+= SST26V_sectbase;
	cout << "sst26_SECT: " << std::dec << wr_sect_by_h32.SST_sect << endl;
	cout << "start_addr: 0x" << std::hex << std::setw(7) << setfill('0') << WrSST_Addr.int_addr << endl;
	cout << "h32_sour_addr_size:" << std::dec << h32_sour_addr_size << endl;

	timetag_t t_bl_wr_SST32;
	time_tag(tag_all, &t_bl_wr_SST32);

	downloading_h32_status.i_curr_msg.clear();
	downloading_h32_status.i_tx_res.clear();
	downloading_h32_status.i_total_msg = h32_source.gen_seg_size;
	;
	poco_periodictimer(100);

	while (i < h32_sour_addr_size)
	{
		if (wr_sect_by_h32.pthread_stop == 1)
		{
			em_stop = 1;
			break;
		}

		h32_row_addr = h32_source.data_addr[i];
		h32_row_chksum = h32_source.row_chksum[i];
		h32_row_data.clear();
		for (j = 0; j < 32; j++)
			h32_row_data.push_back((unsigned char)h32_source.row_data[i][j]);

		if (h32_row_addr > PIC_GenSegAddr_MAX * 2)
		{
			cout << "GenSegmentAddr load completed." << endl;
			break;
		}

		tx_h32_addr = h32_row_addr + WrSST_Addr.int_addr;
		tx_res = bl_write_sstMEM32(tx_h32_addr, h32_row_data, &chksum_fbk, &chksumH_fbk);

		poco_mutex.lock();
		downloading_h32_status.i_curr_msg.push_back(i);
		downloading_h32_status.i_tx_res.push_back(h32_sour_addr_size);
		poco_mutex.unlock();

		//		cout<<"addr:0x"<<std::hex<<std::setw(7)<<setfill('0')<<h32_row_addr;
		//		cout<<"  Tx_chk:0x"<<std::hex<<std::setw(2)<<(int)h32_row_chksum;
		//		cout<<"  Rx_chk:0x"<<std::hex<<std::setw(2)<<(int)chksum_fbk<<endl;
		if (tx_res == -1)
		{
			cout << "Tx time-out." << endl;
			break;
		}
		// wait_50ms.tryWait(50);
		// cout<<"wait_50ms"<<endl;
		i++;
	}

	if (em_stop == 0 && tx_res == 0)
	{
		bl_write_SectInfo_n(wr_sect_by_h32.SST_sect, "clr");
		wait_50ms.tryWait(10);
		bl_write_SectInfo_n(wr_sect_by_h32.SST_sect, "h32");
		wait_50ms.tryWait(10);
		bl_write_SectBoot(wr_sect_by_h32.SST_sect);
	}
	time_tag(tag_delta, &t_bl_wr_SST32);
	printf("Write SST32_sect, execute time:%dms. \n", t_bl_wr_SST32.delta_tn);

	std::string str_end;
	if (em_stop == 1)
	{
		cout << "Em_Stop: row data send not completed." << endl;
		str_end = "Tx_EmergencyStop";
	}
	if (tx_res < 0)
	{
		cout << "RTC_timeout: row data send not completed." << endl;
		str_end = "Tx_TimeoutStop";
	}
	if (em_stop == 0 && tx_res == 0)
	{
		cout << "Tx_Completed." << endl;
		str_end = "Tx_Completed";
	}

	wait_50ms.tryWait(200);
	msg_driv_to_ui_tx_h32_end(str_end);
	poco_periodictimerStop();

	/*
	while(1)
	{
		if( downloading_h32_status.i_send_ui < (downloading_h32_status.i_total_msg - 1) )
		{
			wait_50ms.tryWait(50);
			continue;
		}
		else
		{
			printf("ui_num:%d,num_msg:%d,den_msg:%d\n", downloading_h32_status.i_send_ui,downloading_h32_status.i_curr_msg.back(),downloading_h32_status.i_total_msg);
			cout<<"TimerStop"<<endl;
			poco_periodictimerStop();

			msg_driv_to_ui_tx_h32_end( str_end );
			break;
		}
	}
*/
	cout << "__End_SST32_H32___" << endl;
	return 0;
}

void bl_txH32_Stop(void)
{
	printf(" Tx_SST32_H32 stop :::");
	wr_sect_by_h32.pthread_stop = 1;
}

int bl_Wr_Sect_by_H16(int SST_sect)
{
	if (h16_source.data_addr.empty())
	{
		cout << "inner h16_source is empty." << endl;
		return -1;
	}
	if (SST_sect < 0 || SST_sect > 10)
	{
		cout << "sect out of range." << endl;
		return -2;
	}
	int res;
	//	cout<<"test ok"<<endl;
	//  1. bl initial.
	//  2. bl sect wrbp-0.
	//  3. bl sect block erase.

	res = bl_init();
	if (res == -1)
	{
		goDriverLogger.Log("error", "bl init fault");
		return res;
	}

	res = bl_sstWriteBP_section(SST_sect, 0);
	if (res == -1)
	{
		goDriverLogger.Log("error", "bl WriteBP-clear fault");
		return res;
	}

	res = bl_block_erase_section(SST_sect);
	if (res == -1)
	{
		goDriverLogger.Log("error", "bl block-erase fault");
		return res;
	}
	wr_sect_by_h16.SST_sect = SST_sect;
	wr_sect_by_h16.pthread_stop = 0;
	pthread_create(&rdwr_h32pthread, NULL, writeSSTsect_Hex16, NULL);
	return 0;
}

void *writeSSTsect_Hex16(void *ptr)
{
	Poco::Event wait_50ms;
	int wr_start_addr, wr_sst16_addr;
	int SST26V_sectbase = 0x10000;
	int PIC_GenSegAddr_MAX = 0x55800;
	int h16_sour_addr_size = h16_source.data_addr.size();
	int i = 0;
	int j = 0;
	int em_stop = 0;
	int h16_row_addr;
	int tx_h16_addr;
	unsigned char h16_row_chksum;
	vector<unsigned char> h16_row_data;

	unsigned char chksum_fbk, chksumH_fbk;
	int tx_res;

	wr_start_addr = ((wr_sect_by_h16.SST_sect * 11) << 16) & 0x7fffff;
	wr_start_addr += SST26V_sectbase;
	cout << "sst26_SECT: " << std::dec << wr_sect_by_h16.SST_sect << endl;
	cout << "start_addr: 0x" << std::hex << std::setw(7) << setfill('0') << wr_start_addr << endl;
	cout << "h16_sour_addr_size:" << std::dec << h16_sour_addr_size << endl;

	timetag_t t_bl_wr_SST16;
	time_tag(tag_all, &t_bl_wr_SST16);

	while (i < h16_sour_addr_size)
	{
		if (wr_sect_by_h16.pthread_stop == 1)
		{
			em_stop = 1;
			break;
		}

		h16_row_addr = h16_source.data_addr[i];
		h16_row_chksum = h16_source.row_chksum[i];
		h16_row_data.clear();
		for (j = 0; j < 16; j++)
			h16_row_data.push_back((unsigned char)h16_source.row_data[i][j]);

		if (h16_row_addr > PIC_GenSegAddr_MAX * 2)
		{
			cout << "GenSegmentAddr load completed." << endl;
			break;
		}

		tx_h16_addr = h16_row_addr + wr_start_addr;
		tx_res = bl_write_sstMEM16(tx_h16_addr, h16_row_data, &chksum_fbk, &chksumH_fbk);

		cout << "addr:0x" << std::hex << std::setw(7) << setfill('0') << h16_row_addr;
		cout << "  Tx_chk:0x" << std::hex << std::setw(2) << (int)h16_row_chksum;
		cout << "  Rx_chk:0x" << std::hex << std::setw(2) << (int)chksum_fbk << endl;
		if (tx_res == -1)
		{
			cout << "Tx time-out." << endl;
			break;
		}
		i++;
	}

	time_tag(tag_delta, &t_bl_wr_SST16);
	printf("Write SST16_sect, execute time:%dms. \n", t_bl_wr_SST16.delta_tn);
	if (em_stop == 1)
		cout << "em_Stop: row data send not completed." << endl;
	cout << "end_pthread" << endl;
	return 0;
}

int bl_ExportHex16(char *new_name_h16)
{
	// res = -1: hex source is not exist.
	// res = -2: new_name_h32 size "0"
	// res = -3: file format fault. bytecount > 16.
	// res = -4: file format fault. record type > 5.
	int res = 0;
	if (hex_source.data_addr.empty())
	{
		goDriverLogger.Log("warning", "hex source.address are empty, load hex source file first");
		res = -1;
		return res;
	}
	if (hex_source.row_data.empty())
	{
		goDriverLogger.Log("warning", "hex source.row_data are empty, load hex source file first");
		res = -1;
		return res;
	}
	// create a hex file for 32bytes.
	string file_ext = ".h16";
	string str_now = get_tnow();
	string newfile_name = new_name_h16;
	cout << "str_size:" << newfile_name.size() << endl;
	if (newfile_name.size() == 0)
	{
		goDriverLogger.Log("warning", "input file_name size = 0");
		res = -2;
		return res;
	}

	string::size_type dot_pos;
	dot_pos = newfile_name.find(".");
	if (dot_pos != newfile_name.npos)
	{
		cout << "dot_pos:" << dot_pos << endl;
		if (dot_pos == 0)
		{
			goDriverLogger.Log("warning", "input file_name size = 0, dot in fornt.");
			res = -3;
			return res;
		}
		newfile_name.erase(newfile_name.begin() + dot_pos, newfile_name.end());
	}
	newfile_name = newfile_name.append(file_ext);
	cout << "newfile_name: " << newfile_name << endl;

	ofstream fileout(newfile_name.c_str(), ios::out | ios::trunc);
	fileout.close();
	chmod(newfile_name.c_str(), 0777);
	chown(newfile_name.c_str(), 1000, 1000);

	fstream file_h16;
	file_h16.open(newfile_name.c_str(), ios::app);

	int hexsour_size = hex_source.data_addr.size();
	int i_vect_ptr = 0;
	int i_curr_addr;
	int i_pred_next_addr;
	int i_real_next_addr;
	int i_data_size;
	unsigned char h16_row_chksum;

	string str_pushdata;
	string str_pushtemp;
	std::stringstream stream;

	file_h16 << "#< h16 file for bootloader >#" << endl;
	file_h16 << "#< Date of Export:" << str_now << ">#" << endl;

	while (i_vect_ptr < hexsour_size)
	{
		i_curr_addr = hex_source.data_addr[i_vect_ptr];
		i_data_size = hex_source.row_data[i_vect_ptr].size();
		str_pushdata.clear();
		for (int j = 0; j < 16; j++)
		{
			if (j < i_data_size)
			{
				stream.str("");
				stream.clear();
				stream << std::hex << std::setw(2) << setfill('0') << hex_source.row_data[i_vect_ptr][j];
				stream >> str_pushtemp;
				str_pushdata += str_pushtemp;
			}
			else
			{
				stream.str("");
				stream.clear();
				if ((j + 1) % 4 == 0)
					stream << std::hex << std::setw(2) << setfill('0') << 0x00;
				else
					stream << std::hex << std::setw(2) << setfill('0') << 0xff;
				stream >> str_pushtemp;
				str_pushdata += str_pushtemp;
			}
		}
		i_vect_ptr++;
		h16_row_chksum = hex16_chksum_gen(str_pushdata);
		file_h16 << ":" << std::hex << std::setw(7) << setfill('0') << i_curr_addr;
		file_h16 << ":" << str_pushdata;
		file_h16 << ":" << std::hex << std::setw(2) << setfill('0') << (int)h16_row_chksum << endl;
	}

	file_h16.close();
	return 0;
}

int bl_ReadHex16(char *file_name_h16)
{
	int res = 0;
	int b_filevalid;
	int i_linenum = 0;

	int colon_res = 0;
	int colon_pos1 = 8;
	int colon_pos2 = 41;

	int rowdata_chksum_res = 0;
	int i_rowchksum;
	unsigned char cal_rowdata_chksum;

	//  save data.
	vector<int> row_bytes;
	string str_onebyte;
	int i_onebyte;

	ifstream source_file;
	string file_data;
	string str_startcode;
	string::size_type position;

	string str_rowdata;
	string str_rowchksum;

	string str_row_addr;
	int i_row_addr;
	b_filevalid = access(file_name_h16, F_OK);
	if (b_filevalid == 0)
		res = 0;
	else if (b_filevalid == -1)
	{
		std::string log = file_name_h16;
		log = "'" + log + "'" + " file is not exist.";
		goDriverLogger.Log("warning", log);
		res = -1;
	}

	if (res < 0)
		return res;
	//  1. check startcode ":"
	//  2. check bytecount > 16
	//  3. check record type: 00, 01, 02, 03, 04, 05.
	//  4. check row checksum match.
	source_file.open(string(file_name_h16).c_str(), std::ifstream::in);
	while (std::getline(source_file, file_data)) // read line by line.
	{
		str_startcode.clear();
		str_startcode = str_startcode.assign(file_data, 0, 1); // start 0, size:1.
		if (str_startcode != ":" && str_startcode != "#")
		{
			goDriverLogger.Log("warning", "file format, start code fault. line:" + std::to_string(i_linenum + 1));
			res = -2;
			break;
		}
		i_linenum++;
	}
	source_file.close();
	cout << "h16-startcode check finished." << endl;

	i_linenum = 0;
	source_file.open(string(file_name_h16).c_str(), std::ifstream::in);
	while (std::getline(source_file, file_data)) // read line by line.
	{
		str_startcode.clear();
		str_startcode = str_startcode.assign(file_data, 0, 1); // start 0, size:1.
		position = 0;
		if (str_startcode == ":")
		{
			while (1)
			{
				position = file_data.find(":", position + 1);
				if (position == file_data.npos)
					break;
				else if (position != colon_pos1 && position != colon_pos2)
				{
					cout << "pos:" << position << endl;
					colon_res = -1;
				}
			}
			if (colon_res == -1)
			{
				goDriverLogger.Log("warning", "file format, colon position fault. line:" + std::to_string(i_linenum + 1));
				res = -3;
				break;
			}
		}
		i_linenum++;
	}
	source_file.close();
	cout << "h16-colon pos check finished." << endl;

	i_linenum = 0;
	source_file.open(string(file_name_h16).c_str(), std::ifstream::in);
	while (std::getline(source_file, file_data)) // read line by line.
	{
		str_startcode.clear();
		str_startcode = str_startcode.assign(file_data, 0, 1); // start 0, size:1.
		if (str_startcode == ":")
		{
			str_rowdata = str_rowdata.assign(file_data, colon_pos1 + 1, 32);
			str_rowchksum = str_rowchksum.assign(file_data, colon_pos2 + 1, 2);
			i_rowchksum = HexStringToInt(str_rowchksum);
			cal_rowdata_chksum = hex16_chksum_gen(str_rowdata);
			if (cal_rowdata_chksum != (unsigned char)i_rowchksum)
				rowdata_chksum_res = -1;
		}

		if (rowdata_chksum_res == -1)
		{
			goDriverLogger.Log("warning", "file format, row checksum fault. line:" + std::to_string(i_linenum + 1));
			res = -4;
			break;
		}
		i_linenum++;
	}
	source_file.close();
	cout << "h16-rowchksum check finished." << endl;

	if (res < 0)
		return res;

	//-----------------------------------------------------------------------------
	i_linenum = 0;
	h16_source.data_addr.clear();
	h16_source.row_data.clear();
	h16_source.row_chksum.clear();
	source_file.open(string(file_name_h16).c_str(), std::ifstream::in);
	while (std::getline(source_file, file_data)) // read line by line.
	{
		str_startcode.clear();
		str_startcode = str_startcode.assign(file_data, 0, 1); // start 0, size:1.
		if (str_startcode == ":")
		{
			str_row_addr = str_row_addr.assign(file_data, 1, 7);
			i_row_addr = HexStringToInt(str_row_addr);
			h16_source.data_addr.push_back(i_row_addr);

			row_bytes.clear();
			str_rowdata = str_rowdata.assign(file_data, colon_pos1 + 1, 32);
			for (int i = 0; i < 16; i++)
			{
				str_onebyte = str_onebyte.assign(str_rowdata, i * 2, 2);
				i_onebyte = HexStringToInt(str_onebyte);
				row_bytes.push_back(i_onebyte);
			}
			h16_source.row_data.push_back(row_bytes);
			str_rowchksum = str_rowchksum.assign(file_data, colon_pos2 + 1, 2);
			i_rowchksum = HexStringToInt(str_rowchksum);
			h16_source.row_chksum.push_back((unsigned char)i_rowchksum);
		}
		i_linenum++;
	}
	source_file.close();

	cout << "h16-file load check ok." << endl;
	cout << "h16_sour_addr_size:" << std::dec << h16_source.data_addr.size() << endl;
	cout << "h16_sour_data_size:" << std::dec << h16_source.row_data.size() << endl;
	return res;
}

int bl_Read_GenSegChksum_Hex16(unsigned short *chksum)
{
	int res = 0;
	if (h16_source.data_addr.empty())
	{
		goDriverLogger.Log("warning", "inner h16_source is empty");
		res = -1;
		return res;
	}
	if (h16_source.row_data.empty())
	{
		goDriverLogger.Log("warning", "inner h16_source is empty");
		res = -1;
		return res;
	}

	int i_maxaddr_bk = 0;
	int i_vect_ptr = 0;
	int i_curr_addr;
	int i_pred_next_addr;
	int i_real_next_addr;
	int i_short_data_of_row; // 2bytes;
	int hexsour_size = h16_source.data_addr.size();
	int PIC_GenADDR_MAX = 0x55800 * 2;
	unsigned short sum = 0;

	while (i_vect_ptr < hexsour_size)
	{
		i_curr_addr = h16_source.data_addr[i_vect_ptr];
		if (i_curr_addr > PIC_GenADDR_MAX)
			break;

		i_pred_next_addr = i_curr_addr + 0x10;
		if (i_vect_ptr < hexsour_size)
			i_real_next_addr = h16_source.data_addr[i_vect_ptr + 1];
		else
			i_real_next_addr = -1;

		if (i_pred_next_addr == i_real_next_addr)
		{
			for (int i = 0; i < 16; i += 2)
			{
				i_short_data_of_row = h16_source.row_data[i_vect_ptr][i];
				i_short_data_of_row |= ((int)h16_source.row_data[i_vect_ptr][i + 1]) << 8;
				sum += i_short_data_of_row;
			}
		}
		else
		{
			for (int i = 0; i < 16; i += 2)
			{
				i_short_data_of_row = h16_source.row_data[i_vect_ptr][i];
				i_short_data_of_row |= ((int)h16_source.row_data[i_vect_ptr][i + 1]) << 8;
				sum += i_short_data_of_row;
			}

			for (int j = i_pred_next_addr; j < i_real_next_addr; j += 0x10)
			{
				if (j >= PIC_GenADDR_MAX)
					break;

				for (int i = 0; i < 16; i += 2)
				{
					if (i % 4 == 0)
						i_short_data_of_row = 0xffff;
					else
						i_short_data_of_row = 0x00ff;
					sum += i_short_data_of_row;
				}
			}
		}
		i_vect_ptr++;
	}

	*chksum = 0xffff - sum + 1;
	return 0;
}
int bl_Read_AuxSegChksum_Hex16(unsigned short *chksum)
{
	int res = 0;
	if (h16_source.data_addr.empty())
	{
		goDriverLogger.Log("warning", "inner h16_source is empty");
		res = -1;
		return res;
	}
	if (h16_source.row_data.empty())
	{
		goDriverLogger.Log("warning", "inner h16_source is empty");
		res = -1;
		return res;
	}

	int i_maxaddr_bk = 0;
	int i_vect_ptr = 0;
	int i_curr_addr;
	int i_pred_next_addr;
	int i_real_next_addr;
	int i_short_data_of_row; // 2bytes;
	int hexsour_size = h16_source.data_addr.size();
	int PIC_AuxADDR_FINAL = 0x7ffff0 * 2;
	int PIC_AuxADDR_MAX = 0x7ffff8 * 2;
	int PIC_AuxADDR_MIN = 0x7fc000 * 2 + 0x800 * 2;
	unsigned short sum = 0;

	while (i_vect_ptr < hexsour_size)
	{
		i_curr_addr = h16_source.data_addr[i_vect_ptr];
		if (i_curr_addr > PIC_AuxADDR_MAX)
			break;

		if (i_curr_addr < PIC_AuxADDR_MIN)
		{
			i_vect_ptr++;
			continue;
		}

		i_pred_next_addr = i_curr_addr + 0x10;
		if (i_vect_ptr < hexsour_size)
			i_real_next_addr = h16_source.data_addr[i_vect_ptr + 1];
		else
			i_real_next_addr = -1;

		if (i_pred_next_addr == i_real_next_addr)
		{
			for (int i = 0; i < 16; i += 2)
			{
				i_short_data_of_row = h16_source.row_data[i_vect_ptr][i];
				i_short_data_of_row |= ((int)h16_source.row_data[i_vect_ptr][i + 1]) << 8;
				sum += i_short_data_of_row;
			}
		}
		else
		{
			for (int i = 0; i < 16; i += 2)
			{
				i_short_data_of_row = h16_source.row_data[i_vect_ptr][i];
				i_short_data_of_row |= ((int)h16_source.row_data[i_vect_ptr][i + 1]) << 8;
				sum += i_short_data_of_row;
			}

			for (int j = i_pred_next_addr; j < i_real_next_addr; j += 0x10)
			{
				if (j >= PIC_AuxADDR_MAX)
					break;

				for (int i = 0; i < 16; i += 2)
				{
					if (i % 4 == 0)
						i_short_data_of_row = 0xffff;
					else
						i_short_data_of_row = 0x00ff;
					sum += i_short_data_of_row;
				}
			}
		}
		i_vect_ptr++;
	}

	*chksum = 0xffff - sum + 1;
	return 0;
}
//-----------------------------------------------------------------------------

// hex string to int.
unsigned int HexStringToInt(std::string &input)
{
	unsigned int ret;
	std::stringstream ss;
	ss << std::hex << input;
	ss >> ret;
	return ret;
}

string int2str(int &i)
{
	string s;
	stringstream ss(s);
	ss << i;
	return ss.str();
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
int bl_nop(void)
{
	int res;
	int nop_fbk;
	timetag_t t_bl_nop;
	time_tag(tag_all, &t_bl_nop);

	BL_RTC_CmdMsg.wait_cmdfbk.reset();
	BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd = _Null;
	Msg_Prot_BL_Host_to_RTC_encodeCmd(_NOP, 0, 0);

	nop_fbk = BL_RTC_CmdMsg.wait_cmdfbk.tryWait(50);
	if (nop_fbk == 1)
	{
		if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd == _NOP)
			res = 0;
		else
			res = -1;
	}
	else
		res = -1;
	time_tag(tag_delta, &t_bl_nop);
	printf("_NOP res:%d, execute time:%dms. \n", res, t_bl_nop.delta_tn);
	return res;
}

int bl_init(void)
{
	int res;
	int init_fbk;
	timetag_t t_bl_init;
	time_tag(tag_all, &t_bl_init);

	BL_RTC_CmdMsg.wait_cmdfbk.reset();
	BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd = _Null;
	Msg_Prot_BL_Host_to_RTC_encodeCmd(_BL_INIT, 0, 0);

	init_fbk = BL_RTC_CmdMsg.wait_cmdfbk.tryWait(50);
	if (init_fbk == 1)
	{
		if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd == _BL_INIT)
			res = 0;
		else
			res = -1;
	}
	else
		res = -1;
	time_tag(tag_delta, &t_bl_init);
	printf("_BL_INIT res:%d, execute time:%dms. \n", res, t_bl_init.delta_tn);
	return res;
}

void bl_gotoAUX(void)
{
	int res;
	Msg_Prot_BL_Host_to_RTC_encodeCmd(_gotoAUX, 0, 0);
}

int bl_Jedec_ID_Read(Jedec_ID_t *id)
{
	int res = 0;
	int sstid_fbk;
	Jedec_ID_t readID;
	timetag_t t_bl_rdsstid;
	time_tag(tag_all, &t_bl_rdsstid);

	BL_RTC_CmdMsg.wait_cmdfbk.reset();
	Msg_Prot_BL_Host_to_RTC_encodeCmd(_Jedec_ID_Read, 0, 0);

	sstid_fbk = BL_RTC_CmdMsg.wait_cmdfbk.tryWait(50);
	if (sstid_fbk == 1)
	{
		if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd == _Jedec_ID_Read)
		{
			res = 0;
			readID.manufa_id = BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[0];
			readID.device_typ = BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[1];
			readID.device_id = BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[2];
		}
		else
		{
			res = -1;
			readID.manufa_id = 0xff;
			readID.device_typ = 0xff;
			readID.device_id = 0xff;
		}
	}
	else
	{
		res = -1;
		readID.manufa_id = 0xff;
		readID.device_typ = 0xff;
		readID.device_id = 0xff;
	}

	if (res == -1)
		goDriverLogger.Log("warning", "read-sstid time-out");

	*id = readID;
	time_tag(tag_delta, &t_bl_rdsstid);
	printf("Read SSTID, res:%d, execute time:%dms. \n", res, t_bl_rdsstid.delta_tn);

	return res;
}

int bl_sstWriteBP(vector<unsigned char> bp_reg)
{
	int res = 0;
	int wrbp_fbk;
	timetag_t t_bl_wrsstbp;
	time_tag(tag_all, &t_bl_wrsstbp);
	unsigned char txreg[18];
	cout << "bpg_size: " << bp_reg.size() << endl;

	for (int i = 0; i < 18; i++)
	{
		if (i < bp_reg.size())
			txreg[i] = bp_reg[i];
		else
			txreg[i] = 0xff;
	}
	printf("txBPREG:");
	for (int i = 0; i < 18; i++)
		printf("0x%02X ", txreg[i]);
	cout << endl;

	BL_RTC_CmdMsg.wait_cmdfbk.reset();
	BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd = _Null;
	Msg_Prot_BL_Host_to_RTC_encodeCmd(_WriteBlockProtection, 0, txreg);

	wrbp_fbk = BL_RTC_CmdMsg.wait_cmdfbk.tryWait(50);
	if (wrbp_fbk == 1)
	{
		if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd == _WriteBlockProtection)
			res = 0;
		else
			res = -1;
	}
	else
		res = -1;

	if (res == -1)
		goDriverLogger.Log("warning", "write-sst_BPREG time-out");

	time_tag(tag_delta, &t_bl_wrsstbp);
	printf("Write SST_BPREG, res:%d, execute time:%dms. \n", res, t_bl_wrsstbp.delta_tn);

	return res;

	// 0: unlock.
	// 1: lock.
}

int bl_sstWriteBP_section(int sectN, bool mask)
{
	int res;
	//	unsigned char data_size = 2;
	//	unsigned char data[data_size];
	//	data[0] = _MaxSection;
	//	if( _config == 0 )
	//		data[1] = 0x00;
	//	else
	//		data[1] = 0x01;
	// Msg_Prot_BL_Host_to_RTC_encodeCmd( _WBP_11bit, 2, data );
	// 0: unlock.
	// 1: lock.
	unsigned char bp_reg[18];
	unsigned char config_bit;

	if (mask == 1)
		config_bit = 0xff;
	else
		config_bit = 0x00;

	res = bl_sstReadBP(bp_reg);

	if (res == 0)
	{
		switch (sectN)
		{
		case 0:
			bp_reg[17] = config_bit;
			bp_reg[16] &= 0xf8;
			bp_reg[16] |= (config_bit & 0x07);
			break;
		case 1:
			bp_reg[16] &= 0x07;
			bp_reg[16] |= (config_bit & 0xf8);

			bp_reg[15] &= 0xc0;
			bp_reg[15] |= (config_bit & 0x3f);
			break;
		case 2:
			bp_reg[15] &= 0x3f;
			bp_reg[15] |= (config_bit & 0xc0);

			bp_reg[14] = config_bit;

			bp_reg[13] &= 0xfe;
			bp_reg[13] |= (config_bit & 0x01);
			break;
		case 3:
			bp_reg[13] &= 0x01;
			bp_reg[13] |= (config_bit & 0xfe);
			bp_reg[12] &= 0xf0;
			bp_reg[12] |= (config_bit & 0x0f);
			break;
		case 4:
			bp_reg[12] &= 0x0f;
			bp_reg[12] |= (config_bit & 0xf0);
			bp_reg[11] &= 0x80;
			bp_reg[11] |= (config_bit & 0x7f);
			break;
		case 5:
			bp_reg[11] &= 0x7f;
			bp_reg[11] |= (config_bit & 0x80);

			bp_reg[10] = config_bit;

			bp_reg[9] &= 0xfc;
			bp_reg[9] |= (config_bit & 0x03);
			break;
		case 6:
			bp_reg[9] &= 0x03;
			bp_reg[9] |= (config_bit & 0xfc);
			bp_reg[8] &= 0xe0;
			bp_reg[8] |= (config_bit & 0x1f);
			break;
		case 7:
			bp_reg[8] &= 0x1f;
			bp_reg[8] |= (config_bit & 0xe0);
			bp_reg[7] = config_bit;
			break;
		case 8:
			bp_reg[6] = config_bit;
			bp_reg[5] &= 0xf8;
			bp_reg[5] |= (config_bit & 0x07);
			break;
		case 9:
			bp_reg[5] &= 0x07;
			bp_reg[5] |= (config_bit & 0xf8);
			bp_reg[4] &= 0xc0;
			bp_reg[4] |= (config_bit & 0x3f);
			break;
		case 10:
			bp_reg[4] &= 0x3f;
			bp_reg[4] |= (config_bit & 0xc0);
			bp_reg[3] = config_bit;
			bp_reg[2] &= 0xfe;
			bp_reg[2] |= (config_bit & 0x01);
			break;
		default:
			cout << "wrong number" << endl;
			res = -1;
			break;
		}

		if (res == -1)
			return res;

		vector<unsigned char> vec_BPreg;
		vec_BPreg.clear();
		for (int i = 0; i < 18; i++)
			vec_BPreg.push_back(bp_reg[i]);
		res = bl_sstWriteBP(vec_BPreg);
		//	cout<<"process_ok"<<endl;
	}
	return res;
}

int bl_sstWriteBP_SectInfo(bool mask)
{ // control address: 0x0000~0x1fff(8kbytes)
	int res;
	unsigned char bp_reg[18];
	unsigned char config_bit;

	res = bl_sstReadBP(bp_reg);

	if (res == 0)
	{
		if (mask == 0)
			bp_reg[1] &= 0xfc;
		else
			bp_reg[1] |= 0x03;
		//  if bp-masked.
		// read-out all 0x00.
	}
	printf("bp_reg: 0x%02x\n", bp_reg[16]);
	if (res == -1)
		return res;

	vector<unsigned char> vec_BPreg;
	vec_BPreg.clear();
	for (int i = 0; i < 18; i++)
		vec_BPreg.push_back(bp_reg[i]);

	res = bl_sstWriteBP(vec_BPreg);
	return res;
}

int bl_sstReadBP(unsigned char *bp_reg)
{
	int res = 0;
	int readbp_fbk;
	timetag_t t_bl_rdsstbp;
	time_tag(tag_all, &t_bl_rdsstbp);

	BL_RTC_CmdMsg.wait_cmdfbk.reset();
	Msg_Prot_BL_Host_to_RTC_encodeCmd(_ReadBlockProtection, 0, 0);

	readbp_fbk = BL_RTC_CmdMsg.wait_cmdfbk.tryWait(50);
	if (readbp_fbk == 1)
	{
		if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd == _ReadBlockProtection)
		{
			res = 0;
			for (int i = 0; i < 20; i++)
				bp_reg[i] = BL_RTC_CmdMsg.MSG_BL_RTC_cmd.ext_data[i];
		}
		else
		{
			res = -1;
			for (int i = 0; i < 20; i++)
				bp_reg[i] = 0x00;
		}
	}
	else
	{
		res = -1;
		for (int i = 0; i < 20; i++)
			bp_reg[i] = 0x00;
	}

	if (res == -1)
		goDriverLogger.Log("warning", "read-sst_BPREG time-out");

	time_tag(tag_delta, &t_bl_rdsstbp);
	printf("Read SST_BPREG, res:%d, execute time:%dms. \n", res, t_bl_rdsstbp.delta_tn);
	return res;
}

int bl_sstReadStatus(SST26_status_t *status)
{
	int res = 0;
	int status_fbk;
	timetag_t t_bl_rdstatus;
	time_tag(tag_all, &t_bl_rdstatus);

	SST26_status_t status_temp;

	BL_RTC_CmdMsg.wait_cmdfbk.reset();
	Msg_Prot_BL_Host_to_RTC_encodeCmd(_Read_Status_Reg, 0, 0);

	status_fbk = BL_RTC_CmdMsg.wait_cmdfbk.tryWait(50);
	if (status_fbk == 1)
	{
		if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd == _Read_Status_Reg)
			res = 0;
		else
			res = -1;
	}
	else
		res = -1;

	if (res == 0)
	{
		status_temp.BUSY = 0x01 & (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[1] >> 0);
		status_temp.WEL = 0x01 & (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[1] >> 1);
		status_temp.WSE = 0x01 & (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[1] >> 2);
		status_temp.WSP = 0x01 & (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[1] >> 3);
		status_temp.WPLD = 0x01 & (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[1] >> 4);
		status_temp.SEC = 0x01 & (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[1] >> 5);
		status_temp.RES = 0;
	}
	else
	{
		status_temp.BUSY = -1;
		status_temp.WEL = -1;
		status_temp.WSE = -1;
		status_temp.WSP = -1;
		status_temp.WPLD = -1;
		status_temp.SEC = -1;
		status_temp.RES = -1;
	}

	*status = status_temp;
	if (res == -1)
		goDriverLogger.Log("warning", "read-SST_STATUS time-out");

	time_tag(tag_delta, &t_bl_rdstatus);
	printf("Read SST_STATUS, res:%d, execute time:%dms. \n", res, t_bl_rdstatus.delta_tn);
	return res;
}

int bl_sstWREN(void)
{
	int res;
	int wren_fbk;
	timetag_t t_bl_wren;
	time_tag(tag_all, &t_bl_wren);

	BL_RTC_CmdMsg.wait_cmdfbk.reset();
	BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd = _Null;
	Msg_Prot_BL_Host_to_RTC_encodeCmd(_WREN_SST26V, 0, 0);

	wren_fbk = BL_RTC_CmdMsg.wait_cmdfbk.tryWait(50);
	if (wren_fbk == 1)
	{
		// printf("wren:%d\n",BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd );
		if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd == _WREN_SST26V)
			res = 0;
		else
			res = -1;
	}
	else
		res = -1;
	time_tag(tag_delta, &t_bl_wren);
	printf("_WREN_SST26V res:%d, execute time:%dms. \n", res, t_bl_wren.delta_tn);
	return res;
}

int bl_sstWRDI(void)
{
	int res;
	int wrdi_fbk;
	timetag_t t_bl_wrdi;
	time_tag(tag_all, &t_bl_wrdi);

	BL_RTC_CmdMsg.wait_cmdfbk.reset();
	BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd = _Null;
	Msg_Prot_BL_Host_to_RTC_encodeCmd(_WRDI, 0, 0);

	wrdi_fbk = BL_RTC_CmdMsg.wait_cmdfbk.tryWait(50);
	if (wrdi_fbk == 1)
	{
		// printf("wrdi_fbk:%d\n",BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd );
		if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd == _WRDI)
			res = 0;
		else
			res = -1;
	}
	else
		res = -1;
	time_tag(tag_delta, &t_bl_wrdi);
	printf("_WRDI res:%d, execute time:%dms. \n", res, t_bl_wrdi.delta_tn);
	return res;
}

void bl_sstRESET(void)
{
	int res;
	Msg_Prot_BL_Host_to_RTC_encodeCmd(_RESET_SST26V, 0, 0);
}

void bl_picRESET(void)
{
	int res;
	Msg_Prot_BL_Host_to_RTC_encodeCmd(_RESET_PIC, 0, 0);
}

int bl_block_erase(int addr)
{ // addr range: 000000H - 7FFFFFH.
	unsigned int ADDR_MASKp = 0x007fffff;
	unsigned int addr_temp = (unsigned int)addr & ADDR_MASKp;
	unsigned char addr_byte[3];
	addr_byte[2] = (addr_temp >> 16) & 0xff;
	addr_byte[1] = (addr_temp >> 8) & 0xff;
	addr_byte[0] = (addr_temp)&0xff;
	printf("addr:[ %02x, %02x, %02x]\n", addr_byte[2], addr_byte[1], addr_byte[0]);

	int res;
	int be_fbk;
	timetag_t t_bl_be;
	time_tag(tag_all, &t_bl_be);

	BL_RTC_CmdMsg.wait_cmdfbk.reset();
	BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd = _Null;
	Msg_Prot_BL_Host_to_RTC_encodeCmd(_Block_Erase, addr_byte, 0);

	be_fbk = BL_RTC_CmdMsg.wait_cmdfbk.tryWait(70);
	if (be_fbk == 1)
	{
		if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd == _Block_Erase)
			res = 0;
		else
			res = -1;
	}
	else
	{
		res = -1;
		goDriverLogger.Log("warning", "_Block_Erase time-out");
	}
	time_tag(tag_delta, &t_bl_be);
	printf("_Block_Erase res:%d, execute time:%dms. \n", res, t_bl_be.delta_tn);
	return res;
}

int bl_block_erase_section(int sectN)
{ // addr range: 000000H - 7FFFFFH.
	// //executed time: 400ms.
	int res;
	int SST_BASE_ADDR = 0x10000;
	Poco::Event wait_50ms;
	wait_50ms.reset();
	if (sectN > 10 || sectN < 0)
	{
		cout << "out of range" << endl;
		return -1;
	}
	bl_sstWriteBP_section(sectN, 0);
	timetag_t t_bl_be_sect;
	time_tag(tag_all, &t_bl_be_sect);

	sst_addr_t EraseAddr;
	EraseAddr.bytes.LLB = 0;
	EraseAddr.bytes.LHB = 0;
	EraseAddr.bytes.HLB = SST_SectBase[sectN];
	EraseAddr.bytes.HHB = 0;
	printf("EraseAddr: 0x%06X\n", EraseAddr.int_addr);
	for (int i = 0; i < 11; i++)
	{
		res = bl_block_erase(EraseAddr.int_addr);

		if (res == -1)
			break;
		EraseAddr.bytes.HLB++;
		wait_50ms.tryWait(10);
	}

	if (res == -1)
		cout << "SST-sect ease timeout." << endl;

	time_tag(tag_delta, &t_bl_be_sect);
	printf("SST-Sect_BlockErase res:%d, execute time:%dms. \n", res, t_bl_be_sect.delta_tn);
	return res;
}

int bl_sector_erase(int addr)
{ // addr range: 000000H - 7FFFFFH.
	unsigned int ADDR_MASKp = 0x007fffff;
	unsigned int addr_temp = (unsigned int)addr & ADDR_MASKp;
	unsigned char addr_byte[3];
	addr_byte[2] = (addr_temp >> 16) & 0xff;
	addr_byte[1] = (addr_temp >> 8) & 0xff;
	addr_byte[0] = (addr_temp)&0xff;
	printf("addr:[ %02x, %02x, %02x]\n", addr_byte[2], addr_byte[1], addr_byte[0]);

	int res;
	int se_fbk;
	timetag_t t_bl_se;
	time_tag(tag_all, &t_bl_se);

	BL_RTC_CmdMsg.wait_cmdfbk.reset();
	BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd = _Null;
	Msg_Prot_BL_Host_to_RTC_encodeCmd(_Sector_Erase, addr_byte, 0);

	se_fbk = BL_RTC_CmdMsg.wait_cmdfbk.tryWait(70);
	if (se_fbk == 1)
	{
		if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd == _Sector_Erase)
			res = 0;
		else
			res = -1;
	}
	else
		res = -1;

	if (res == -1)
		cout << "sector-erase timeout." << endl;

	time_tag(tag_delta, &t_bl_se);
	printf("_Sector_Erase res:%d, execute time:%dms. \n", res, t_bl_se.delta_tn);
	return res;
}

int bl_chip_erase(void)
{
	int res;
	int ce_fbk;
	timetag_t t_bl_ce;
	time_tag(tag_all, &t_bl_ce);

	BL_RTC_CmdMsg.wait_cmdfbk.reset();
	BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd = _Null;
	Msg_Prot_BL_Host_to_RTC_encodeCmd(_Chip_Erase, 0, 0);

	ce_fbk = BL_RTC_CmdMsg.wait_cmdfbk.tryWait(70);
	if (ce_fbk == 1)
	{
		if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd == _Chip_Erase)
			res = 0;
		else
			res = -1;
	}
	else
		res = -1;

	if (res == -1)
		cout << "chip-erase timeout." << endl;

	time_tag(tag_delta, &t_bl_ce);
	printf("_Chip_Erase res:%d, execute time:%dms. \n", res, t_bl_ce.delta_tn);
	return res;
}

int bl_backup_GenSeg_SSTsection(int sectN)
{ // executed time: 5200ms.
	int res;
	int backup_fbk;

	if (sectN > 10 || sectN < 0)
	{
		cout << "out of range" << endl;
		return -1;
	}
	unsigned char cmd_data[3];
	cmd_data[0] = 0x00;
	cmd_data[1] = BUSY_RespTime;
	cmd_data[2] = 0xff & (unsigned char)sectN;

	timetag_t t_bl_backup_pic;
	time_tag(tag_all, &t_bl_backup_pic);

	BL_RTC_CmdMsg.wait_cmdfbk.reset();
	BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd = _Null;
	Msg_Prot_BL_Host_to_RTC_encodeCmd(_Backup_GenSeg_SSTsect, cmd_data, 0);

	while (1)
	{
		backup_fbk = BL_RTC_CmdMsg.wait_cmdfbk.tryWait(BUSY_RespTime + 100);
		if (backup_fbk == 1)
		{
			if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[0] == _busyResp)
			{
				cout << "RTC is busy(backup_GenSeg)." << endl;
				continue;
			}
			else if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[0] == _positivResp)
			{
				if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd == _Backup_GenSeg_SSTsect_Resp)
				{
					cout << "RTC_Done (backup_GenSeg)." << endl;
					res = 0;
					break;
				}
				else
				{
					res = -1;
					break;
				}
			}
		}
		else
		{
			res = -2;
			break;
		}
	}
	time_tag(tag_delta, &t_bl_backup_pic);
	printf("Backup PIC-GenSeg_SSTsect, res:%d, execute time:%dms. \n", res, t_bl_backup_pic.delta_tn);

	return res;
}

int bl_read_sstMEM32(int addr, int *read_addr, unsigned char *read32)
{ // addr range: 000000H - 7FFFFFH.
	int res = 0;
	int readMEM_fbk;
	int addr_MAX = 0x7fffff;

	timetag_t t_bl_readMEM;
	time_tag(tag_all, &t_bl_readMEM);

	if (addr > addr_MAX)
	{
		cout << "out of range" << endl;
		return -1;
	}
	printf("ReqAddr:%04X \n", addr);
	BL_RTC_MemRead32.wait_mem32fbk.reset();
	BL_RTC_MemRead32.MSG_BL_RTC_mem.type = _Null_MemCtrl;

	Msg_Prot_BL_Host_to_RTC_encodeMEMRead(_HOST_READ_SST32, addr);

	readMEM_fbk = BL_RTC_MemRead32.wait_mem32fbk.tryWait(50);
	if (readMEM_fbk == 1)
	{
		if (BL_RTC_MemRead32.MSG_BL_RTC_mem.type = _RTCr_READ_SST32)
			res = 0;
		else
			res = -1;
	}
	else
		res = -1;

	if (res == 0)
	{
		printf(" \n");
		printf("addr[0]:0x%02X, ", BL_RTC_MemRead32.MSG_BL_RTC_mem.addr[0]);
		printf("addr[1]:0x%02X, ", BL_RTC_MemRead32.MSG_BL_RTC_mem.addr[1]);
		printf("addr[2]:0x%02X, ", BL_RTC_MemRead32.MSG_BL_RTC_mem.addr[2]);
		*read_addr = 0;
		*read_addr = 0x0000ff & ((int)BL_RTC_MemRead32.MSG_BL_RTC_mem.addr[0]);
		*read_addr |= 0x00ff00 & ((int)BL_RTC_MemRead32.MSG_BL_RTC_mem.addr[1]) << 8;
		*read_addr |= 0xff0000 & ((int)BL_RTC_MemRead32.MSG_BL_RTC_mem.addr[2]) << 16;
		for (int i = 0; i < 32; i++)
			read32[i] = BL_RTC_MemRead32.MSG_BL_RTC_mem.data[i];
	}

	if (res == -1)
		goDriverLogger.Log("warning", "read-sst_MEM32 time-out");

	time_tag(tag_delta, &t_bl_readMEM);
	printf("Read sst_MEM32, res:%d, execute time:%dms. \n", res, t_bl_readMEM.delta_tn);
	return res;
}

int bl_write_sstMEM32(int addr, vector<unsigned char> write32, unsigned char *chksum, unsigned char *chksumH)
{ // addr range: 000000H - 7FFFFFH.
	int res = 0;
	int writeMEM_fbk;
	int addr_MAX = 0x7fffff;
	Poco::Event wait_20ms;
	timetag_t t_bl_readMEM;
	time_tag(tag_all, &t_bl_readMEM);

	if (addr > addr_MAX)
	{
		cout << "out of range" << endl;
		return -1;
	}

	unsigned char txreg[32];

	for (int i = 0; i < 32; i++)
	{
		if (i < write32.size())
			txreg[i] = write32[i];
		else
			txreg[i] = 0xff;
	}
	//	cout << "write32_size: "<<std::dec<< write32.size() << endl;
	//	for( int i = 0; i<32; i++ )
	//	{
	//		printf("wr[%2d]:0x%02X, ",i,txreg[i]);
	//			if( (i+1)%8 == 0 )
	//		printf("\n");
	//	}

	BL_RTC_MemEcho.wait_memechofbk.reset();
	BL_RTC_MemEcho.MSG_BL_RTC_mem.type = _Null_MemCtrl;
	// wait_20ms.tryWait(40);
	Msg_Prot_BL_Host_to_RTC_encodeMEMWrite(_HOST_WRIT_SST32, addr, txreg);

	writeMEM_fbk = BL_RTC_MemEcho.wait_memechofbk.tryWait(100);
	if (writeMEM_fbk == 1)
	{
		if (BL_RTC_MemEcho.MSG_BL_RTC_mem.type = _RTCr_WRIT_SST32)
			res = 0;
		else
			res = -2;
	}
	else
		res = -1;

	if (res == 0)
	{
		*chksum = BL_RTC_MemEcho.MSG_BL_RTC_mem.chksum;
		*chksumH = BL_RTC_MemEcho.MSG_BL_RTC_mem.chksumH;
	}

	if (res == -1)
		goDriverLogger.Log("warning", "write-sst_MEM32 time-out");
	if (res == -2)
		goDriverLogger.Log("warning", "write-sst_MEM32 data-missing");

	time_tag(tag_delta, &t_bl_readMEM);
	if (res == -1)
		printf("Write sst_MEM32, res:%d, execute time:%dms. \n", res, t_bl_readMEM.delta_tn);
	return res;
}

int bl_read_sstMEM16(int addr, int *read_addr, unsigned char *read16)
{ // addr range: 000000H - 7FFFFFH.
	int res = 0;
	int readMEM_fbk;
	int addr_MAX = 0x7fffff;

	timetag_t t_bl_readMEM;
	time_tag(tag_all, &t_bl_readMEM);

	if (addr > addr_MAX)
	{
		cout << "out of range" << endl;
		return -1;
	}

	BL_RTC_MemRead16.wait_mem16fbk.reset();
	BL_RTC_MemRead16.MSG_BL_RTC_mem.type = _Null_MemCtrl;

	Msg_Prot_BL_Host_to_RTC_encodeMEMRead(_HOST_READ_SST16, addr);

	readMEM_fbk = BL_RTC_MemRead16.wait_mem16fbk.tryWait(50);
	if (readMEM_fbk == 1)
	{
		if (BL_RTC_MemRead16.MSG_BL_RTC_mem.type = _RTCr_READ_SST16)
			res = 0;
		else
			res = -1;
	}
	else
		res = -1;

	if (res == 0)
	{
		*read_addr = 0;
		*read_addr = 0x0000ff & ((int)BL_RTC_MemRead16.MSG_BL_RTC_mem.addr[0]);
		*read_addr |= 0x00ff00 & ((int)BL_RTC_MemRead16.MSG_BL_RTC_mem.addr[1]) << 8;
		*read_addr |= 0xff0000 & ((int)BL_RTC_MemRead16.MSG_BL_RTC_mem.addr[2]) << 16;
		for (int i = 0; i < 16; i++)
			read16[i] = BL_RTC_MemRead16.MSG_BL_RTC_mem.data[i];
	}

	if (res == -1)
		goDriverLogger.Log("warning", "read-sst_MEM16 time-out");

	time_tag(tag_delta, &t_bl_readMEM);
	printf("Read sst_MEM16, res:%d, execute time:%dms. \n", res, t_bl_readMEM.delta_tn);
	return res;
}
int bl_write_sstMEM16(int addr, vector<unsigned char> write16, unsigned char *chksum, unsigned char *chksumH)
{ // addr range: 000000H - 7FFFFFH.
	int res = 0;
	int writeMEM_fbk;
	int addr_MAX = 0x7fffff;

	timetag_t t_bl_readMEM;
	time_tag(tag_all, &t_bl_readMEM);

	if (addr > addr_MAX)
	{
		cout << "out of range" << endl;
		return -1;
	}

	unsigned char txreg[16];

	for (int i = 0; i < 16; i++)
	{
		if (i < write16.size())
			txreg[i] = write16[i];
		else
			txreg[i] = 0xff;
	}

	//	for( int i = 0; i<16; i++ )
	//	{
	//		printf("wr[%2d]:0x%02X, ",i,txreg[i]);
	//			if( (i+1)%8 == 0 )
	//		printf("\n");
	//	}

	BL_RTC_MemEcho.wait_memechofbk.reset();
	BL_RTC_MemEcho.MSG_BL_RTC_mem.type = _Null_MemCtrl;

	Msg_Prot_BL_Host_to_RTC_encodeMEMWrite(_HOST_WRIT_SST16, addr, txreg);

	writeMEM_fbk = BL_RTC_MemEcho.wait_memechofbk.tryWait(50);
	if (writeMEM_fbk == 1)
	{
		if (BL_RTC_MemEcho.MSG_BL_RTC_mem.type = _RTCr_WRIT_SST16)
			res = 0;
		else
			res = -1;
	}
	else
		res = -1;

	if (res == 0)
	{
		*chksum = BL_RTC_MemEcho.MSG_BL_RTC_mem.chksum;
		*chksumH = BL_RTC_MemEcho.MSG_BL_RTC_mem.chksumH;
	}

	if (res == -1)
		goDriverLogger.Log("warning", "write-sst_MEM16 time-out");

	time_tag(tag_delta, &t_bl_readMEM);
	//	printf("Write sst_MEM16, res:%d, execute time:%dms. \n", res,t_bl_readMEM.delta_tn );
	return res;
}

int bl_read_picMEM32(int addr, int *read_addr, unsigned char *read32)
{
	int res = 0;
	int readMEM_fbk;
	int addr_MAX = 0x7ffff8;

	timetag_t t_bl_readMEM;
	time_tag(tag_all, &t_bl_readMEM);

	if (addr > addr_MAX)
	{
		cout << "out of range" << endl;
		return -1;
	}

	BL_RTC_MemRead32.wait_mem32fbk.reset();
	BL_RTC_MemRead32.MSG_BL_RTC_mem.type = _Null_MemCtrl;

	Msg_Prot_BL_Host_to_RTC_encodeMEMRead(_HOST_READ_PIC32, addr);

	readMEM_fbk = BL_RTC_MemRead32.wait_mem32fbk.tryWait(50);
	if (readMEM_fbk == 1)
	{
		if (BL_RTC_MemRead32.MSG_BL_RTC_mem.type = _RTCr_READ_PIC32)
			res = 0;
		else
			res = -1;
	}
	else
		res = -1;

	if (res == 0)
	{
		*read_addr = 0;
		*read_addr = 0x0000ff & ((int)BL_RTC_MemRead32.MSG_BL_RTC_mem.addr[0]);
		*read_addr |= 0x00ff00 & ((int)BL_RTC_MemRead32.MSG_BL_RTC_mem.addr[1]) << 8;
		*read_addr |= 0xff0000 & ((int)BL_RTC_MemRead32.MSG_BL_RTC_mem.addr[2]) << 16;
		for (int i = 0; i < 32; i++)
			read32[i] = BL_RTC_MemRead32.MSG_BL_RTC_mem.data[i];
	}
	if (res == -1)
		goDriverLogger.Log("warning", "read-pic_MEM32 time-out");

	time_tag(tag_delta, &t_bl_readMEM);
	printf("Read pic_MEM32, res:%d, execute time:%dms. \n", res, t_bl_readMEM.delta_tn);
	return res;
}
int bl_read_picMEM16(int addr, int *read_addr, unsigned char *read16)
{
	int res = 0;
	int readMEM_fbk;
	int addr_MAX = 0x7ffff8;
	Poco::Event wait_10ms;

	timetag_t t_bl_readMEM;
	time_tag(tag_all, &t_bl_readMEM);

	if (addr > addr_MAX)
	{
		cout << "out of range" << endl;
		return -1;
	}

	BL_RTC_MemRead16.wait_mem16fbk.reset();
	BL_RTC_MemRead16.MSG_BL_RTC_mem.type = _Null_MemCtrl;

	Msg_Prot_BL_Host_to_RTC_encodeMEMRead(_HOST_READ_PIC16, addr);
	readMEM_fbk = BL_RTC_MemRead16.wait_mem16fbk.tryWait(50);
	if (readMEM_fbk == 1)
	{
		if (BL_RTC_MemRead16.MSG_BL_RTC_mem.type = _RTCr_READ_PIC16)
			res = 0;
		else
			res = -1;
	}
	else
		res = -1;

	if (res == 0)
	{
		*read_addr = 0;
		*read_addr = 0x0000ff & ((int)BL_RTC_MemRead16.MSG_BL_RTC_mem.addr[0]);
		*read_addr |= 0x00ff00 & ((int)BL_RTC_MemRead16.MSG_BL_RTC_mem.addr[1]) << 8;
		*read_addr |= 0xff0000 & ((int)BL_RTC_MemRead16.MSG_BL_RTC_mem.addr[2]) << 16;
		for (int i = 0; i < 16; i++)
			read16[i] = BL_RTC_MemRead16.MSG_BL_RTC_mem.data[i];
	}
	if (res == -1)
		goDriverLogger.Log("warning", "read-pic_MEM16 time-out");

	time_tag(tag_delta, &t_bl_readMEM);
	printf("Read pic_MEM16, res:%d, execute time:%dms. \n", res, t_bl_readMEM.delta_tn);
	return res;
}

int bl_read_chksum_PIC(int seg_sel, Read_segchksum_t *seg_checksum)
{
	// executed time:
	//	general-segment:  450ms.
	//	auxiliary-segment: 25ms.
	int res = 0;
	int rdchksum_fbk;
	unsigned char tx_cmddata[3];
	Read_segchksum_t resp_chksum;

	for (int i = 0; i < 3; i++)
		tx_cmddata[i] = 0;

	if (seg_sel > 0)
		tx_cmddata[0] = 1;
	tx_cmddata[1] = BUSY_RespTime;

	timetag_t t_bl_rdchksum_pic;
	time_tag(tag_all, &t_bl_rdchksum_pic);

	BL_RTC_CmdMsg.wait_cmdfbk.reset();
	Msg_Prot_BL_Host_to_RTC_encodeCmd(_Read_Chksum_PIC, tx_cmddata, 0);

	while (1)
	{
		rdchksum_fbk = BL_RTC_CmdMsg.wait_cmdfbk.tryWait(BUSY_RespTime + 50);
		if (rdchksum_fbk == 1)
		{
			if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[0] == _busyResp)
			{
				cout << "RTC is busy." << endl;
				continue;
			}
			else if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[0] == _positivResp)
			{
				if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd == _Resp_Chksum_PIC)
				{
					resp_chksum.chksum = 0x00ff & (unsigned short)BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[1];
					resp_chksum.chksum |= 0xff00 & ((unsigned short)BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[2] << 8);
					resp_chksum.segment = tx_cmddata[0];
					res = 0;
					break;
				}
				else
				{
					res = -1;
					resp_chksum.segment = -1;
					resp_chksum.chksum = -1;
				}
			}
		}
		else
		{
			res = -2;
			resp_chksum.segment = -1;
			resp_chksum.chksum = -1;
			break;
		}
	}

	if (res == -1)
		goDriverLogger.Log("warning", "read-chksum-pic response fault");
	if (res == -2)
		goDriverLogger.Log("warning", "read-chksum-pic time-out");

	*seg_checksum = resp_chksum;
	time_tag(tag_delta, &t_bl_rdchksum_pic);
	printf("Read Chksum_PIC, res:%d, execute time:%dms. \n", res, t_bl_rdchksum_pic.delta_tn);

	return res;
}

int bl_read_chksum_SST(int sectN, int seg_sel, Read_segchksum_t *seg_checksum)
{
	// executed time:
	//	general-segment:  2250ms.

	int res = 0;
	int rdchksum_fbk;
	unsigned char tx_cmddata[3];
	Read_segchksum_t resp_chksum;

	if (sectN < 0 || sectN > 10)
		return -1;

	for (int i = 0; i < 3; i++)
		tx_cmddata[i] = 0;

	if (seg_sel > 0)
		tx_cmddata[0] = 1;
	tx_cmddata[1] = BUSY_RespTime + 50;
	tx_cmddata[2] = 0xff & (unsigned char)sectN;

	timetag_t t_bl_rdchksum_sst;
	time_tag(tag_all, &t_bl_rdchksum_sst);

	BL_RTC_CmdMsg.wait_cmdfbk.reset();
	BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd = _Null;
	Msg_Prot_BL_Host_to_RTC_encodeCmd(_Read_Chksum_SSTsect, tx_cmddata, 0);

	while (1)
	{
		rdchksum_fbk = BL_RTC_CmdMsg.wait_cmdfbk.tryWait(BUSY_RespTime + 100);
		if (rdchksum_fbk == 1)
		{
			if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[0] == _busyResp)
			{
				cout << "RTC is busy." << endl;
				continue;
			}
			else if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[0] == _positivResp)
			{
				if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd == _Resp_Chksum_SSTsect)
				{
					resp_chksum.chksum = 0x00ff & (unsigned short)BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[1];
					resp_chksum.chksum |= 0xff00 & ((unsigned short)BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[2] << 8);
					resp_chksum.segment = tx_cmddata[0];
					res = 0;
					break;
				}
				else
				{
					res = -1;
					resp_chksum.segment = -1;
					resp_chksum.chksum = -1;
				}
			}
		}
		else
		{
			res = -2;
			resp_chksum.segment = -1;
			resp_chksum.chksum = -1;
			break;
		}
	}

	if (res == -1)
		goDriverLogger.Log("warning", "read-chksum-sst26 response fault");
	if (res == -2)
		goDriverLogger.Log("warning", "read-chksum-sst26 time-out");

	*seg_checksum = resp_chksum;
	time_tag(tag_delta, &t_bl_rdchksum_sst);
	printf("Read Chksum_SST26, res:%d, execute time:%dms. \n", res, t_bl_rdchksum_sst.delta_tn);

	return res;
}

int bl_read_h32_VersionNum(VersionInfo_t *ver_fbk)
{
	int res = 0;
	if (h32_source.data_addr.empty())
	{
		goDriverLogger.Log("warning", "inner h32_source is empty");
		res = -1;
		return res;
	}
	if (h32_source.row_data.empty())
	{
		goDriverLogger.Log("warning", "inner h32_source is empty");
		res = -1;
		return res;
	}

	int i_vect_ptr = 0;
	int i_curr_addr;
	int hexsour_size = h32_source.data_addr.size();
	int PIC_GenADDR_MAX = 0x55800 * 2;
	int PIC_VERSION_INFO_ADDR = 0x200 * 2;
	int PIC_BOOTLOAD_INFO_ADDR = 0x210 * 2;

	VersionInfo_t vernum;

	while (i_vect_ptr < hexsour_size)
	{
		i_curr_addr = h32_source.data_addr[i_vect_ptr];
		if (i_curr_addr > PIC_GenADDR_MAX)
		{
			cout << "PIC_GenADDR_MAX reached." << endl;
			res = -2;
			break;
		}

		if (i_curr_addr == PIC_VERSION_INFO_ADDR)
		{
			vernum.RTC_Base = 0x00ff & h32_source.row_data[i_vect_ptr][0];
			vernum.RTC_Base |= 0xff00 & (h32_source.row_data[i_vect_ptr][1]) << 8;
			vernum.RTC_Major = 0x00ff & h32_source.row_data[i_vect_ptr][4];
			vernum.RTC_Major |= 0xff00 & (h32_source.row_data[i_vect_ptr][5]) << 8;
			vernum.RTC_Minor = 0x00ff & h32_source.row_data[i_vect_ptr][8];
			vernum.RTC_Minor |= 0xff00 & (h32_source.row_data[i_vect_ptr][9]) << 8;

			vernum.MsgProt_Base = 0x00ff & h32_source.row_data[i_vect_ptr][16];
			vernum.MsgProt_Base |= 0xff00 & (h32_source.row_data[i_vect_ptr][17]) << 8;
			vernum.MsgProt_Major = 0x00ff & h32_source.row_data[i_vect_ptr][20];
			vernum.MsgProt_Major |= 0xff00 & (h32_source.row_data[i_vect_ptr][21]) << 8;
			vernum.MsgProt_Minor = 0x00ff & h32_source.row_data[i_vect_ptr][24];
			vernum.MsgProt_Minor |= 0xff00 & (h32_source.row_data[i_vect_ptr][25]) << 8;
			res = 0;
		}

		if (i_curr_addr == PIC_BOOTLOAD_INFO_ADDR)
		{
			vernum.BL_Base = 0x00ff & h32_source.row_data[i_vect_ptr][0];
			vernum.BL_Base |= 0xff00 & (h32_source.row_data[i_vect_ptr][1]) << 8;
			vernum.BL_Major = 0x00ff & h32_source.row_data[i_vect_ptr][4];
			vernum.BL_Major |= 0xff00 & (h32_source.row_data[i_vect_ptr][5]) << 8;
			vernum.BL_Minor = 0x00ff & h32_source.row_data[i_vect_ptr][8];
			vernum.BL_Minor |= 0xff00 & (h32_source.row_data[i_vect_ptr][9]) << 8;
			res = 0;
			break;
		}
		i_vect_ptr++;
	}

	if (res < 0)
	{
		vernum.RTC_Base = -1;
		vernum.RTC_Major = -1;
		vernum.RTC_Minor = -1;
		vernum.MsgProt_Base = -1;
		vernum.MsgProt_Major = -1;
		vernum.MsgProt_Minor = -1;
		vernum.BL_Base = -1;
		vernum.BL_Major = -1;
		vernum.BL_Minor = -1;
	}

	*ver_fbk = vernum;
	return res;
}

int bl_read_pic_VersionNum(VersionInfo_t *ver_fbk)
{
	int res = 0;
	int readbp_fbk;
	timetag_t t_bl_rdpicver;
	time_tag(tag_all, &t_bl_rdpicver);
	unsigned char fbk_extdata[20];
	BL_RTC_CmdMsg.wait_cmdfbk.reset();
	BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd = _Null;
	Msg_Prot_BL_Host_to_RTC_encodeCmd(_Read_Version_PIC, 0, 0);

	VersionInfo_t vernum;
	readbp_fbk = BL_RTC_CmdMsg.wait_cmdfbk.tryWait(50);
	if (readbp_fbk == 1)
	{
		if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd == _Read_Version_PIC)
		{
			res = 0;
			for (int i = 0; i < 20; i++)
				fbk_extdata[i] = BL_RTC_CmdMsg.MSG_BL_RTC_cmd.ext_data[i];
		}
		else
		{
			res = -1;
			for (int i = 0; i < 20; i++)
				fbk_extdata[i] = 0x00;
		}
	}
	else
	{
		res = -1;
		for (int i = 0; i < 20; i++)
			fbk_extdata[i] = 0x00;
	}

	if (res == 0)
	{
		vernum.RTC_Base = 0x00ff & (int)fbk_extdata[0];
		vernum.RTC_Base |= 0xff00 & ((int)fbk_extdata[1]) << 8;
		vernum.RTC_Major = 0x00ff & (int)fbk_extdata[2];
		vernum.RTC_Major |= 0xff00 & ((int)fbk_extdata[3]) << 8;
		vernum.RTC_Minor = 0x00ff & (int)fbk_extdata[4];
		vernum.RTC_Minor |= 0xff00 & ((int)fbk_extdata[5]) << 8;

		vernum.MsgProt_Base = 0x00ff & (int)fbk_extdata[6];
		vernum.MsgProt_Base |= 0xff00 & ((int)fbk_extdata[7]) << 8;
		vernum.MsgProt_Major = 0x00ff & (int)fbk_extdata[8];
		vernum.MsgProt_Major |= 0xff00 & ((int)fbk_extdata[9]) << 8;
		vernum.MsgProt_Minor = 0x00ff & (int)fbk_extdata[10];
		vernum.MsgProt_Minor |= 0xff00 & ((int)fbk_extdata[11]) << 8;

		vernum.BL_Base = 0x00ff & (int)fbk_extdata[12];
		vernum.BL_Base |= 0xff00 & ((int)fbk_extdata[13]) << 8;
		vernum.BL_Major = 0x00ff & (int)fbk_extdata[14];
		vernum.BL_Major |= 0xff00 & ((int)fbk_extdata[15]) << 8;
		vernum.BL_Minor = 0x00ff & (int)fbk_extdata[16];
		vernum.BL_Minor |= 0xff00 & ((int)fbk_extdata[17]) << 8;

		for (int i = 0; i < 18; i++)
			printf(": 0x%02x ", fbk_extdata[i]);
	}

	if (res < 0)
	{
		vernum.RTC_Base = -1;
		vernum.RTC_Major = -1;
		vernum.RTC_Minor = -1;
		vernum.MsgProt_Base = -1;
		vernum.MsgProt_Major = -1;
		vernum.MsgProt_Minor = -1;
		vernum.BL_Base = -1;
		vernum.BL_Major = -1;
		vernum.BL_Minor = -1;
	}

	*ver_fbk = vernum;

	if (res == -1)
		goDriverLogger.Log("warning", "read-pic version number time-out");
	time_tag(tag_delta, &t_bl_rdpicver);
	printf("_Read_Version_PIC, res:%d, execute time:%dms. \n", res, t_bl_rdpicver.delta_tn);
	return res;
}

int bl_read_SST_VersionNum(int sectN, VersionInfo_t *ver_fbk)
{
	int res = 0;
	int rdsstver_fbk;
	unsigned char tx_cmddata[3];
	unsigned char fbk_extdata[20];
	VersionInfo_t SSTvernum;

	if (sectN < 0 || sectN > 10)
		return -1;

	for (int i = 0; i < 3; i++)
		tx_cmddata[i] = 0;
	tx_cmddata[0] = 0xff & (unsigned char)sectN;

	timetag_t t_bl_rdsstver;
	time_tag(tag_all, &t_bl_rdsstver);

	BL_RTC_CmdMsg.wait_cmdfbk.reset();
	BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd = _Null;
	Msg_Prot_BL_Host_to_RTC_encodeCmd(_Read_Version_SSTsect, tx_cmddata, 0);

	rdsstver_fbk = BL_RTC_CmdMsg.wait_cmdfbk.tryWait(50);
	if (rdsstver_fbk == 1)
	{
		if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd == _Read_Version_SSTsect)
		{
			res = 0;
			for (int i = 0; i < 20; i++)
				fbk_extdata[i] = BL_RTC_CmdMsg.MSG_BL_RTC_cmd.ext_data[i];
		}
		else
		{
			res = -1;
			for (int i = 0; i < 20; i++)
				fbk_extdata[i] = 0x00;
		}
	}
	else
	{
		res = -1;
		for (int i = 0; i < 20; i++)
			fbk_extdata[i] = 0x00;
	}

	if (res == 0)
	{
		SSTvernum.RTC_Base = 0x00ff & (int)fbk_extdata[0];
		SSTvernum.RTC_Base |= 0xff00 & ((int)fbk_extdata[1]) << 8;
		SSTvernum.RTC_Major = 0x00ff & (int)fbk_extdata[2];
		SSTvernum.RTC_Major |= 0xff00 & ((int)fbk_extdata[3]) << 8;
		SSTvernum.RTC_Minor = 0x00ff & (int)fbk_extdata[4];
		SSTvernum.RTC_Minor |= 0xff00 & ((int)fbk_extdata[5]) << 8;

		SSTvernum.MsgProt_Base = 0x00ff & (int)fbk_extdata[6];
		SSTvernum.MsgProt_Base |= 0xff00 & ((int)fbk_extdata[7]) << 8;
		SSTvernum.MsgProt_Major = 0x00ff & (int)fbk_extdata[8];
		SSTvernum.MsgProt_Major |= 0xff00 & ((int)fbk_extdata[9]) << 8;
		SSTvernum.MsgProt_Minor = 0x00ff & (int)fbk_extdata[10];
		SSTvernum.MsgProt_Minor |= 0xff00 & ((int)fbk_extdata[11]) << 8;

		SSTvernum.BL_Base = 0x00ff & (int)fbk_extdata[12];
		SSTvernum.BL_Base |= 0xff00 & ((int)fbk_extdata[13]) << 8;
		SSTvernum.BL_Major = 0x00ff & (int)fbk_extdata[14];
		SSTvernum.BL_Major |= 0xff00 & ((int)fbk_extdata[15]) << 8;
		SSTvernum.BL_Minor = 0x00ff & (int)fbk_extdata[16];
		SSTvernum.BL_Minor |= 0xff00 & ((int)fbk_extdata[17]) << 8;

		for (int i = 0; i < 18; i++)
			printf(": 0x%02x ", fbk_extdata[i]);
	}

	if (res < 0)
	{
		SSTvernum.RTC_Base = -1;
		SSTvernum.RTC_Major = -1;
		SSTvernum.RTC_Minor = -1;
		SSTvernum.MsgProt_Base = -1;
		SSTvernum.MsgProt_Major = -1;
		SSTvernum.MsgProt_Minor = -1;
		SSTvernum.BL_Base = -1;
		SSTvernum.BL_Major = -1;
		SSTvernum.BL_Minor = -1;
	}

	*ver_fbk = SSTvernum;

	if (res == -1)
		goDriverLogger.Log("warning", "read-sst-n version number time-out");
	time_tag(tag_delta, &t_bl_rdsstver);
	printf("_Read_Version_SSTsect, res:%d, execute time:%dms. \n", res, t_bl_rdsstver.delta_tn);
	return res;
}

int bl_read_h32_BLmessage(char *bl_msg)
{
	int res = 0;
	if (h32_source.data_addr.empty())
	{
		goDriverLogger.Log("warning", "inner h32_source is empty");
		res = -1;
		return res;
	}
	if (h32_source.row_data.empty())
	{
		goDriverLogger.Log("warning", "inner h32_source is empty");
		res = -1;
		return res;
	}

	int i_vect_ptr = 0;
	int i_curr_addr;
	int hexsour_size = h32_source.data_addr.size();
	int PIC_GenADDR_MAX = 0x55800 * 2;
	int PIC_BL_MSG_ADDR1 = 0x220 * 2;
	int PIC_BL_MSG_ADDR2 = 0x230 * 2;
	int PIC_BL_MSG_ADDR3 = 0x240 * 2;

	char rd_bl_msg[48];

	while (i_vect_ptr < hexsour_size)
	{
		i_curr_addr = h32_source.data_addr[i_vect_ptr];
		if (i_curr_addr > PIC_GenADDR_MAX)
		{
			cout << "PIC_GenADDR_MAX reached." << endl;
			res = -2;
			break;
		}

		if (i_curr_addr == PIC_BL_MSG_ADDR1)
		{
			for (int i = 0; i < 16; i += 2)
			{
				rd_bl_msg[i] = (char)h32_source.row_data[i_vect_ptr][i * 2];
				rd_bl_msg[i + 1] = (char)h32_source.row_data[i_vect_ptr][i * 2 + 1];
				// printf(": 0x%02x,0x%02x\n",rd_bl_msg[i],rd_bl_msg[i+1]);
			}
		}
		if (i_curr_addr == PIC_BL_MSG_ADDR2)
		{
			for (int i = 0; i < 16; i += 2)
			{
				rd_bl_msg[i + 16] = (char)h32_source.row_data[i_vect_ptr][i * 2];
				rd_bl_msg[i + 16 + 1] = (char)h32_source.row_data[i_vect_ptr][i * 2 + 1];
				// printf(": 0x%02x,0x%02x\n",rd_bl_msg[i],rd_bl_msg[i+1]);
			}
		}
		if (i_curr_addr == PIC_BL_MSG_ADDR3)
		{
			for (int i = 0; i < 16; i += 2)
			{
				rd_bl_msg[i + 32] = (char)h32_source.row_data[i_vect_ptr][i * 2];
				rd_bl_msg[i + 32 + 1] = (char)h32_source.row_data[i_vect_ptr][i * 2 + 1];
				// printf(": 0x%02x,0x%02x\n",rd_bl_msg[i],rd_bl_msg[i+1]);
			}
			res = 0;
			break;
		}
		i_vect_ptr++;
	}
	for (int i = 0; i < 48; ++i)
		bl_msg[i] = rd_bl_msg[i];
	return res;
}

int bl_read_pic_BLmessage(char *bl_msg)
{
	int res = 0;
	int blmsg_fbk;
	timetag_t t_bl_rdpicmsg;
	time_tag(tag_all, &t_bl_rdpicmsg);
	unsigned char fbk_extdata[20];
	char fbkmsg[48];
	for (int i = 0; i < 48; i++)
		fbkmsg[i] = 0;

	BL_RTC_CmdMsg.wait_cmdfbk.reset();
	BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd = _Null;
	Msg_Prot_BL_Host_to_RTC_encodeCmd(_Read_Note_PIC, 0, 0);

	while (1)
	{
		blmsg_fbk = BL_RTC_CmdMsg.wait_cmdfbk.tryWait(50);
		if (blmsg_fbk == 1)
		{
			if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd == _Read_Note_PIC)
			{
				if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[0] == 1)
				{
					for (int i = 0; i < 16; i++)
						fbkmsg[i] = BL_RTC_CmdMsg.MSG_BL_RTC_cmd.ext_data[i];
					continue;
				}
				else if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[0] == 2)
				{
					for (int i = 0; i < 16; i++)
						fbkmsg[i + 16] = BL_RTC_CmdMsg.MSG_BL_RTC_cmd.ext_data[i];
					continue;
				}
				else if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[0] == 3)
				{
					for (int i = 0; i < 16; i++)
						fbkmsg[i + 32] = BL_RTC_CmdMsg.MSG_BL_RTC_cmd.ext_data[i];
					continue;
				}
				else if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[0] == 0xff)
				{
					cout << "end of message." << endl;
					res = 0;
					break;
				}
			}
			else
			{
				cout << "read message missing." << endl;
				res = -1;
				break;
			}
		}
		else
		{
			cout << "read message time-out" << endl;
			res = -2;
			break;
		}
	}

	if (res < 0)
	{
		for (int i = 0; i < 48; i++)
			fbkmsg[i] = 0;
	}

	for (int i = 0; i < 48; ++i)
		bl_msg[i] = fbkmsg[i];

	if (res == -1)
		goDriverLogger.Log("warning", "read-pic bl-message data-missing");
	if (res == -2)
		goDriverLogger.Log("warning", "read-pic bl-message time-out");
	time_tag(tag_delta, &t_bl_rdpicmsg);
	printf("_Read_Message_PIC, res:%d, execute time:%dms. \n", res, t_bl_rdpicmsg.delta_tn);
	return res;
}

int bl_read_sst_BLmessage(int sectN, char *bl_msg)
{
	int res = 0;
	int blmsg_fbk;
	timetag_t t_bl_rdsstmsg;
	time_tag(tag_all, &t_bl_rdsstmsg);
	unsigned char fbk_extdata[20];
	unsigned char tx_cmddata[3];
	char fbkmsg[48];

	if (sectN < 0 || sectN > 10)
		return -1;

	for (int i = 0; i < 3; i++)
		tx_cmddata[i] = 0;
	tx_cmddata[0] = 0xff & (unsigned char)sectN;
	for (int i = 0; i < 48; i++)
		fbkmsg[i] = 0;

	BL_RTC_CmdMsg.wait_cmdfbk.reset();
	BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd = _Null;
	Msg_Prot_BL_Host_to_RTC_encodeCmd(_Read_Note_SSTsect, tx_cmddata, 0);

	while (1)
	{
		blmsg_fbk = BL_RTC_CmdMsg.wait_cmdfbk.tryWait(50);
		if (blmsg_fbk == 1)
		{
			if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd == _Read_Note_SSTsect)
			{
				if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[0] == 1)
				{
					for (int i = 0; i < 16; i++)
						fbkmsg[i] = BL_RTC_CmdMsg.MSG_BL_RTC_cmd.ext_data[i];
					continue;
				}
				else if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[0] == 2)
				{
					for (int i = 0; i < 16; i++)
						fbkmsg[i + 16] = BL_RTC_CmdMsg.MSG_BL_RTC_cmd.ext_data[i];
					continue;
				}
				else if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[0] == 3)
				{
					for (int i = 0; i < 16; i++)
						fbkmsg[i + 32] = BL_RTC_CmdMsg.MSG_BL_RTC_cmd.ext_data[i];
					continue;
				}
				else if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[0] == 0xff)
				{
					cout << "end of sst-message." << endl;
					res = 0;
					break;
				}
			}
			else
			{
				cout << "read message missing." << endl;
				res = -1;
				break;
			}
		}
		else
		{
			cout << "read message time-out" << endl;
			res = -2;
			break;
		}
	}

	if (res < 0)
	{
		for (int i = 0; i < 48; i++)
			fbkmsg[i] = 0;
	}

	for (int i = 0; i < 48; ++i)
	{
		bl_msg[i] = fbkmsg[i];
		printf("0x%2x ", fbkmsg[i]);
	}
	printf(" \n");

	if (res == -1)
		goDriverLogger.Log("warning", "read-sst bl-message data-missing");
	if (res == -2)
		goDriverLogger.Log("warning", "read-sst bl-message time-out");
	time_tag(tag_delta, &t_bl_rdsstmsg);
	printf("_Read_Note_SSTsect, res:%d, execute time:%dms. \n", res, t_bl_rdsstmsg.delta_tn);
	return res;
}

int bl_read_SectInfo_n(int sectN, SectInfo_n_t *fbk_info_n)
{ // n=0~10
	int res = 0;
	int sectinfo_fbk;
	timetag_t t_bl_rdsectinfo;
	time_tag(tag_all, &t_bl_rdsectinfo);
	unsigned char fbk_extdata[20];
	unsigned char tx_cmddata[3];
	char fbkmsg[64];
	char *pfbkmsg = fbkmsg;

	if (sectN < 0 || sectN > 10)
		return -1;

	for (int i = 0; i < 3; i++)
		tx_cmddata[i] = 0;
	tx_cmddata[0] = 0xff & (unsigned char)sectN;
	for (int i = 0; i < sizeof(fbkmsg); i++)
		fbkmsg[i] = 0;

	BL_RTC_CmdMsg.wait_cmdfbk.reset();
	BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd = _Null;
	Msg_Prot_BL_Host_to_RTC_encodeCmd(_Read_SectInfo_n, tx_cmddata, 0);
	while (1)
	{
		sectinfo_fbk = BL_RTC_CmdMsg.wait_cmdfbk.tryWait(50);
		if (sectinfo_fbk == 1)
		{
			if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd == _Read_SectInfo_n)
			{
				if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[0] == 1)
				{
					for (int i = 0; i < 16; i++)
						fbkmsg[i] = BL_RTC_CmdMsg.MSG_BL_RTC_cmd.ext_data[i];
					continue;
				}
				else if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[0] == 2)
				{
					for (int i = 0; i < 16; i++)
						fbkmsg[i + 16] = BL_RTC_CmdMsg.MSG_BL_RTC_cmd.ext_data[i];
					continue;
				}
				else if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[0] == 3)
				{
					for (int i = 0; i < 16; i++)
						fbkmsg[i + 32] = BL_RTC_CmdMsg.MSG_BL_RTC_cmd.ext_data[i];
					continue;
				}
				else if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[0] == 4)
				{
					for (int i = 0; i < 16; i++)
						fbkmsg[i + 48] = BL_RTC_CmdMsg.MSG_BL_RTC_cmd.ext_data[i];
					continue;
				}
				else if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[0] == 0xff)
				{
					cout << "end of sst-sect-info:" << sectN << endl;
					res = 0;
					break;
				}
			}
			else
			{
				cout << "read sst-sect-info missing." << endl;
				res = -1;
				break;
			}
		}
		else
		{
			cout << "read sst-sect-info time-out" << endl;
			res = -2;
			break;
		}
	}

	if (res < 0)
	{
		for (int i = 0; i < sizeof(fbkmsg); i++)
			fbkmsg[i] = 0;
	}

	if (res == 0)
	{
		fbk_info_n->sect_chksum = (unsigned short)(0x00ff & *(pfbkmsg + pSectInfo_chksumL));
		fbk_info_n->sect_chksum |= (unsigned short)(0xff00 & *(pfbkmsg + pSectInfo_chksumH) << 8);

		fbk_info_n->sect_verinfo.RTC_Base = (int)(0x00ff & *(pfbkmsg + pSectInfo_rtcBase));
		fbk_info_n->sect_verinfo.RTC_Major = (int)(0x00ff & *(pfbkmsg + pSectInfo_rtcMajo));
		fbk_info_n->sect_verinfo.RTC_Minor = (int)(0x00ff & *(pfbkmsg + pSectInfo_rtcMino));

		fbk_info_n->sect_verinfo.MsgProt_Base = (int)(0x00ff & *(pfbkmsg + pSectInfo_protBase));
		fbk_info_n->sect_verinfo.MsgProt_Major = (int)(0x00ff & *(pfbkmsg + pSectInfo_protMajo));
		fbk_info_n->sect_verinfo.MsgProt_Minor = (int)(0x00ff & *(pfbkmsg + pSectInfo_protMino));

		fbk_info_n->sect_verinfo.BL_Base = (int)(0x00ff & *(pfbkmsg + pSectInfo_blBase));
		fbk_info_n->sect_verinfo.BL_Major = (int)(0x00ff & *(pfbkmsg + pSectInfo_blMajo));
		fbk_info_n->sect_verinfo.BL_Minor = (int)(0x00ff & *(pfbkmsg + pSectInfo_blMino));

		for (int i = 0; i < 48; i++)
			fbk_info_n->sect_msg[i] = fbkmsg[i + pSectInfo_blNote];
		/*
		cout<<"SectInfo-"<<std::dec<<sectN<<endl;
		cout<<"General-Segment chksum:0x"<<std::hex<<std::setw(4)<<setfill('0')<<fbk_info_n->sect_chksum<<endl;
		cout<<"PIC version number :"<<endl;
		cout<<"RTC     -Base: "<<std::dec<<fbk_info_n->sect_verinfo.RTC_Base<<",";
		cout<<"-Major: "       <<std::dec<<fbk_info_n->sect_verinfo.RTC_Major<<",";
		cout<<"-Minor: "       <<std::dec<<fbk_info_n->sect_verinfo.RTC_Minor<<endl;
		cout<<"MsgProt -Base: "<<std::dec<<fbk_info_n->sect_verinfo.MsgProt_Base<<",";
		cout<<"-Major: "       <<std::dec<<fbk_info_n->sect_verinfo.MsgProt_Major<<",";
		cout<<"-Minor: "       <<std::dec<<fbk_info_n->sect_verinfo.MsgProt_Minor<<endl;
		cout<<"BootLoad-Base: "<<std::dec<<fbk_info_n->sect_verinfo.BL_Base<<",";
		cout<<"-Major: "       <<std::dec<<fbk_info_n->sect_verinfo.BL_Major<<",";
		cout<<"-Minor: "       <<std::dec<<fbk_info_n->sect_verinfo.BL_Minor<<endl;
		//printf("sect-info msg:%s\n", fbk_info_n->sect_msg );
		string str_msg;
		str_msg.clear();
		str_msg.assign(fbk_info_n->sect_msg,0,48);

		string str_null;
		str_null.clear();
		for(int i=0;i<48;i++)
			str_null.push_back(0xff);
		str_null.push_back('\0');

		str_msg.push_back('\0');
		cout<<"sect-info msg size:"<<str_msg.size()<<endl;
		cout<<"sect-info msg:"<<str_msg<<endl;

		if(str_msg.compare(str_null) == 0)
			cout<<"sect-info msg is null"<<endl;
*/
	}

	if (res == -1)
		goDriverLogger.Log("warning", "read sst-sect-info data-missing");
	if (res == -2)
		goDriverLogger.Log("warning", "read sst-sect-info time-out");
	time_tag(tag_delta, &t_bl_rdsectinfo);
	printf("_Read_Note_SSTsect, res:%d, execute time:%dms. \n", res, t_bl_rdsectinfo.delta_tn);
	return res;
}

int bl_write_SectInfo_n(int sectN, string sour_sel)
{
	//	sectN    = 0~10
	//	sour_sel = 1:h16, 2:h32. -1,clear.
	//	negative:
	//		return -1: sectN > 10.
	//		return -2: source is empty.
	//		return -3: wrong command.
	//		return -4: rtc response time-out.
	//		return -5: rtc response data lose.
	//		return -6: rtc response data checksum error.
	//		return -7: sect not empty, need clr first.
	int show = 0;
	int res = 0;
	int sectinfo_fbk;
	timetag_t t_bl_wrsectinfo;
	time_tag(tag_all, &t_bl_wrsectinfo);
	unsigned char tx_extdata[20];
	unsigned char tx_cmddata[3];
	char txmsg[64];
	unsigned char txmsg_chksum;
	char *ptxmsg = txmsg;
	Poco::Event wait_10ms;

	if (sectN < 0 || sectN > 10)
		return -1;

	if (sour_sel == "h16")
	{
		if (h16_source.data_addr.empty())
		{
			cout << "inner h16_source is empty." << endl;
			return -2;
		}
	}
	else if (sour_sel == "h32")
	{
		if (h32_source.data_addr.empty())
		{
			cout << "inner h32_source is empty." << endl;
			return -2;
		}
	}
	else if (sour_sel == "clr")
	{
		cout << "clear buffer" << endl;
	}
	else
	{
		cout << "wrong sour_sel command" << endl;
		return -3;
	}

	//-----------------------------------------------------------------------------
	//  check empty first.
	SectInfo_n_t fbk_sectinfo_n;
	if (sour_sel == "h16" || sour_sel == "h32")
	{
		res = bl_read_SectInfo_n(sectN, &fbk_sectinfo_n);
		if (res == 0)
		{
			if (fbk_sectinfo_n.sect_chksum != 0xffff)
				return -7;
			if (fbk_sectinfo_n.sect_verinfo.RTC_Base != 0xff)
				return -7;
			if (fbk_sectinfo_n.sect_verinfo.RTC_Major != 0xff)
				return -7;
			if (fbk_sectinfo_n.sect_verinfo.RTC_Minor != 0xff)
				return -7;
			if (fbk_sectinfo_n.sect_verinfo.MsgProt_Base != 0xff)
				return -7;
			if (fbk_sectinfo_n.sect_verinfo.MsgProt_Major != 0xff)
				return -7;
			if (fbk_sectinfo_n.sect_verinfo.MsgProt_Minor != 0xff)
				return -7;
			if (fbk_sectinfo_n.sect_verinfo.BL_Base != 0xff)
				return -7;
			if (fbk_sectinfo_n.sect_verinfo.BL_Major != 0xff)
				return -7;
			if (fbk_sectinfo_n.sect_verinfo.BL_Minor != 0xff)
				return -7;
		}
	}

	//-----------------------------------------------------------------------------

	SectInfo_n_t inner_buf;

	if (sour_sel == "h16")
	{
		return -2;
	}
	else if (sour_sel == "h32")
	{
		//		read chksum.
		res = bl_Read_GenSegChksum_Hex32(&inner_buf.sect_chksum);
		if (res < 0)
			return -2;
		//		read version number.
		res = bl_read_h32_VersionNum(&inner_buf.sect_verinfo);
		if (res < 0)
			return -2;
		//		read message.
		res = bl_read_h32_BLmessage(inner_buf.sect_msg);
		if (res < 0)
			return -2;
	}
	else if (sour_sel == "clr")
	{
		inner_buf.sect_chksum = 0xffff;
		inner_buf.sect_verinfo.RTC_Base = -1;
		inner_buf.sect_verinfo.RTC_Major = -1;
		inner_buf.sect_verinfo.RTC_Minor = -1;
		inner_buf.sect_verinfo.MsgProt_Base = -1;
		inner_buf.sect_verinfo.MsgProt_Major = -1;
		inner_buf.sect_verinfo.MsgProt_Minor = -1;
		inner_buf.sect_verinfo.BL_Base = -1;
		inner_buf.sect_verinfo.BL_Major = -1;
		inner_buf.sect_verinfo.BL_Minor = -1;

		for (int i = 0; i < 48; i++)
			inner_buf.sect_msg[i] = 0xff;
	}

	if (res == 0 && show == 0)
	{
		cout << "SectInfo-" << std::dec << sectN << endl;
		if (inner_buf.sect_chksum != 0xffff)
			cout << "General segment chksum:0x" << std::hex << std::setw(4) << setfill('0') << inner_buf.sect_chksum << endl;
		else
			cout << "General segment chksum:----" << endl;
		cout << "ver-num(Base,Major,Minor): ";
		cout << "RTC: ";
		if (inner_buf.sect_verinfo.RTC_Base != 0xff)
			cout << std::dec << inner_buf.sect_verinfo.RTC_Base << ",";
		else
			cout << "--"
				 << ",";
		if (inner_buf.sect_verinfo.RTC_Major != 0xff)
			cout << std::dec << inner_buf.sect_verinfo.RTC_Major << ",";
		else
			cout << "--"
				 << ",";
		if (inner_buf.sect_verinfo.RTC_Minor != 0xff)
			cout << std::dec << inner_buf.sect_verinfo.RTC_Minor << "     ";
		else
			cout << "--"
				 << "     ";
		cout << "MsgProt: ";
		if (inner_buf.sect_verinfo.MsgProt_Base != 0xff)
			cout << std::dec << inner_buf.sect_verinfo.MsgProt_Base << ",";
		else
			cout << "--"
				 << ",";
		if (inner_buf.sect_verinfo.MsgProt_Major != 0xff)
			cout << std::dec << inner_buf.sect_verinfo.MsgProt_Major << ",";
		else
			cout << "--"
				 << ",";
		if (inner_buf.sect_verinfo.MsgProt_Minor != 0xff)
			cout << std::dec << inner_buf.sect_verinfo.MsgProt_Minor << "     ";
		else
			cout << "--"
				 << "     ";
		cout << "BootLoad: ";
		if (inner_buf.sect_verinfo.BL_Base != 0xff)
			cout << std::dec << inner_buf.sect_verinfo.BL_Base << ",";
		else
			cout << "--"
				 << ",";
		if (inner_buf.sect_verinfo.BL_Major != 0xff)
			cout << std::dec << inner_buf.sect_verinfo.BL_Major << ",";
		else
			cout << "--"
				 << ",";
		if (inner_buf.sect_verinfo.BL_Minor != 0xff)
			cout << std::dec << inner_buf.sect_verinfo.BL_Minor << "     ";
		else
			cout << "--"
				 << "     ";
		cout << endl;
		string str_msg, str_null;
		str_msg.clear();
		str_msg.assign(inner_buf.sect_msg, 0, 48);
		str_msg.push_back('\0');

		str_null.clear();
		for (int i = 0; i < 48; i++)
			str_null.push_back(0xff);
		str_null.push_back('\0');

		if (str_msg.compare(str_null) == 0)
			cout << "sect-info msg:---" << endl;
		else
			cout << "sect-info msg:" << str_msg << endl;
	}
	*(ptxmsg + pSectInfo_chksumL) = (unsigned char)(0x00ff & inner_buf.sect_chksum);
	*(ptxmsg + pSectInfo_chksumH) = (unsigned char)(0x00ff & inner_buf.sect_chksum >> 8);
	*(ptxmsg + pSectInfo_rtcBase) = (unsigned char)(0x00ff & inner_buf.sect_verinfo.RTC_Base);
	*(ptxmsg + pSectInfo_rtcMajo) = (unsigned char)(0x00ff & inner_buf.sect_verinfo.RTC_Major);
	*(ptxmsg + pSectInfo_rtcMino) = (unsigned char)(0x00ff & inner_buf.sect_verinfo.RTC_Minor);
	*(ptxmsg + pSectInfo_protBase) = (unsigned char)(0x00ff & inner_buf.sect_verinfo.MsgProt_Base);
	*(ptxmsg + pSectInfo_protMajo) = (unsigned char)(0x00ff & inner_buf.sect_verinfo.MsgProt_Major);
	*(ptxmsg + pSectInfo_protMino) = (unsigned char)(0x00ff & inner_buf.sect_verinfo.MsgProt_Minor);
	*(ptxmsg + pSectInfo_blBase) = (unsigned char)(0x00ff & inner_buf.sect_verinfo.BL_Base);
	*(ptxmsg + pSectInfo_blMajo) = (unsigned char)(0x00ff & inner_buf.sect_verinfo.BL_Major);
	*(ptxmsg + pSectInfo_blMino) = (unsigned char)(0x00ff & inner_buf.sect_verinfo.BL_Minor);
	*(ptxmsg + pSectInfo_null01) = 0xff;
	*(ptxmsg + pSectInfo_null02) = 0xff;
	*(ptxmsg + pSectInfo_null03) = 0xff;
	*(ptxmsg + pSectInfo_null04) = 0xff;
	*(ptxmsg + pSectInfo_null05) = 0xff;

	for (int i = 0; i < 48; i++)
		*(ptxmsg + pSectInfo_blNote + i) = inner_buf.sect_msg[i];
	//	for( int i= 0;i<sizeof(txmsg);i++ )
	//		printf( " %02X",*(ptxmsg+i) );
	//	printf("\n");

	for (int i = 0; i < 3; i++)
		tx_cmddata[i] = 0;
	//	tx_cmddata[0] = sectN.
	//	tx_cmddata[1] = tx-sequnce. 0xff = end
	tx_cmddata[0] = 0xff & (unsigned char)sectN;

	//  seq-0.
	tx_cmddata[1] = 0x00;
	txmsg_chksum = 0;
	BL_RTC_CmdMsg.wait_cmdfbk.reset();
	BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd = _Null;
	for (int i = 0; i < 16; i++)
		tx_extdata[i] = 0;
	// printf( "txmsg_chksum:%02X \n",txmsg_chksum );
	Msg_Prot_BL_Host_to_RTC_encodeCmd(_Write_SectInfo_n, tx_cmddata, tx_extdata);
	wait_10ms.tryWait(10);
	printf("seq-0\n");

	//  seq-1.
	tx_cmddata[1] = 1;
	for (int i = 0; i < 16; i++)
	{
		tx_extdata[i] = *(ptxmsg + i);
		txmsg_chksum += *(ptxmsg + i);
		// printf( "%02X ",tx_extdata[i] );
	}
	// printf( "txmsg_chksum-1:%02X \n",txmsg_chksum );
	Msg_Prot_BL_Host_to_RTC_encodeCmd(_Write_SectInfo_n, tx_cmddata, tx_extdata);
	wait_10ms.tryWait(10);
	printf("seq-1\n");

	//  seq-2.
	tx_cmddata[1] = 2;
	for (int i = 0; i < 16; i++)
	{
		tx_extdata[i] = *(ptxmsg + i + 16);
		txmsg_chksum += *(ptxmsg + i + 16);
		// printf( "%02X ",tx_extdata[i] );
	}
	// printf( "txmsg_chksum-2:%02X \n",txmsg_chksum );
	Msg_Prot_BL_Host_to_RTC_encodeCmd(_Write_SectInfo_n, tx_cmddata, tx_extdata);
	wait_10ms.tryWait(10);
	printf("seq-2\n");

	//  seq-3.
	tx_cmddata[1] = 3;
	for (int i = 0; i < 16; i++)
	{
		tx_extdata[i] = *(ptxmsg + i + 32);
		txmsg_chksum += *(ptxmsg + i + 32);
		// printf( "%02X ",tx_extdata[i] );
	}
	// printf( "txmsg_chksum-3:%02X \n",txmsg_chksum );
	Msg_Prot_BL_Host_to_RTC_encodeCmd(_Write_SectInfo_n, tx_cmddata, tx_extdata);
	wait_10ms.tryWait(10);
	printf("seq-3\n");

	//  seq-4.
	tx_cmddata[1] = 4;
	for (int i = 0; i < 16; i++)
	{
		tx_extdata[i] = *(ptxmsg + i + 48);
		txmsg_chksum += *(ptxmsg + i + 48);
		// printf( "%02X ",tx_extdata[i] );
	}
	// printf( "txmsg_chksum-4:%02X \n",txmsg_chksum );
	Msg_Prot_BL_Host_to_RTC_encodeCmd(_Write_SectInfo_n, tx_cmddata, tx_extdata);
	wait_10ms.tryWait(10);
	printf("seq-4\n");

	//  seq-5.
	tx_cmddata[1] = 0xff;
	tx_cmddata[2] = 0xff - txmsg_chksum + 1;
	// printf( "tx_chksum:%02X, txmsg_chksum:%02X \n",tx_cmddata[2],txmsg_chksum );
	BL_RTC_CmdMsg.wait_cmdfbk.reset();
	BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd = _Null;
	for (int i = 0; i < 16; i++)
		tx_extdata[i] = 0;
	Msg_Prot_BL_Host_to_RTC_encodeCmd(_Write_SectInfo_n, tx_cmddata, tx_extdata);
	wait_10ms.tryWait(10);
	printf("seq-5\n");

	sectinfo_fbk = BL_RTC_CmdMsg.wait_cmdfbk.tryWait(100);
	if (sectinfo_fbk == 1)
	{
		printf("fbk bl_cmd:%02x\n", BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd);
		printf("fbk cmd_data[0]:%02x\n", BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[0]);
		if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd == _Write_SectInfo_n && BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[0] == _positivResp)
		{
			res = 0;
			cout << "write sst-sect-info completed" << endl;
		}
		else if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd == _Write_SectInfo_n && BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[0] == _generalReject)
		{
			res = -6;
			goDriverLogger.Log("warning", "write sst-sect-info chksum error");
		}
		else
		{
			res = -5;
			goDriverLogger.Log("warning", "write sst-sect-info known error");
		}
	}
	else
	{
		cout << "write sst-sect-info feedback time-out" << endl;
		goDriverLogger.Log("warning", "write sst-sect-info feedback time-out");
		res = -4;
	}

	time_tag(tag_delta, &t_bl_wrsectinfo);
	printf("_Write_SectInfo_n, res:%d, execute time:%dms. \n", res, t_bl_wrsectinfo.delta_tn);
	return res;
}

int bl_read_SectInfo_All(void)
{
	int res;
	int i;
	Poco::Event wait_50ms;
	Vec_SectInfo.clear();
	SectInfo_n_t fbk_sectinfo_n;
	timetag_t t_rd_all_info;
	time_tag(tag_all, &t_rd_all_info);

	for (i = 0; i < 11; i++)
	{
		res = bl_read_SectInfo_n(i, &fbk_sectinfo_n);

		if (res == 0)
			Vec_SectInfo.push_back(fbk_sectinfo_n);
		else
			break;

		wait_50ms.tryWait(50);
	}

	if (res == 0)
		cout << "read SectInfo Okay" << endl;
	else
	{
		Vec_SectInfo.clear();
		cout << "read SectInfo Stop & clr_vecSectInfo:" << res << ",i:" << i << endl;
	}

	time_tag(tag_delta, &t_rd_all_info);
	printf("read all sect-info, execute time:%dms. \n", t_rd_all_info.delta_tn);
	return res;
}

int bl_show_SectInfo_All(void)
{
	int res;
	string str_msg, str_null;

	if (Vec_SectInfo.empty())
	{
		cout << "Vec_SectInfo empty" << endl;
		res = -1;
		return res;
	}

	str_null.clear();
	for (int i = 0; i < 48; i++)
		str_null.push_back(0xff);
	str_null.push_back('\0');

	cout << "sect-info size:" << Vec_SectInfo.size() << endl
		 << endl;

	cout << "|" << std::setw(6) << "SectNo";
	cout << "|" << std::setw(7) << "chksum";
	cout << std::setw(2) << setfill(' ') << "|";
	cout << std::setw(11) << " RTC-ver" << std::setw(2) << setfill(' ') << "|";
	cout << std::setw(11) << "Prot-ver" << std::setw(2) << setfill(' ') << "|";
	cout << std::setw(11) << "  BL-ver" << std::setw(2) << setfill(' ') << "|";
	cout << "  note  ";
	cout << endl;
	for (int i = 0; i < Vec_SectInfo.size(); i++)
	{
		cout << "|" << std::setw(3) << setfill(' ') << i << std::setw(4) << setfill(' ') << "|";
		if (Vec_SectInfo[i].sect_chksum != 0xffff)
			cout << " 0x" << std::hex << std::setw(4) << setfill('0') << Vec_SectInfo[i].sect_chksum;
		else
			cout << std::hex << std::setw(7) << setfill(' ') << "------";
		cout << std::setw(2) << setfill(' ') << "|";

		if (Vec_SectInfo[i].sect_verinfo.RTC_Base != 0xff)
			cout << std::dec << std::setw(3) << setfill(' ') << Vec_SectInfo[i].sect_verinfo.RTC_Base;
		else
			cout << std::dec << std::setw(3) << setfill(' ') << "--";
		cout << " ";
		if (Vec_SectInfo[i].sect_verinfo.RTC_Major != 0xff)
			cout << std::dec << std::setw(3) << setfill(' ') << Vec_SectInfo[i].sect_verinfo.RTC_Major;
		else
			cout << std::dec << std::setw(3) << setfill(' ') << "--";
		cout << " ";
		if (Vec_SectInfo[i].sect_verinfo.RTC_Minor != 0xff)
			cout << std::dec << std::setw(3) << setfill(' ') << Vec_SectInfo[i].sect_verinfo.RTC_Minor;
		else
			cout << std::dec << std::setw(3) << setfill(' ') << "--";
		cout << std::setw(2) << setfill(' ') << "|";

		if (Vec_SectInfo[i].sect_verinfo.MsgProt_Base != 0xff)
			cout << std::dec << std::setw(3) << setfill(' ') << Vec_SectInfo[i].sect_verinfo.MsgProt_Base;
		else
			cout << std::dec << std::setw(3) << setfill(' ') << "--";
		cout << " ";
		if (Vec_SectInfo[i].sect_verinfo.MsgProt_Major != 0xff)
			cout << std::dec << std::setw(3) << setfill(' ') << Vec_SectInfo[i].sect_verinfo.MsgProt_Major;
		else
			cout << std::dec << std::setw(3) << setfill(' ') << "--";
		cout << " ";
		if (Vec_SectInfo[i].sect_verinfo.MsgProt_Minor != 0xff)
			cout << std::dec << std::setw(3) << setfill(' ') << Vec_SectInfo[i].sect_verinfo.MsgProt_Minor;
		else
			cout << std::dec << std::setw(3) << setfill(' ') << "--";
		cout << std::setw(2) << setfill(' ') << "|";

		if (Vec_SectInfo[i].sect_verinfo.BL_Base != 0xff)
			cout << std::dec << std::setw(3) << setfill(' ') << Vec_SectInfo[i].sect_verinfo.BL_Base;
		else
			cout << std::dec << std::setw(3) << setfill(' ') << "--";
		cout << " ";
		if (Vec_SectInfo[i].sect_verinfo.BL_Major != 0xff)
			cout << std::dec << std::setw(3) << setfill(' ') << Vec_SectInfo[i].sect_verinfo.BL_Major;
		else
			cout << std::dec << std::setw(3) << setfill(' ') << "--";
		cout << " ";
		if (Vec_SectInfo[i].sect_verinfo.BL_Minor != 0xff)
			cout << std::dec << std::setw(3) << setfill(' ') << Vec_SectInfo[i].sect_verinfo.BL_Minor;
		else
			cout << std::dec << std::setw(3) << setfill(' ') << "--";
		cout << std::dec << std::setw(2) << setfill(' ') << "|";

		str_msg.clear();
		str_msg.assign(Vec_SectInfo[i].sect_msg, 0, 48);
		str_msg.push_back('\0');

		if (str_msg.compare(str_null) == 0)
			cout << std::dec << std::setw(3) << setfill(' ') << "(null)";
		else
			cout << std::dec << std::setw(3) << setfill(' ') << str_msg;

		cout << endl;
	}
	return 0;
}

int bl_read_SectBoot(SectBootInfo_t *fbk_sectboot)
{
	// to read which SST-section(0-10) re-flash to PIC general-segment.
	// fbk_sectboot = 0~10

	// chksum check: [self_chksum + sour_chksum] 1's comp
	// result = ff - [self + sour ] + 1 = 0.
	// the result must equal to 0.
	int res = 0;
	int sectboot_fbk;
	timetag_t t_bl_rdsectboot;
	time_tag(tag_all, &t_bl_rdsectboot);

	SectBootInfo_t fbk_msg;
	unsigned char chksum_sum;
	unsigned char chksum_cal;

	BL_RTC_CmdMsg.wait_cmdfbk.reset();
	BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd = _Null;
	Msg_Prot_BL_Host_to_RTC_encodeCmd(_Read_SectBoot, 0, 0);

	sectboot_fbk = BL_RTC_CmdMsg.wait_cmdfbk.tryWait(50);
	if (sectboot_fbk == 1)
	{
		if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd == _Read_SectBoot)
		{
			fbk_msg.sect_num = BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[0];
			fbk_msg.s_chksum = BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[1];
			chksum_cal = fbk_msg.sect_num;
			chksum_sum = 0xff - (chksum_cal + fbk_msg.s_chksum) + 1;
			// printf("rx sect_boot: 0x%02x, rx_chksum: 0x%02x\n",fbk_msg.sect_num,fbk_msg.s_chksum);
			// printf("re-checksum : 0x%02x\n",chksum_sum);

			if (chksum_sum > 0)
				res = -3;
		}
		else
			res = -2;
	}
	else
		res = -1;

	*fbk_sectboot = fbk_msg;

	if (res == -1)
		goDriverLogger.Log("warning", "read SectBoot time-out");
	if (res == -2)
		goDriverLogger.Log("warning", "read SectBoot data-missing");
	if (res == -3)
		goDriverLogger.Log("warning", "read SectBoot chksum error");
	time_tag(tag_delta, &t_bl_rdsectboot);
	printf("_Read_Note_SSTsect, res:%d, execute time:%dms. \n", res, t_bl_rdsectboot.delta_tn);
	return res;
}

int bl_write_SectBoot(int sectN)
{
	int res;
	int wrsb_fbk;

	if (sectN > 10 || sectN < 0)
	{
		cout << "out of range" << endl;
		return -1;
	}
	unsigned char cmd_data[3];
	cmd_data[0] = sectN;
	cmd_data[1] = 0xff - sectN + 1; // chksum;
	cmd_data[2] = 0xff;
	printf("cmd_data[0]:0x%02x, cmd_data[1]:0x%02x, ", cmd_data[0], cmd_data[1]);
	timetag_t t_bl_wr_sectboot;
	time_tag(tag_all, &t_bl_wr_sectboot);

	BL_RTC_CmdMsg.wait_cmdfbk.reset();
	BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd = _Null;
	Msg_Prot_BL_Host_to_RTC_encodeCmd(_Write_SectBoot, cmd_data, 0);

	wrsb_fbk = BL_RTC_CmdMsg.wait_cmdfbk.tryWait(50);
	if (wrsb_fbk == 1)
	{
		if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[0] == _positivResp)
			res = 0;
		else
			res = -2;
	}
	else
		res = -1;

	if (res == -1)
		goDriverLogger.Log("warning", "write SectBoot time-out");
	if (res == -2)
		goDriverLogger.Log("warning", "write SectBoot chksum error");

	res = 0;
	return res;
}

int bl_dsPIC_EraseAux(void)
{ // addr range: 000000H - 7FFFFFH.
	int res;
	Msg_Prot_BL_Host_to_RTC_encodeCmd(_dsPIC_Erase_AuxSeg, 0, 0);
	return res;
}

int bl_dsPIC_AuxSegFlash(void)
{ // addr range: 000000H - 7FFFFFH.
	int res;
	Msg_Prot_BL_Host_to_RTC_encodeCmd(_dsPIC_AuxSegFlash, 0, 0);
	return res;
}

int bl_set_ConfigBit_RSTPRI(int seg_sel, unsigned char *ConfigBit)
{
	int res = 0;
	int ConfigBit_fbk;
	unsigned char FICD_res;
	unsigned char tx_cmddata[3];

	for (int i = 0; i < 3; i++)
		tx_cmddata[i] = 0;

	if (seg_sel > 0)
		tx_cmddata[0] = 1;

	timetag_t t_bl_setConfigBit;
	time_tag(tag_all, &t_bl_setConfigBit);

	BL_RTC_CmdMsg.wait_cmdfbk.reset();
	Msg_Prot_BL_Host_to_RTC_encodeCmd(_ConfigBit_RSTPRI, tx_cmddata, 0);

	while (1)
	{
		ConfigBit_fbk = BL_RTC_CmdMsg.wait_cmdfbk.tryWait(50);
		if (ConfigBit_fbk == 1)
		{
			if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd == _ConfigBit_RSTPRI)
			{
				FICD_res = BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[0];
				*ConfigBit = FICD_res;
				res = 0;
			}
			else
				res = -1;

			break;
		}
		else
		{
			res = -2;
			break;
		}
	}

	if (res == -1)
		goDriverLogger.Log("warning", "set_ConfigBit_RSTPRI response fault");
	if (res == -2)
		goDriverLogger.Log("warning", "set_ConfigBit_RSTPRI time-out");

	time_tag(tag_delta, &t_bl_setConfigBit);
	printf("Set ConfigBit_RSTPRI, res:%d, execute time:%dms. \n", res, t_bl_setConfigBit.delta_tn);
	printf("ConfigBit_RSTPRI res: 0x%2x, located: bit 2, [Aux:Pri]=0:1, res:%s\n", FICD_res, ((FICD_res >> 2) & 0x01) ? "Pri Flash" : "Aux Flash");
	return res;
}
/*
int bl_dsPIC_setSECT( int section )
{
	int res = 0;
	unsigned char data_size = 1;
	unsigned char data[data_size];

	const int sect_0  = 0;
	const int sect_10 = 10;

	if( section < sect_0 )
		return -1;
	else if( section > sect_10 )
		return -2;
	else
	{
		data[0] = (unsigned char) ( section & 0xff );
	//	res = Msg_Prot_BL_Host_to_RTC_encodeCmd( _dsPIC_SetFlash_SECT, data_size, data );
	}

	return res;
}
*/

//-----------------------------------------------------------------------------
int Msg_BL_fromRTC_CMD(ONS_BYTE *Msg_Buff)
{
	int resp = 0;
	Msg_Prot_BL_fromRTC_Command_decode(Msg_Buff, &BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd, BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data, BL_RTC_CmdMsg.MSG_BL_RTC_cmd.ext_data);

	//-------------------------------------------------------------------------
	if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd == _dsPIC_Erase_AuxSeg)
		cout << "dsPIC AuxFlash Cleared." << endl;
	//-------------------------------------------------------------------------
	if (BL_RTC_CmdMsg.MSG_BL_RTC_cmd.bl_cmd == _dsPIC_AuxSegFlash)
	{
		cout << "dsPIC AuxFlash reflahsed." << endl;
		int aux_writeCount;
		aux_writeCount = (int)BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[1] & 0x00ff;
		aux_writeCount |= ((int)BL_RTC_CmdMsg.MSG_BL_RTC_cmd.cmd_data[2] << 8) & 0xff00;
		cout << "aux_writeCount:" << aux_writeCount << endl;
	}
	//-------------------------------------------------------------------------
	BL_RTC_CmdMsg.wait_cmdfbk.set();
	return resp;
}

int Msg_Prot_BL_fromRTC_Command_decode(ONS_BYTE *Msg_Buff, unsigned char *bl_cmd, unsigned char *cmd_data, unsigned char *ext_data)
{
	int res = 0;
	Msg_Prot_BL_fromRTC_CMD_t *msg_p;
	msg_p = (Msg_Prot_BL_fromRTC_CMD_t *)Msg_Buff;
	*bl_cmd = msg_p->bl_cmd;

	cmd_data[0] = msg_p->cmd_data[0];
	cmd_data[1] = msg_p->cmd_data[1];
	cmd_data[2] = msg_p->cmd_data[2];

	for (int i = 0; i < 20; i++)
		ext_data[i] = msg_p->ext_data[i];
	return res;
}

// bl_RTC_mem_16_t   BL_RTC_MemRead16;
// bl_RTC_mem_16_t   BL_RTC_MemRead32;
// bl_RTC_mem_echo_t BL_RTC_MemEcho;
int Msg_BL_fromRTC_MEM(ONS_BYTE *Msg_Buff)
{
	int resp = 0;
	unsigned char mem_type;
	Msg_Prot_BL_fromRTC_MEMtype_t *msg_p;
	msg_p = (Msg_Prot_BL_fromRTC_MEMtype_t *)Msg_Buff;

	mem_type = msg_p->type;

	if (mem_type == _RTCr_READ_PIC16 || mem_type == _RTCr_READ_SST16)
	{
		Msg_Prot_BL_fromRTC_MEM16read_decode(Msg_Buff, &BL_RTC_MemRead16);
	}
	else if (mem_type == _RTCr_READ_PIC32 || mem_type == _RTCr_READ_SST32)
	{
		Msg_Prot_BL_fromRTC_MEM32read_decode(Msg_Buff, &BL_RTC_MemRead32);
	}
	else if (mem_type == _RTCr_WRIT_SST16 || mem_type == _RTCr_WRIT_SST32 || mem_type == _RTCr_WRIT_SST48)
	{
		Msg_Prot_BL_fromRTC_MEMecho_decode(Msg_Buff, &BL_RTC_MemEcho);
	}

	return resp;
}
int Msg_Prot_BL_fromRTC_MEM16read_decode(ONS_BYTE *Msg_Buff, bl_RTC_mem_16_t *memread16)
{
	int res = 0;
	Msg_Prot_BL_fromRTC_MEMread_t *msg_p;
	msg_p = (Msg_Prot_BL_fromRTC_MEMread_t *)Msg_Buff;

	memread16->MSG_BL_RTC_mem.type = msg_p->type;
	memread16->MSG_BL_RTC_mem.addr[0] = msg_p->addr[0];
	memread16->MSG_BL_RTC_mem.addr[1] = msg_p->addr[1];
	memread16->MSG_BL_RTC_mem.addr[2] = msg_p->addr[2];

	for (int i = 0; i < 16; i++)
		memread16->MSG_BL_RTC_mem.data[i] = msg_p->data[i];

	BL_RTC_MemRead16.wait_mem16fbk.set();
	return res;
}
int Msg_Prot_BL_fromRTC_MEM32read_decode(ONS_BYTE *Msg_Buff, bl_RTC_mem_32_t *memread32)
{
	int res = 0;
	Msg_Prot_BL_fromRTC_MEMread_t *msg_p;
	msg_p = (Msg_Prot_BL_fromRTC_MEMread_t *)Msg_Buff;

	memread32->MSG_BL_RTC_mem.type = msg_p->type;
	memread32->MSG_BL_RTC_mem.addr[0] = msg_p->addr[0];
	memread32->MSG_BL_RTC_mem.addr[1] = msg_p->addr[1];
	memread32->MSG_BL_RTC_mem.addr[2] = msg_p->addr[2];

	for (int i = 0; i < 32; i++)
		memread32->MSG_BL_RTC_mem.data[i] = msg_p->data[i];
	BL_RTC_MemRead32.wait_mem32fbk.set();
	return res;
}

int Msg_Prot_BL_fromRTC_MEMecho_decode(ONS_BYTE *Msg_Buff, bl_RTC_mem_echo_t *memecho)
{
	int res = 0;
	Msg_Prot_BL_fromRTC_MEMecho_t *msg_p;
	msg_p = (Msg_Prot_BL_fromRTC_MEMecho_t *)Msg_Buff;

	memecho->MSG_BL_RTC_mem.type = msg_p->type;
	memecho->MSG_BL_RTC_mem.chksum = msg_p->chksum;
	memecho->MSG_BL_RTC_mem.chksumH = msg_p->chksumH;
	BL_RTC_MemEcho.wait_memechofbk.set();
	return res;
}

//-----------------------------------------------------------------------------
int Msg_Prot_BL_Host_to_RTC_encodeCmd(unsigned char bl_cmd, unsigned char *cmd_data, unsigned char *ext_data)
{
	int res = 0;
	int i = 0;
	Msg_Prot_BL_ToRTC_CMD_t bl_msg_toRTC;
	bl_msg_toRTC.Id = MSG_PROT_BL_CmdReq;
	bl_msg_toRTC.null_size = 0xaaaa;
	bl_msg_toRTC.null_sn = 0xbbbbbbbb;
	bl_msg_toRTC.null_ts = 0xcccccccc;

	bl_msg_toRTC.bl_cmd = bl_cmd;
	for (i = 0; i < sizeof(bl_msg_toRTC.cmd_data); i++)
		bl_msg_toRTC.cmd_data[i] = 0;
	for (i = 0; i < sizeof(bl_msg_toRTC.ext_data); i++)
		bl_msg_toRTC.ext_data[i] = 0;

	//========================
	// printf("TxCmd:%31s \n",  (bl_msg_toRTC.bl_cmd == _NOP) ? "_NOP" : "");
	if (bl_msg_toRTC.bl_cmd == _NOP)
		printf("TxCmd:%6s \n", "_NOP");
	else if (bl_msg_toRTC.bl_cmd == _BL_INIT)
		printf("TxCmd:%8s \n", "_BL_INIT");
	else if (bl_msg_toRTC.bl_cmd == _ResetEn)
		printf("TxCmd:%8s \n", "_ResetEn");
	else if (bl_msg_toRTC.bl_cmd == _Reset)
		printf("TxCmd:%8s \n", "_Reset");
	else if (bl_msg_toRTC.bl_cmd == _Read_Status_Reg)
		printf("TxCmd:%12s \n", "_Read_Status");
	else if (bl_msg_toRTC.bl_cmd == _Write_Status_Reg)
		printf("TxCmd:%13s \n", "_Write_Status");
	else if (bl_msg_toRTC.bl_cmd == _Read_Configuration_Reg)
		printf("TxCmd:%15s \n", "_Read_ConfigReg");
	else if (bl_msg_toRTC.bl_cmd == _Read)
		printf("TxCmd:%5s \n", "_Read");
	else if (bl_msg_toRTC.bl_cmd == _HighSpeed_Read)
		printf("TxCmd:%15s \n", "_HighSpeed_Read");
	else if (bl_msg_toRTC.bl_cmd == _Read_Cont)
		printf("TxCmd:%10s \n", "_Read_Cont");
	else if (bl_msg_toRTC.bl_cmd == _Jedec_ID_Read)
		printf("TxCmd:%15s \n", "_Jedec_ID_Read");
	else if (bl_msg_toRTC.bl_cmd == _SFDP_Read)
		printf("TxCmd:%10s \n", "_SFDP_Read");
	else if (bl_msg_toRTC.bl_cmd == _WREN_SST26V)
		printf("TxCmd:%5s \n", "_WREN_SST26V");
	else if (bl_msg_toRTC.bl_cmd == _WRDI)
		printf("TxCmd:%5s \n", "_WRDI");
	else if (bl_msg_toRTC.bl_cmd == _Sector_Erase)
	{
		printf("TxCmd:%13s \n", "_Sector_Erase");
		for (i = 0; i < 3; i++)
			bl_msg_toRTC.cmd_data[i] = cmd_data[i];
	}
	else if (bl_msg_toRTC.bl_cmd == _Block_Erase)
	{
		printf("TxCmd:%13s \n", "_Block_Erase");
		for (i = 0; i < 3; i++)
			bl_msg_toRTC.cmd_data[i] = cmd_data[i];
	}
	else if (bl_msg_toRTC.bl_cmd == _Chip_Erase)
		printf("TxCmd:%13s \n", "_Chip_Erase");
	else if (bl_msg_toRTC.bl_cmd == _Page_Program)
		printf("TxCmd:%13s \n", "_Page_Program");
	else if (bl_msg_toRTC.bl_cmd == _Write_Suspend)
		printf("TxCmd:%15s \n", "_Write_Suspend");
	else if (bl_msg_toRTC.bl_cmd == _Write_Resume)
		printf("TxCmd:%13s \n", "_Write_Resume");
	else if (bl_msg_toRTC.bl_cmd == _ReadBlockProtection)
		printf("TxCmd:%14s \n", "_ReadBlockProt");
	else if (bl_msg_toRTC.bl_cmd == _WriteBlockProtection)
	{
		printf("TxCmd:%15s \n", "_WriteBlockProt");
		for (i = 0; i < 18; i++)
			bl_msg_toRTC.ext_data[i] = ext_data[i];
	}
	else if (bl_msg_toRTC.bl_cmd == _LockBlockProtection)
		printf("TxCmd:%16s \n", "LockBlockProtect");
	else if (bl_msg_toRTC.bl_cmd == _NonVolWriteLockProtection)
		printf("TxCmd:%20s \n", "_NonVolWriteLockProt");
	else if (bl_msg_toRTC.bl_cmd == _Global_Block_Protection_Unlock)
		printf("TxCmd:%25s \n", "GlobalBlockProtect_Unlock");
	else if (bl_msg_toRTC.bl_cmd == _Global_Block_Protection_Unlock)
		printf("TxCmd:%25s \n", "GlobalBlockProtect_Unlock");
	else if (bl_msg_toRTC.bl_cmd == _ReadSID)
		printf("TxCmd:%8s \n", "_ReadSID");
	else if (bl_msg_toRTC.bl_cmd == _ProgSID)
		printf("TxCmd:%8s \n", "_ProgSID");
	else if (bl_msg_toRTC.bl_cmd == _LockSID)
		printf("TxCmd:%8s \n", "_LockSID");
	else if (bl_msg_toRTC.bl_cmd == _Read_Chksum_PIC)
	{
		printf("TxCmd:%16s \n", "_Read_Chksum_PIC");
		for (i = 0; i < 3; i++)
			bl_msg_toRTC.cmd_data[i] = cmd_data[i];
	}
	else if (bl_msg_toRTC.bl_cmd == _Read_Chksum_SSTsect)
	{
		printf("TxCmd:%16s \n", "_Read_Chksum_SSTsect");
		for (i = 0; i < 3; i++)
			bl_msg_toRTC.cmd_data[i] = cmd_data[i];
	}
	else if (bl_msg_toRTC.bl_cmd == _RESET_PIC)
		printf("TxCmd:%11s \n", "_RESET_PIC");
	else if (bl_msg_toRTC.bl_cmd == _RESET_SST26V)
		printf("TxCmd:%11s \n", "_RESET_SST26V");
	else if (bl_msg_toRTC.bl_cmd == _gotoAUX)
		printf("TxCmd:%11s \n", "_gotoAUX");
	else if (bl_msg_toRTC.bl_cmd == _Backup_GenSeg_SSTsect)
	{
		printf("TxCmd:%11s \n", "_Backup_GenSeg_SSTsect");
		for (i = 0; i < 3; i++)
			bl_msg_toRTC.cmd_data[i] = cmd_data[i];
	}
	else if (bl_msg_toRTC.bl_cmd == _Read_Version_PIC)
		printf("TxCmd:%11s \n", "_Read_Version_PIC");
	else if (bl_msg_toRTC.bl_cmd == _Read_Note_PIC)
		printf("TxCmd:%11s \n", "_Read_Note_PIC");
	else if (bl_msg_toRTC.bl_cmd == _Read_Version_SSTsect)
	{
		printf("TxCmd:%11s \n", "_Read_Version_SSTsect");
		for (i = 0; i < 3; i++)
			bl_msg_toRTC.cmd_data[i] = cmd_data[i];
	}
	else if (bl_msg_toRTC.bl_cmd == _Read_Note_SSTsect)
	{
		printf("TxCmd:%11s \n", "_Read_Note_SSTsect");
		for (i = 0; i < 3; i++)
			bl_msg_toRTC.cmd_data[i] = cmd_data[i];
	}
	else if (bl_msg_toRTC.bl_cmd == _Read_SectInfo_n)
	{
		printf("TxCmd:%11s \n", "_Read_SectInfo_n");
		for (i = 0; i < 3; i++)
			bl_msg_toRTC.cmd_data[i] = cmd_data[i];
	}
	else if (bl_msg_toRTC.bl_cmd == _Write_SectInfo_n)
	{
		printf("TxCmd:%11s \n", "_Write_SectInfo_n");
		for (i = 0; i < 3; i++)
			bl_msg_toRTC.cmd_data[i] = cmd_data[i];
		for (i = 0; i < 18; i++)
			bl_msg_toRTC.ext_data[i] = ext_data[i];
	}
	else if (bl_msg_toRTC.bl_cmd == _Read_SectBoot)
		printf("TxCmd:%11s \n", "_Read_SectBoot");
	else if (bl_msg_toRTC.bl_cmd == _Write_SectBoot)
	{
		printf("TxCmd:%11s \n", "_Write_SectBoot");
		for (i = 0; i < 3; i++)
			bl_msg_toRTC.cmd_data[i] = cmd_data[i];
	}
	else if (bl_msg_toRTC.bl_cmd == _ConfigBit_RSTPRI)
	{
		printf("TxCmd:%11s \n", "_ConfigBit_RSTPRI");
		for (i = 0; i < 3; i++)
			bl_msg_toRTC.cmd_data[i] = cmd_data[i];
	}

	//=========================================================================
	int tx_size = 36;
	res = USBComm_Send_Message((USBComm_Buffer_t)&bl_msg_toRTC, tx_size);
	return res;
}

//-----------------------------------------------------------------------------
int Msg_Prot_BL_Host_to_RTC_encodeMEMRead(memCtrl_enum_t mem_ctrl, int addr) // vector<unsigned char> *data
{
	int res = 0;
	Msg_Prot_BL_ToRTC_MEM_t bl_txmsg;
	bl_txmsg.Id = MSG_PROT_BL_DataReq;
	bl_txmsg.null_size = 0xaa55;
	bl_txmsg.null_sn = 0xbbbb4444;
	bl_txmsg.null_ts = 0xcccc3333;
	bl_txmsg.type = (unsigned char)mem_ctrl;
	bl_txmsg.addr[0] = addr & 0xff;
	bl_txmsg.addr[1] = (addr >> 8) & 0xff;
	bl_txmsg.addr[2] = (addr >> 16) & 0xff;

	printf("ReqAdd0:%04X \n", bl_txmsg.addr[0]);
	printf("ReqAdd1:%04X \n", bl_txmsg.addr[1]);
	printf("ReqAdd2:%04X \n", bl_txmsg.addr[2]);
	res = USBComm_Send_Message((USBComm_Buffer_t)&bl_txmsg, 16);
	return res;
}

int Msg_Prot_BL_Host_to_RTC_encodeMEMWrite(unsigned char mem_ctrl, int addr, unsigned char *data) // vector<unsigned char> *data
{
	int res = 0;
	Msg_Prot_BL_ToRTC_MEM_t bl_txmsg;
	bl_txmsg.Id = MSG_PROT_BL_DataReq;
	bl_txmsg.null_size = 0xaa55;
	bl_txmsg.null_sn = 0xbbbb4444;
	bl_txmsg.null_ts = 0xcccc3333;
	bl_txmsg.type = (unsigned char)mem_ctrl;
	bl_txmsg.addr[0] = addr & 0xff;
	bl_txmsg.addr[1] = (addr >> 8) & 0xff;
	bl_txmsg.addr[2] = (addr >> 16) & 0xff;

	if (mem_ctrl == _HOST_WRIT_SST16)
	{ // cout<<"tx_HOST_WRIT_SST16"<<endl;
		for (int i = 0; i < 16; i++)
			bl_txmsg.data[i] = data[i];
		res = USBComm_Send_Message((USBComm_Buffer_t)&bl_txmsg, 32);
	}
	else if (mem_ctrl == _HOST_WRIT_SST32)
	{ // cout<<"tx_HOST_WRIT_SST32"<<endl;
		for (int i = 0; i < 32; i++)
			bl_txmsg.data[i] = data[i];
		res = USBComm_Send_Message((USBComm_Buffer_t)&bl_txmsg, 48);
	}
	else if (mem_ctrl == _HOST_WRIT_SST48)
	{ // cout<<"tx_HOST_WRIT_SST48"<<endl;
		for (int i = 0; i < 48; i++)
			bl_txmsg.data[i] = data[i];
		res = USBComm_Send_Message((USBComm_Buffer_t)&bl_txmsg, 64);
	}

	return res;
}

string get_tnow(void)
{
	auto time_now = std::chrono::system_clock::now();
	std::time_t cur_time = std::chrono::system_clock::to_time_t(time_now);
	string str_now = std::ctime(&cur_time);
	//	std::replace( str_now.begin(), str_now.end(), ' ', '-');
	// cout<<"strNOW_size:"<<str_now.size()<<endl;
	str_now.erase(str_now.begin() + str_now.size() - 1);
	return str_now;
}

unsigned char hex_chksum_cal(string row_data)
{
	unsigned char res = 0;
	int i_bytecount;
	int i_addrH, i_addrL;
	int i_recordtyp, i_bytedata;

	string str_bytecount;
	string str_addrH;
	string str_addrL;
	string str_recordtyp;
	string str_bytedata;
	string str_row_bytes;

	str_bytecount = str_bytecount.assign(row_data, 1, 2); // start 1, size:2.
	str_addrH = str_addrH.assign(row_data, 3, 2);
	str_addrL = str_addrL.assign(row_data, 5, 2);
	str_recordtyp = str_recordtyp.assign(row_data, 7, 2); // start 7, size:2.

	i_bytecount = HexStringToInt(str_bytecount);
	i_addrH = HexStringToInt(str_addrH);
	i_addrL = HexStringToInt(str_addrL);
	i_recordtyp = HexStringToInt(str_recordtyp);

	str_row_bytes = str_row_bytes.assign(row_data, 9, i_bytecount * 2);

	res += (unsigned char)i_bytecount;
	res += (unsigned char)i_addrH;
	res += (unsigned char)i_addrL;
	res += (unsigned char)i_recordtyp;
	// printf("res: 0x%02X\n",res);
	for (int i = 0; i < i_bytecount; i++)
	{
		str_bytedata = str_bytedata.assign(str_row_bytes, i * 2, 2);
		i_bytedata = HexStringToInt(str_bytedata);
		res += (unsigned char)i_bytedata;
	}
	res = 0xff - res + 1;
	// get 1's complement.
	return res;
}

unsigned char hex32_chksum_gen(string row_data)
{
	unsigned char res = 0;
	int i_bytedata;

	string str_bytedata;
	for (int i = 0; i < 32; i++)
	{
		str_bytedata = str_bytedata.assign(row_data, i * 2, 2);
		i_bytedata = HexStringToInt(str_bytedata);
		res += (unsigned char)i_bytedata;
	}
	res = 0xff - res + 1;
	return res;
}

unsigned char hex16_chksum_gen(string row_data)
{
	unsigned char res = 0;
	int i_bytedata;

	string str_bytedata;
	for (int i = 0; i < 16; i++)
	{
		str_bytedata = str_bytedata.assign(row_data, i * 2, 2);
		i_bytedata = HexStringToInt(str_bytedata);
		res += (unsigned char)i_bytedata;
	}
	res = 0xff - res + 1;
	return res;
}

int rc5_key_setting(char *key_in)
{
	unsigned char key_temp[16];
	int key_size = strlen(key_in);
	printf("Get length of key -> %d\n", key_size);

	if (key_size >= 16)
	{
		for (int i = 0; i < 16; i++)
			key_temp[i] = key_in[i];
	}
	else
	{
		for (int i = 0; i < 16; i++)
		{
			if (i < key_size)
				key_temp[i] = key_in[i];
			else
				key_temp[i] = 0x00;
		}
	}

	v_rc5key.clear();
	for (int i = 0; i < 16; i++)
	{
		printf("[%d]:0x%02x ", i, key_temp[i]);
		v_rc5key.push_back(key_temp[i]);
	}
	printf("\n");

	return 0;
}

int rc5_crypt(vector<unsigned char> data_in, vector<unsigned char> *data_crypt)
{
	if (v_rc5key.empty())
		return -1;

	RC5Simple rc5_crypt(true);

	rc5_crypt.RC5_SetKey(v_rc5key);
	rc5_crypt.RC5_Encrypt(data_in, *data_crypt);
	return 0;
}

int rc5_uncrypt(vector<unsigned char> data_in, vector<unsigned char> *data_crypt)
{
	if (v_rc5key.empty())
		return -1;

	RC5Simple rc5_crypt(true);

	rc5_crypt.RC5_SetKey(v_rc5key);
	rc5_crypt.RC5_Decrypt(data_in, *data_crypt);
	return 0;
}

int rc5_encryptfile(char *file_name)
{
	if (v_rc5key.empty())
		return -1;

	int b_filevalid;
	int res;

	b_filevalid = access(file_name, F_OK);
	if (b_filevalid == 0)
		res = 0;
	else if (b_filevalid == -1)
	{
		std::string log = file_name;
		log = "'" + log + "'" + " file is not exist.";
		goDriverLogger.Log("warning", log);
		res = -2;
	}
	if (res < 0)
		return res;

	string crypt_newfile;
	crypt_newfile = file_name;
	std::string strCrypt = "-encrypt";
	crypt_newfile.append(strCrypt);
	RC5Simple rc5_crypt(true);
	rc5_crypt.RC5_SetKey(v_rc5key);
	rc5_crypt.RC5_EncryptFile(string(file_name).c_str(), crypt_newfile.c_str());
	chmod(crypt_newfile.c_str(), 0777);
	chown(crypt_newfile.c_str(), 1000, 1000);
	return 0;
}

int rc5_decryptfile(char *file_name, char **decrypt_filename)
{
	if (v_rc5key.empty())
		return -1;

	int b_filevalid;
	int res;

	b_filevalid = access(file_name, F_OK);
	if (b_filevalid == 0)
		res = 0;
	else if (b_filevalid == -1)
	{
		std::string log = file_name;
		log = "'" + log + "'" + " file is not exist.";
		goDriverLogger.Log("warning", log);
		res = -2;
	}
	if (res < 0)
		return res;

	string file_ext = ".deh";
	string newfile_name = file_name;

	if (newfile_name.size() == 0)
	{
		goDriverLogger.Log("warning", "input file_name size = 0");
		res = -2;
		return res;
	}

	string::size_type dot_pos;
	dot_pos = newfile_name.find(".");
	if (dot_pos != newfile_name.npos)
	{
		if (dot_pos == 0)
		{
			goDriverLogger.Log("warning", "input file_name size = 0, dot in fornt.");
			res = -3;
			return res;
		}
		newfile_name.erase(newfile_name.begin() + dot_pos, newfile_name.end());
	}
	newfile_name = newfile_name.append(file_ext);
	// cout << "decrypt_name: " << newfile_name << endl;
	*decrypt_filename = (char *)malloc(sizeof(char) * 32);
	strcpy(*decrypt_filename, newfile_name.c_str());

	RC5Simple rc5_crypt(true);
	rc5_crypt.RC5_SetKey(v_rc5key);
	rc5_crypt.RC5_DecryptFile(string(file_name).c_str(), newfile_name.c_str());

	chmod(newfile_name.c_str(), 0777);
	chown(newfile_name.c_str(), 1000, 1000);
	return 0;
}

int bl_readme_inifile(char *file_name, ini_sourcefile_t *readme_info)
{
	int res;
	int size_res;
	dictionary *dict = NULL;

	dict = iniparser_load(file_name);
	if (dict == NULL)
	{
		cout << "iniparser load file failed." << endl;
		return -1;
	}

	iniparser_dump(dict, stdout);

	h32_readme.bl_title.assign((iniparser_getstring(dict, "rtc_update:package_title", "")));
	h32_readme.bl_description.assign((iniparser_getstring(dict, "rtc_update:package_description", "")));
	h32_readme.bl_date.assign((iniparser_getstring(dict, "rtc_update:package_revisedate", "")));
	h32_readme.bl_version.assign((iniparser_getstring(dict, "rtc_update:package_version", "")));

	cout << "bl_title:" << h32_readme.bl_title << endl;
	cout << "bl_description:" << h32_readme.bl_description << endl;
	cout << "bl_date:" << h32_readme.bl_date << endl;
	cout << "bl_version:" << h32_readme.bl_version << endl;

	string str_shellcmd;
	size_t pos_ini;
	string str_filename(file_name);
	h32_readme.bl_ini_path = str_filename;

	pos_ini = str_filename.find("ini");
	str_filename.erase(str_filename.begin() + pos_ini, str_filename.end());
	str_filename.append("h32");

	cout << "h32_path:" << str_filename << endl;
	h32_readme.bl_h32_path = str_filename;

	std::vector<char> char_name(str_filename.begin(), str_filename.end());
	char_name.push_back('\0');
	char *size_filename = &char_name[0];

	size_res = bl_read_filesize(size_filename, &h32_readme.bl_filesize);

	//	cout<<"title:"<<h32_readme.bl_title<<endl;
	//	cout<<"description:"<<h32_readme.bl_description<<endl;
	//	cout<<"date:"<<h32_readme.bl_date<<endl;
	//	cout<<"version:"<<h32_readme.bl_version<<endl;
	//	cout<<"filesize:"<<h32_readme.bl_filesize<<endl;

	*readme_info = h32_readme;
	return 0;
}

int bl_read_filesize(char *file_name, string *str_size)
{ // stat -c "%s %n" rtc.ini
	int res = 0;
	int shcmd_res;
	int i_size;
	string str_filename(file_name);
	string sh_cmd("stat -c \"%s \" ");
	string sh_res;
	string size_conv;
	sh_cmd = sh_cmd + str_filename;
	// cout<<"sh_cmd:"<<sh_cmd<<endl;
	shcmd_res = shellCmd(sh_cmd, sh_res);
	// printf("shcmd_res:%d\n", shcmd_res);
	// cout<<"result:"<<sh_res<<endl;

	double d_size = 0;
	std::stringstream size_preci;
	if (shcmd_res == 0)
	{
		i_size = stoi(sh_res);
		cout << "i_size:" << i_size << endl;

		if (i_size > 1000 && i_size < 1e6)
		{
			d_size = double(i_size) / 1000;
			d_size = rounding(d_size, 1);
			// size_conv = to_string(d_size);
			size_preci << std::fixed << std::setprecision(1) << d_size;
			size_conv = size_preci.str();
			size_conv.append("kB");
		}
		else if (i_size > 1e6)
		{
			d_size = double(i_size) / 1e6;
			d_size = rounding(d_size, 1);
			// size_conv = to_string(d_size);
			size_preci << std::fixed << std::setprecision(1) << d_size;
			size_conv = size_preci.str();
			size_conv.append("MB");
		}
		else
		{
			size_conv = to_string(i_size);
			size_conv.append("bytes");
		}

		// cout<<"size_conv:"<<size_conv<<endl;
		*str_size = size_conv;
	}

	return shcmd_res;
}

int shellCmd(const string &cmd, string &result)
{
	char buffer[512];
	result = "";
	// Open pipe to file
	FILE *pipe = popen(cmd.c_str(), "r");
	if (!pipe)
	{
		return -1;
	}

	// read till end of process:
	while (!feof(pipe))
	{
		// use buffer to read and add to result
		if (fgets(buffer, sizeof(buffer), pipe) != NULL)
			result += buffer;
	}

	pclose(pipe);

	if (result.empty())
	{
		return -1;
	}
	else
		return 0;
}

double rounding(double num, int index)
{
	bool isNegative = false; // whether is negative number or not

	if (num < 0) // if this number is negative, then convert to positive number
	{
		isNegative = true;
		num = -num;
	}

	if (index >= 0)
	{
		int multiplier;
		multiplier = pow(10, index);
		num = (int)(num * multiplier + 0.5) / (multiplier * 1.0);
	}

	if (isNegative) // if this number is negative, then convert to negative number
	{
		num = -num;
	}
	return num;
}

int poco_periodictimer(int peridoic_ms)
{
	k1.PeriodicTimer_stop();
	k1.PeriodicTimer_setting(0, peridoic_ms);
	k1.PeriodicTimer_callback();
	return 0;
}

int poco_periodictimerStop(void)
{
	k1.PeriodicTimer_stop();
	return 0;
}

PeriodicTimer::PeriodicTimer()
{
	//_sw.start();
}

void PeriodicTimer::PeriodicTimer_trig(Timer &timer)
{
	this->cnt++;
	std::string log("periodic_time:");
	log = log + std::to_string(this->cnt) + "(unit: 100ms)";
	goDriverLogger.Log("info", log);

	if (!downloading_h32_status.i_curr_msg.empty())
	{

		tx_h32_resp_t tx_msg;
		poco_mutex.lock();
		tx_msg.i_total_msg = downloading_h32_status.i_total_msg;
		tx_msg.i_curr_msg = downloading_h32_status.i_curr_msg.back();
		tx_msg.i_tx_res = downloading_h32_status.i_tx_res.back();

		downloading_h32_status.i_send_ui = tx_msg.i_curr_msg;
		poco_mutex.unlock();
		msg_driv_to_ui_tx_h32_resp(tx_msg);
	}
}

void PeriodicTimer::PeriodicTimer_callback()
{
	TimerCallback<PeriodicTimer> callback(*this, &PeriodicTimer::PeriodicTimer_trig);
	timer.start(callback);
}

void PeriodicTimer::PeriodicTimer_setting(int t0, int tn)
{
	this->timer.setStartInterval(t0);
	this->timer.setPeriodicInterval(tn);
	this->cnt = 0;
}

void PeriodicTimer::PeriodicTimer_stop()
{
	this->timer.stop();
}

void PeriodicTimer::PeriodicTimer_zero()
{
	this->timer.setStartInterval(0);
	this->timer.setPeriodicInterval(0);
}

int bl_LoadHexSour(char *file_name, hex_sourcefile_t *hexsour)
{ // intel hex ref: https://en.wikipedia.org/wiki/Intel_HEX

	// res = -1: file not exist.
	// res = -2: file format fault. startcode not ":"
	// res = -3: file format fault. bytecount > 16.
	// res = -4: file format fault. record type > 5.
	int res = 0;
	int i_line = 0;
	int b_filevalid;
	int i_bytecount;
	int i_recordtyp;
	int i_addrH, i_addrL, i_addr;

	int i_bytecountMAX = 16;
	int i_rectyp_valid = 5; // valid: 0x00, 0x01, 0x02, 0x03, 0x04, 0x05.
	int i_rowchksum;
	int i_rowchksum_cal;
	int i_bytedata;
	char hex_filename[64];

	fstream source_file;
	string file_data;
	string str_startcode;
	string str_bytecount;
	string str_recordtyp;
	string str_rowchksum;

	string str_addrH, str_addrL, str_addr;
	string str_rowdata, str_bytedata;

	b_filevalid = access(file_name, F_OK);
	if (b_filevalid == 0)
		res = 0;
	else if (b_filevalid == -1)
	{
		std::string log = file_name;
		log = "'" + log + "'" + " file is not exist.";
		goDriverLogger.Log("warning", log);
		res = -1;
	}

	if (res < 0)
		return res;
	memset(hexsour->hex_filename, ' ', 48);
	strcpy(hexsour->hex_filename, file_name);
	source_file.open(string(file_name).c_str());

	//  1. check startcode ":"
	//  2. check bytecount > 16
	//  3. check record type: 00, 01, 02, 03, 04, 05.
	//  4. check row checksum match.
	while (source_file >> file_data)
	{
		str_startcode = str_startcode.assign(file_data, 0, 1); // start 0, size:1.
		str_bytecount = str_bytecount.assign(file_data, 1, 2); // start 1, size:2.
		i_bytecount = HexStringToInt(str_bytecount);

		str_recordtyp = str_recordtyp.assign(file_data, 7, 2); // start 7, size:2.
		i_recordtyp = HexStringToInt(str_recordtyp);

		// if( i_recordtyp == )
		str_rowdata = str_rowdata.assign(file_data, 9, i_bytecount * 2);

		str_addrH = str_addrH.assign(file_data, 3, 2);
		str_addrL = str_addrL.assign(file_data, 5, 2);
		i_addrH = HexStringToInt(str_addrH);
		i_addrL = HexStringToInt(str_addrL);

		str_rowchksum = str_rowchksum.assign(file_data, 9 + i_bytecount * 2, 2);
		i_rowchksum = HexStringToInt(str_rowchksum);
		i_line++;

		//------row checksum calculation.
		i_rowchksum_cal = hex_chksum_cal(file_data);
		// printf("row_checksum: 0x%02X\n",i_rowchksum_cal);

		if (str_startcode != ":")
		{
			goDriverLogger.Log("warning", "file format, start code fault");
			res = -2;
			break;
		}

		if (i_bytecount > i_bytecountMAX)
		{
			goDriverLogger.Log("warning", "file format, byte count fault");
			cout << "warning-file format, byte count fault" << endl;
			res = -3;
			break;
		}

		if (i_recordtyp > i_rectyp_valid)
		{
			goDriverLogger.Log("warning", "file format, record type fault");
			res = -4;
			break;
		}

		if (i_rowchksum_cal != i_rowchksum)
		{
			goDriverLogger.Log("warning", "file format, row checksum fault,line_num:" + std::to_string(i_line));
			res = -5;
			break;
		}
	}

	source_file.close();
	if (res == 0)
	{
		std::string log = file_name;
		log = "'" + log + "'" + " file format check successfully.";
		goDriverLogger.Log("info", log);
	}

	if (res < 0)
		return res;

	//  save data.
	vector<int> row_bytes;
	string str_onebyte;
	string str_addrshift;

	int i_onebyte;
	int i_addr_shift;
	int i_addr_offset = 0;
	source_file.open(string(file_name).c_str());

	// hex_source.data_addr.clear();
	// hex_source.row_data.clear();
	// hex_source.rec_type[0] = 0;
	// hex_source.rec_type[1] = 0;
	// hex_source.rec_type[2] = 0;
	// hex_source.rec_type[3] = 0;
	// hex_source.rec_type[4] = 0;
	hexsour->data_addr.clear();
	hexsour->row_data.clear();
	memset(hexsour->rec_type, 0, sizeof(hexsour->rec_type));

	while (source_file >> file_data)
	{
		str_bytecount = str_bytecount.assign(file_data, 1, 2); // start 1, size:2.
		i_bytecount = HexStringToInt(str_bytecount);

		str_recordtyp = str_recordtyp.assign(file_data, 7, 2); // start 7, size:2.
		i_recordtyp = HexStringToInt(str_recordtyp);
		// if( i_recordtyp == 0 )
		//	hex_source.rec_type[0] += 1;
		// else if( i_recordtyp == 1 )
		//	hex_source.rec_type[1] += 1;
		// else if( i_recordtyp == 2 )
		//	hex_source.rec_type[2] += 1;
		// else if( i_recordtyp == 3 )
		//	hex_source.rec_type[3] += 1;
		// else if( i_recordtyp == 4 )
		//	hex_source.rec_type[4] += 1;
		if (i_recordtyp == 0)
			hexsour->rec_type[0] += 1;
		else if (i_recordtyp == 1)
			hexsour->rec_type[1] += 1;
		else if (i_recordtyp == 2)
			hexsour->rec_type[2] += 1;
		else if (i_recordtyp == 3)
			hexsour->rec_type[3] += 1;
		else if (i_recordtyp == 4)
			hexsour->rec_type[4] += 1;

		row_bytes.clear();
		if (i_recordtyp == 0)
		{
			str_rowdata = str_rowdata.assign(file_data, 9, i_bytecount * 2);
			for (int i = 0; i < i_bytecount; i++)
			{
				str_onebyte = str_onebyte.assign(str_rowdata, i * 2, 2);
				i_onebyte = HexStringToInt(str_onebyte);
				row_bytes.push_back(i_onebyte);
			}
			// hex_source.row_data.push_back(row_bytes);
			hexsour->row_data.push_back(row_bytes);

			str_addr = str_addrH.assign(file_data, 3, 4);
			i_addr = HexStringToInt(str_addr);
			i_addr |= i_addr_offset;
			// printf("i_addr: 0x%04x\n",i_addr );
			// hex_source.data_addr.push_back( i_addr );
			hexsour->data_addr.push_back(i_addr);
		}
		else if (i_recordtyp == 1)
		{
			// cout<<"end of hex"<<endl;
			break;
		}
		else if (i_recordtyp == 2)
		{
			cout << "xc16 without this type:0x02" << endl;
			continue;
		}
		else if (i_recordtyp == 3)
		{
			cout << "xc16 without this type:0x03" << endl;
			continue;
		}
		else if (i_recordtyp == 4)
		{
			str_addrshift = str_rowdata.assign(file_data, 9, 4);
			i_addr_shift = HexStringToInt(str_addrshift);
			// printf("addr_shift: 0x%04x\n",i_addr_shift );
			i_addr_offset = i_addr_shift;
			i_addr_offset = (i_addr_offset << 16) & 0xffff0000;
			continue;
		}
	}
	source_file.close();

	cout << "Record type:[ DataSize | EOF | Exten-Seg | Start-Seg | Ext-Linear ]" << endl;
	cout << setw(13) << "[" << setw(10) << centered(std::to_string(hexsour->rec_type[0])) << "|";
	cout << setw(5) << centered(std::to_string(hexsour->rec_type[1])) << "|";
	cout << setw(11) << centered(std::to_string(hexsour->rec_type[2])) << "|";
	cout << setw(11) << centered(std::to_string(hexsour->rec_type[3])) << "|";
	cout << setw(12) << centered(std::to_string(hexsour->rec_type[4])) << "]";
	cout << endl;
	return res;
}

int bl_Read_HexFile_Range_Checksum(hex_sourcefile_t *hex_sour, int StartAddr, int End_Addr, unsigned short *fbk_chksum)
{
#define printscreen 0

	int res = 0;
	if (hex_sour->data_addr.empty())
	{
		goDriverLogger.Log("warning", "hex source address was empty, load hex source file first");
		res = -1;
		return res;
	}

	if (StartAddr > End_Addr)
	{
		goDriverLogger.Log("warning", "address range error");
		res = -2;
		return res;
	}

	int i_vect_ptr = 0;
	int i_curr_addr;
	int i_curr_size;
	int i_next_addr_pred;
	int i_next_addr_real;
	int hexsour_size = hex_sour->data_addr.size();
	unsigned short sum = 0;
	unsigned short word = 0;

	sum = 0;
	i_vect_ptr = 0;
	while (i_vect_ptr < hexsour_size)
	{
		i_curr_addr = hex_sour->data_addr[i_vect_ptr];
		i_curr_size = hex_sour->row_data[i_vect_ptr].size();
		i_next_addr_real = hex_sour->data_addr[i_vect_ptr + 1];

		if (i_curr_addr < StartAddr)
		{
			i_vect_ptr++;
			continue;
		}

		if (i_curr_addr >= End_Addr)
		{
			break;
		}

		if ((i_curr_addr + i_curr_size) == i_next_addr_real)
		{
			if (printscreen == 1)
				printf("0x%06x[%02d] ", i_curr_addr, i_curr_size);
			for (int i = 0; i < i_curr_size; i += 2)
			{
				word = 0x00ff & ((unsigned short)hex_sour->row_data[i_vect_ptr][i]);
				word |= 0xff00 & ((unsigned short)hex_sour->row_data[i_vect_ptr][i + 1] << 8);
				sum += word;
				if (printscreen == 1)
					printf("%04x ", word);
			}
			if (printscreen == 1)
				printf("\n");
		}
		else
		{
			i_next_addr_pred = i_curr_addr + 0x10;
			i_next_addr_real = hex_sour->data_addr[i_vect_ptr + 1];
			if (printscreen == 1)
				printf("0x%06x[%02d] ", i_curr_addr, i_curr_size);
			for (int i = 0; i < 16; i += 2)
			{
				if (i + i_curr_addr >= End_Addr)
				{
					if (printscreen == 1)
						printf("q1\n");
					break;
				}

				if (i < i_curr_size)
				{
					word = 0x00ff & ((unsigned short)hex_sour->row_data[i_vect_ptr][i]);
					word |= 0xff00 & ((unsigned short)hex_sour->row_data[i_vect_ptr][i + 1] << 8);
				}
				else
				{
					word = (i % 4 == 0) ? 0xffff : 0x00ff;
				}
				sum += word;
				if (printscreen == 1)
					printf("%04x ", word);
			}
			if (printscreen == 1)
				printf("\n");

			for (int j = i_next_addr_pred; j < i_next_addr_real; j += 0x10)
			{
				int k = j + 2;
				if (printscreen == 1)
					printf("0x%06x[%02d] ", j, i_curr_size);
				if (j >= End_Addr)
				{
					if (printscreen == 1)
						printf("\n");
					break;
				}
				for (int i = 0; i < 16; i += 2)
				{
					word = (i % 4 == 0) ? 0xffff : 0x00ff;
					sum += word;
					if (printscreen == 1)
						printf("%04x ", word);

					if (k == i_next_addr_real)
					{
						if (printscreen == 1)
							printf("q3\n");
						break;
					}
					else
						k += 2;
				}
				if (printscreen == 1)
					printf("\n");
			}
		}
		i_vect_ptr++;
	}
	*fbk_chksum = 0xffff - sum + 1;
	return res;
}

int bl_WrHexRow_Thread_Init(void)
{
	int res_wrhex_pth = pthread_create(&WriteHexRow_Thread, NULL, bl_WriteHexRowThread_Main_32, NULL);
	WriteHex_32bytes.g_WriteHexRowService_idle.reset();
	// printf("bl_WrHexRow_Thread_Init:%d\n", res_wrhex_pth);
	return 0;
}

int bl_WrHexRow_Thread_Terminate(void)
{
	g_b_WrHexRow_run = false;
	WriteHex_32bytes.g_WriteHexRowService_idle.set();

	if (pthread_join(WriteHexRow_Thread, NULL))
		return 0;
	else
		return -1;
}

int bl_WrHexRow_Wakeup(hex_sourcefile_t *hex_sour, bool tx_info_enable)
{
	int res = 0;
	if (hex_sour->data_addr.empty())
	{
		goDriverLogger.Log("warning", "hex source address was empty, load hex source file first");
		res = -1;
		return res;
	}

	WriteHex_32bytes.hex_sour.data_addr.clear();
	WriteHex_32bytes.hex_sour.row_data.clear();
	memset(WriteHex_32bytes.hex_sour.rec_type, 0, 5);
	memset(WriteHex_32bytes.hex_sour.hex_filename, 0, 48);

	WriteHex_32bytes.hex_sour.data_addr.assign(hex_sour->data_addr.begin(), hex_sour->data_addr.end());
	WriteHex_32bytes.hex_sour.row_data.assign(hex_sour->row_data.begin(), hex_sour->row_data.end());
	memcpy(WriteHex_32bytes.hex_sour.rec_type, hex_sour->rec_type, 5);
	memcpy(WriteHex_32bytes.hex_sour.hex_filename, hex_sour->hex_filename, 48);

	WriteHex_32bytes.WriteHexRow_start = 1;
	WriteHex_32bytes.WriteHexRow_abort_trigger = 0;
	WriteHex_32bytes.WriteHexRow_rtc_response_timeout = 0;
	WriteHex_32bytes.g_WriteHexRowService_idle.set();
	WriteHex_32bytes.WriteHex_tx_info_enable = tx_info_enable;
	return res;
}

int bl_WrHexRow_Abort()
{
	string tmp_string;
	char str_abortlog[256];

	int res;
	if (WriteHex_32bytes.WriteHexRow_start == 1)
	{
		WriteHex_32bytes.WriteHexRow_abort_trigger = 1;
		res = 0;
	}
	else
	{
		res = -1;
	}
	sprintf(str_abortlog, "%s[%d], WrHexRow_Abort res:%d", __func__, __LINE__, res);
	tmp_string.clear();
	tmp_string.assign(str_abortlog);
	goDriverLogger.Log("debug", tmp_string);
	return res;
}
void print_color_green()
{
	printf("\033[1;32m");
}
void print_color_reset()
{
	printf("\033[0m");
}
void progress_bar(int barwidth, float progress)
{
	char str_progress[10];
	int pos = barwidth * progress;
	print_color_green();
	std::cout << "[";
	for (int i = 0; i < barwidth; ++i)
	{
		if (i < pos)
			std::cout << "=";
		else if (i == pos)
			std::cout << ">";
		else
			std::cout << " ";
	}
	sprintf(str_progress, " %3.2f", progress * 100.0);
	std::cout << "]";
	print_color_reset();
	std::cout << str_progress << " %\r";
	std::cout.flush();
}

void *bl_WriteHexRowThread_Main_32(void *p)
{
	int msg_tx = 1;
	int printscr = 0;
	int testing_en = 0;

	// hex_sourcefile_t *hex_ptr = (hex_sourcefile_t *)p;
	hex_sourcefile_t *hex_ptr;
	int i_vect_ptr;
	int i_vect_gen_size;
	float f_vect_ratio;
	int hexsour_size;
	int i_curr_addr;
	int PIC_GENSEG_ADDR_END = 0x55800 * 2;
	int PIC_TESTING_ADDR_END;
	int i_curr_size;
	int i_next_addr_real, i_next_addr_pred;
	int i_vect_ptr_offset;
	int i_n_tab;
	int i_retry_cycle;
	int i_retry_cycle_MAX = 5;
	unsigned short chksum_hexfile, chksum_pic;
	float f_process;

	int sour_addr;
	int cmd_addr, cmd_size;
	unsigned char wrdata[32];
	int fbk_addr;
	unsigned char fbk_size, fbk_chksum;
	int res = 0;
	string logfile_name = "write_hex_record.log";
	string str_pushtemp;
	string str_souraddr, str_sourdata, str_soursize;
	string str_ack_addr, str_ack_val, str_tagstar;
	string str_now;
	string str_hexfile_path;

	string str_chksum_comp;
	string str_chksum_value;

	timetag_t t_wr_execute;
	timetag_t t_wr_prinitf;
	timetag_t t_wr_msginfo;
	pthread_detach(pthread_self());
	while (g_b_WrHexRow_run == true)
	{
		WriteHex_32bytes.g_WriteHexRowService_idle.wait();
		if (g_b_WrHexRow_run == false)
			break;

		i_retry_cycle = 0;
		hex_ptr = &WriteHex_32bytes.hex_sour;

		while (i_retry_cycle < i_retry_cycle_MAX)
		{

			time_tag(tag_all, &t_wr_execute);
			time_tag(tag_all, &t_wr_prinitf);
			time_tag(tag_all, &t_wr_msginfo);
			f_process = 0.0;

			ofstream fileout(logfile_name.c_str(), ios::out | ios::trunc);
			fileout.close();
			chmod(logfile_name.c_str(), 0777);
			chown(logfile_name.c_str(), 1000, 1000);
			fstream file_hex;
			file_hex.open(logfile_name.c_str(), ios::app);

			str_now.clear();
			str_now = get_tnow();
			str_hexfile_path.clear();
			str_hexfile_path.assign(hex_ptr->hex_filename);
			file_hex << "Start:" << str_now << endl;
			file_hex << "Hex-file path:" << str_hexfile_path << endl;

			string info_start_time = "update starting:";
			info_start_time += str_now;
			TxMsg_WriteHexThread_Info(info_start_time, 0);

			string info_sour_path = "source path:    ";
			info_sour_path += str_hexfile_path;
			TxMsg_WriteHexThread_Info(info_sour_path, 0);

			i_n_tab = 0;
			//	1. clear general segment.
			cmd_addr = ADDR_MASK;
			cmd_size = 0;
			if (testing_en == 1)
				PIC_TESTING_ADDR_END = 0x800 * 2;
			else
				PIC_TESTING_ADDR_END = 0x800000 * 2;

			res = (msg_tx == 1) ? AuxBL_Clear_GenFlash_Message(cmd_addr, cmd_size, &fbk_addr, &fbk_size) : 0;
			if (res == 0)
			{
				if (msg_tx == 1)
					printf("AuxBL Clear General Segment Memory:addr:0x%08x, size[%d]\n", fbk_addr, fbk_size);
				TxMsg_WriteHexThread_Info("earse RTC flash memory", 0);
			}
			else
			{
				printf("AuxBL Clear General Flash Fault\n");
				TxMsg_WriteHexThread_Info("earse RTC  flash memory fault", 0);
			}

			//	2. tx hex row by row
			i_vect_ptr = 0;
			i_vect_gen_size = 0;
			hexsour_size = hex_ptr->data_addr.size();

			while (i_vect_gen_size < hexsour_size)
			{
				i_curr_addr = hex_ptr->data_addr[i_vect_gen_size];
				if (i_curr_addr >= PIC_GENSEG_ADDR_END)
					break;
				i_vect_gen_size++;
			}

			while (i_vect_ptr < hexsour_size)
			{
				i_curr_addr = hex_ptr->data_addr[i_vect_ptr];
				if (i_curr_addr >= PIC_GENSEG_ADDR_END)
					break;
				if (i_curr_addr > PIC_TESTING_ADDR_END)
					break;

				if (WriteHex_32bytes.WriteHexRow_abort_trigger == 1)
					break;

				str_pushtemp.clear();
				str_souraddr.clear();
				str_sourdata.clear();
				str_soursize.clear();
				str_ack_addr.clear();
				str_ack_val.clear();
				str_tagstar.clear();

				sour_addr = hex_ptr->data_addr[i_vect_ptr];
				cmd_addr = sour_addr >> 1;
				memset(wrdata, 0, sizeof(wrdata));

				i_next_addr_real = hex_ptr->data_addr[i_vect_ptr + 1];
				i_next_addr_pred = sour_addr + 0x10;

				if (i_next_addr_real == i_next_addr_pred)
				{
					i_curr_size = hex_ptr->row_data[i_vect_ptr].size() + hex_ptr->row_data[i_vect_ptr + 1].size();
					i_vect_ptr_offset = 2;
				}
				else
				{
					i_curr_size = hex_ptr->row_data[i_vect_ptr].size();
					i_vect_ptr_offset = 1;
				}

				if (printscr == 1)
					printf("0x%06x [%02d] ", sour_addr, i_curr_size);
				else
				{
					time_tag(tag_delta, &t_wr_msginfo);
					if (t_wr_msginfo.delta_tn > 300)
					{
						progress_bar(70, f_process);
						f_process = (float)i_vect_ptr / (float)i_vect_gen_size;

						TxMsg_Info_percentage(i_vect_ptr, i_vect_gen_size);
						time_tag(tag_all, &t_wr_msginfo);
					}
				}

				str_pushtemp = "0x";
				str_souraddr = var_to_string((int)sour_addr, 6);
				str_pushtemp += str_souraddr;
				str_soursize = var_to_string((int)i_curr_size, 2);
				str_pushtemp = str_pushtemp + "[" + str_soursize + "] ";

				for (int i = 0; i < i_curr_size; i++)
				{
					if (i < 16)
						wrdata[i] = (unsigned char)hex_ptr->row_data[i_vect_ptr][i];
					else
						wrdata[i] = (unsigned char)hex_ptr->row_data[i_vect_ptr + 1][i - 16];

					if (printscr == 1)
						printf("%02x ", wrdata[i]);

					str_sourdata = var_to_string((int)wrdata[i], 2);
					str_pushtemp += str_sourdata + " ";
				}

				// res = (msg_tx == 1) ? AuxBL_WriteFlashMemory_Message(cmd_addr, i_curr_size, wrdata, &fbk_addr, &fbk_size, &fbk_chksum) : 0;

				if (msg_tx == 1)
					res = AuxBL_WriteFlashMemory_Message(cmd_addr, i_curr_size, wrdata, &fbk_addr, &fbk_size, &fbk_chksum);
				else
					res = 0;

				if (res == 0)
				{
					// printf("AuxBL Write Memory_ack: 0x%08x, size[%d]\n", fbk_addr, fbk_size);
					if (msg_tx == 0)
					{
						fbk_addr = sour_addr;
						fbk_size = i_curr_size;
						fbk_chksum = 0xff;
					}

					if (printscr == 1)
						printf(" ||:0x%06x[%d], 0x%02x,", fbk_addr, fbk_size, fbk_chksum);

					if (fbk_chksum != 0x00)
						str_tagstar = " {*} ";
					else
						str_tagstar = "";

					str_ack_addr = var_to_string((int)fbk_addr, 6);
					str_ack_val = var_to_string((int)fbk_chksum, 2);
					str_pushtemp += "||:0x" + str_ack_addr + " 0x" + str_ack_val + str_tagstar;
				}
				else
				{
					printf("AuxBL Write Memory Fault\n");
					WriteHex_32bytes.WriteHexRow_rtc_response_timeout = 1;
					break;
				}

				if (printscr == 1)
					printf("\n ");

				file_hex << str_pushtemp << endl;

				i_vect_ptr += i_vect_ptr_offset;
			}
			time_tag(tag_delta, &t_wr_execute);
			std::string s_exet = "execute time:";
			s_exet += std::to_string(t_wr_execute.delta_tn) + "ms";
			file_hex << s_exet << endl;

			f_process = (float)i_vect_ptr / (float)i_vect_gen_size;
			progress_bar(70, f_process);
			printf("\n");
			printf("Write Hex Row, execute time:%dms. \n", t_wr_execute.delta_tn);

			TxMsg_Info_percentage(i_vect_ptr, i_vect_gen_size);

			string info_duration_time = "duration time:";
			info_duration_time += std::to_string(t_wr_execute.delta_tn / 1000) + "sec";
			TxMsg_WriteHexThread_Info(info_duration_time, 0);

			if (WriteHex_32bytes.WriteHexRow_abort_trigger == 1)
			{
				break;
			}

			if (WriteHex_32bytes.WriteHexRow_rtc_response_timeout == 0)
			{
				//	3. compare chksum from hex-file & pic memory.
				res = AuxBL_Read_Segment_Checksum_Message(0, &chksum_pic);
				if (res != 0)
					chksum_pic = 0xffff;

				res = bl_Read_HexFile_Range_Checksum(hex_ptr, 0x0000, 0x55800 * 2, &chksum_hexfile);
				printf("chksum hex-file/pic: 0x%04x/0x%04x \n ", chksum_hexfile, chksum_pic);

				str_chksum_comp.clear();
				str_chksum_comp = "chksum hex-file/pic: 0x";
				str_chksum_value.clear();
				str_chksum_value = var_to_string((int)chksum_hexfile, 4);
				str_chksum_comp += str_chksum_value;
				str_chksum_comp += ", 0x";
				str_chksum_value.clear();
				str_chksum_value = var_to_string((int)chksum_pic, 4);
				str_chksum_comp += str_chksum_value;
				str_chksum_comp += ", retry: ";
				str_chksum_comp += std::to_string(i_retry_cycle) + "cycle";

				file_hex << str_chksum_comp << endl;
				str_now.clear();
				str_now = get_tnow();
				file_hex << "End:" << str_now << endl;

				string info_chksum_retry = "checksum>> imgfile: 0x";
				info_chksum_retry += var_to_string((int)chksum_hexfile, 4);
				info_chksum_retry += ", pic: 0x" + var_to_string((int)chksum_pic, 4);
				info_chksum_retry += ", retry: " + std::to_string(i_retry_cycle) + "cycle";
				TxMsg_WriteHexThread_Info(info_chksum_retry, 0);

				if (chksum_pic != chksum_hexfile)
				{
					i_retry_cycle++;
					printf("hold on 1 sec\n\n");
					sleep(1);
				}
				else
					break;
			}
			else
			{
				file_hex << "WrHex not Completed, timeout" << endl;
				WriteHex_32bytes.WriteHexRow_rtc_response_timeout = 0;
				printf("hold on 1 sec\n\n");
				sleep(1);
				i_retry_cycle++;
			}
			file_hex.close();

			printf("retry cycle:%d\n\n", i_retry_cycle);
			string info_timeout_retry = "timeout retry:";
			info_timeout_retry += std::to_string(i_retry_cycle) + "cycle";
			TxMsg_WriteHexThread_Info(info_timeout_retry, 0);
		}

		WriteHex_32bytes.WriteHexRow_start = 0;

		if (WriteHex_32bytes.WriteHexRow_abort_trigger == 0)
		{
			string info_wrhex_done = "update process done.";
			TxMsg_WriteHexThread_Info(info_wrhex_done, 2);
		}
		else if (WriteHex_32bytes.WriteHexRow_abort_trigger == 1)
		{
			string info_wrhex_abort = "updating interrupt";
			TxMsg_WriteHexThread_Info(info_wrhex_abort, 3);
		}
		// printf("WriteHexRowThread_Sleep\n");
	}
	printf("WriteHexRowThread_Terminated\n");
	return 0;
}

void TxMsg_Info_percentage(int num, int den)
{
	string str_num = to_string(num);
	string str_den = to_string(den);
	string str_percentage = str_num + "," + str_den;
	TxMsg_WriteHexThread_Info(str_percentage, 1);
}

string var_to_string(int var, int setw_size)
{
	std::stringstream stream;
	std::string str_temp;
	stream.str("");
	stream.clear();
	str_temp.clear();
	stream << std::hex << std::uppercase << std::setw(setw_size) << setfill('0') << var;
	stream >> str_temp;
	return str_temp;
}

int bl_EnduranceTest_Thread_Init(void)
{
	int res = pthread_create(&BLEndurance_Thread, NULL, bl_EnduranceTest_Main, NULL);
	BL_EnduranceTest.g_Endurance_Test_idle.reset();
	// printf("bl_EnduranceTest_Thread_Init:%d\n", res);
	return 0;
}

int bl_EnduranceTest_Thread_Terminate(void)
{
	g_b_BLEndurance_run = false;
	BL_EnduranceTest.g_Endurance_Test_idle.set();
	if (pthread_join(BLEndurance_Thread, NULL))
		return 0;
	else
		return -1;
}

int bl_EnduranceTest_Wakeup(int cycle)
{
	int res = 0;
	BL_EnduranceTest.Endurance_Test_cycle = cycle;
	BL_EnduranceTest.Endurance_Test_start = 1;
	BL_EnduranceTest.Endurance_Test_abort_trigger = 0;
	BL_EnduranceTest.Endurance_Test_state_error = 0;
	BL_EnduranceTest.Endurance_DeviceState = DEVICE_NULL;
	memset(BL_EnduranceTest.hexfile1_path, ' ', 48);
	memset(BL_EnduranceTest.hexfile2_path, ' ', 48);
	memset(BL_EnduranceTest.hexfile3_path, ' ', 48);
	strcpy(BL_EnduranceTest.hexfile1_path, "hex_sour/mer0.hex");
	strcpy(BL_EnduranceTest.hexfile2_path, "hex_sour/mer1.hex");
	strcpy(BL_EnduranceTest.hexfile3_path, "hex_sour/mer2.hex");
	BL_EnduranceTest.g_Endurance_Test_idle.set();
	return res;
}

int bl_EnduranceTest_Abort()
{
	string tmp_string;
	char str_abortlog[256];

	int res;
	if (BL_EnduranceTest.Endurance_Test_start == 1)
	{
		BL_EnduranceTest.Endurance_Test_abort_trigger = 1;
		res = 0;
	}
	else
	{
		res = -1;
	}
	sprintf(str_abortlog, "%s[%d], EnduranceTest_Abortres:%d", __func__, __LINE__, res);
	tmp_string.clear();
	tmp_string.assign(str_abortlog);
	goDriverLogger.Log("debug", tmp_string);
	return res;
}

void *bl_EnduranceTest_Main(void *p)
{
	int msg_tx = 1;
	int printscr = 0;
	int testing_en = 0;
	int current_device;
	int i_hexfile_sel;
	int i_endurance_test_inc;
	int res_load_hexfile;
	char *hex_filename;
	int res;
	unsigned short pic_chksum, hexsour_gen_chksum, hexsour_aux_chksum;
	int fbk_addr;
	unsigned int fbk_addr_ons;
	unsigned char fbk_size;
	unsigned char config_reg;
	unsigned char fbk_reset_mode;
	char str_endu_log[256];
	string tmp_string;
	timetag_t t_hexwr;
	int rtc_base, rtc_major, rtc_minor, prot_base, prot_major, prot_minor;

	Poco::Event comm_wait;
	pthread_detach(pthread_self());
	while (g_b_BLEndurance_run == true)
	{
		BL_EnduranceTest.g_Endurance_Test_idle.wait();
		if (g_b_BLEndurance_run == false)
			break;

		i_hexfile_sel = 0;
		i_endurance_test_inc = 0;

		sprintf(str_endu_log, "%s[%d]BL_EnduranceTest Start, Cycle:%d", __func__, __LINE__, BL_EnduranceTest.Endurance_Test_cycle);
		tmp_string.clear();
		tmp_string.assign(str_endu_log);
		goDriverLogger.Log("info", tmp_string);

		while (BL_EnduranceTest.Endurance_Test_start == 1)
		{
			rtc_base = -1;
			rtc_major = -1;
			rtc_minor = -1;
			prot_base = -1;
			prot_major = -1;
			prot_minor = -1;

			switch (BL_EnduranceTest.Endurance_DeviceState)
			{
			case DEVICE_NULL:
				current_device = BL_USBComm_Driver_getTargetDevice();
				printf("current_device:%d\n", current_device);

				if (current_device == DEVICE_ONS)
					BL_EnduranceTest.Endurance_DeviceState = DEVICE_ONS;
				else if (current_device == DEVICE_BL)
					BL_EnduranceTest.Endurance_DeviceState = DEVICE_BL;
				break;

			case DEVICE_ONS:
				BL_EnduranceTest.Endurance_ONS_State = ONS_READ_VER_NUM;
				while (BL_EnduranceTest.Endurance_Test_state_error == 0)
				{
					if (BL_EnduranceTest.Endurance_ONS_State == ONS_SWITCH_SUCCESS)
					{
						BL_EnduranceTest.Endurance_DeviceState = DEVICE_NULL;
						break;
					}

					switch (BL_EnduranceTest.Endurance_ONS_State)
					{
					case ONS_READ_VER_NUM:
						printf("ONS_READ_VER_NUM -- \n");
						res = AuxBL_ONS_VerionReq(&rtc_base, &rtc_major, &rtc_minor, &prot_base, &prot_major, &prot_minor);
						if (res == 0)
						{
							printf("RtcVer:[%d,%d,%d], ProtVer:[%d,%d,%d]\n", rtc_base, rtc_major, rtc_minor, prot_base, prot_major, prot_minor);
							BL_EnduranceTest.Endurance_ONS_State = ONS_SWITCH_TO_BL;
						}
						else
						{
							printf("ONS_VerionReq Fault\n");
							BL_EnduranceTest.Endurance_ONS_State = ONS_READ_VER_NUM_FAULT;
						}
						break;

					case ONS_SWITCH_TO_BL:
						printf("ONS_SWITCH_TO_BL -- \n");
						res = AuxBL_ONS_Read_ConfigReg_Req(0x00F8000E, &fbk_addr_ons, &config_reg);
						if (res != 0)
						{
							BL_EnduranceTest.Endurance_ONS_State = ONS_SWITCH_TO_BL_FAULT;
							break;
						}
						res = AuxBL_ONS_Write_ConfigReg_Req(0x00F8000E, 0xdb, &fbk_addr_ons, &config_reg);
						if (res != 0)
						{
							BL_EnduranceTest.Endurance_ONS_State = ONS_SWITCH_TO_BL_FAULT;
							break;
						}
						res = AuxBL_ONS_Reset_PIC_Req(1, &fbk_reset_mode);
						if (res != 0)
						{
							BL_EnduranceTest.Endurance_ONS_State = ONS_SWITCH_TO_BL_FAULT;
							break;
						}
						printf("Reset PIC mode:%d\n", fbk_reset_mode);
						BL_EnduranceTest.Endurance_ONS_State = ONS_SWITCH_SUCCESS;
						break;

					case ONS_READ_VER_NUM_FAULT:
						BL_EnduranceTest.Endurance_Test_state_error = -1;
						BL_EnduranceTest.Endurance_Test_start = 0;
						break;
					case ONS_SWITCH_TO_BL_FAULT:
						BL_EnduranceTest.Endurance_Test_state_error = -2;
						BL_EnduranceTest.Endurance_Test_start = 0;
						break;
					}
					comm_wait.tryWait(500);
				}
				break;

			case DEVICE_BL:
				BL_EnduranceTest.Endurance_BL_State = BL_DEFAULT;

				while (BL_EnduranceTest.Endurance_Test_state_error == 0)
				{
					if (BL_EnduranceTest.Endurance_BL_State == BL_SWITCH_SUCCESS)
					{
						BL_EnduranceTest.Endurance_DeviceState = DEVICE_NULL;
						break;
					}

					switch (BL_EnduranceTest.Endurance_BL_State)
					{
					case BL_DEFAULT:
						BL_EnduranceTest.Endurance_Test_cycle--;
						i_endurance_test_inc++;
						// printf(">>>>>>>>>>TestCycle:%d,inc:%d\n\n\n",BL_EnduranceTest.Endurance_Test_cycle,i_endurance_test_inc);

						sprintf(str_endu_log, "%s[%d] remnant:%d, Inc:%d", __func__, __LINE__, BL_EnduranceTest.Endurance_Test_cycle, i_endurance_test_inc);
						tmp_string.clear();
						tmp_string.assign(str_endu_log);
						goDriverLogger.Log("info", tmp_string);
						printf("\n\n\n");

						if (BL_EnduranceTest.Endurance_Test_cycle >= 0)
						{
							BL_EnduranceTest.Endurance_BL_State = BL_LOAD_HEXFILE;
							// printf("BL_LOAD_HEXFILE, TestCycle:%d\n",BL_EnduranceTest.Endurance_Test_cycle);
						}
						else
						{
							BL_EnduranceTest.Endurance_BL_State = BL_SWITCH_SUCCESS;
							BL_EnduranceTest.Endurance_Test_start = 0;
							// printf("TestStart=0, TestCycle:%d\n",BL_EnduranceTest.Endurance_Test_cycle);
						}
						break;

					case BL_LOAD_HEXFILE:
						if (i_hexfile_sel == 0)
							hex_filename = &BL_EnduranceTest.hexfile1_path[0];
						else if (i_hexfile_sel == 1)
							hex_filename = &BL_EnduranceTest.hexfile2_path[0];
						else if (i_hexfile_sel == 2)
							hex_filename = &BL_EnduranceTest.hexfile3_path[0];
						else
						{
							i_hexfile_sel = 0;
							hex_filename = &BL_EnduranceTest.hexfile1_path[0];
						}

						// printf("i_hexfile_sel:%d, path:%s\n",i_hexfile_sel,hex_filename);

						sprintf(str_endu_log, "%s[%d] Hexfile_path:%s", __func__, __LINE__, hex_filename);
						tmp_string.clear();
						tmp_string.assign(str_endu_log);
						goDriverLogger.Log("info", tmp_string);

						i_hexfile_sel++;
						if (i_hexfile_sel > 3)
							i_hexfile_sel = 0;

						if (bl_LoadHexSour(hex_filename, &BL_EnduranceTest.hexfile_hexsour) == 0)
							BL_EnduranceTest.Endurance_BL_State = BL_SEND_HEXFILE;
						else
							BL_EnduranceTest.Endurance_BL_State = BL_LOAD_HEXFILE_FAULT;
						break;

					case BL_SEND_HEXFILE:
						if (bl_WrHexRow_Wakeup(&BL_EnduranceTest.hexfile_hexsour, false) == 0)
						{
							BL_EnduranceTest.Endurance_BL_State = BL_WAIT_WRHEX;
							time_tag(tag_all, &t_hexwr);
						}
						else
							BL_EnduranceTest.Endurance_BL_State = BL_SEND_HEXFILE_FAULT;
						break;

					case BL_WAIT_WRHEX:
						if (WriteHex_32bytes.WriteHexRow_start == 0)
						{
							// printf("BL_WAIT_WRHEX -- Finished \n");
							time_tag(tag_delta, &t_hexwr);
							if (WriteHex_32bytes.WriteHexRow_rtc_response_timeout == 1) // this procedure is okay.
							{
								BL_EnduranceTest.Endurance_BL_State = BL_WAIT_WRHEX_FAULT;
								sprintf(str_endu_log, "%s[%d]WR_HEX Fault, execute time: %dms", __func__, __LINE__, t_hexwr.delta_tn);
							}
							else
							{
								BL_EnduranceTest.Endurance_BL_State = BL_COMP_CHECKSUM;
								sprintf(str_endu_log, "%s[%d]WR_HEX Completed, execute time: %dms", __func__, __LINE__, t_hexwr.delta_tn);
							}
							tmp_string.clear();
							tmp_string.assign(str_endu_log);
							goDriverLogger.Log("info", tmp_string);
						}
						break;

					case BL_COMP_CHECKSUM:
						printf("BL_COMP_CHECKSUM -- \n");

						res = AuxBL_Read_Segment_Checksum_Message(0, &pic_chksum);
						if (res != 0)
							pic_chksum = 0xffff;
						// printf("AuxBL Read General Segment Checksum: 0x%04x\n", pic_chksum);

						res = bl_Read_HexFile_Range_Checksum(&BL_EnduranceTest.hexfile_hexsour, 0x0000, 0x55800 * 2, &hexsour_gen_chksum);
						res = bl_Read_HexFile_Range_Checksum(&BL_EnduranceTest.hexfile_hexsour, 0x7FC000 * 2, 0x800000 * 2, &hexsour_aux_chksum);
						// printf("file name:%s, GenSeg|AuxSeg checksum: [0x%04x, 0x%04x]\n", BL_EnduranceTest.hexfile_hexsour.hex_filename,hexsour_gen_chksum, hexsour_aux_chksum);

						sprintf(str_endu_log, "%s[%d]Compare gen-chksum: HexFile:0x%04x,PIC:0x%04x", __func__, __LINE__, hexsour_gen_chksum, pic_chksum);
						tmp_string.clear();
						tmp_string.assign(str_endu_log);
						goDriverLogger.Log("info", tmp_string);

						if (pic_chksum == hexsour_gen_chksum)
							BL_EnduranceTest.Endurance_BL_State = BL_SWITCH_TO_ONS;
						else
							BL_EnduranceTest.Endurance_BL_State = BL_COMP_CHECKSUM_FAULT;
						break;

					case BL_SWITCH_TO_ONS:
						printf("BL_SWITCH_TO_ONS -- \n");

						res = AuxBL_WriteConfigReg_Message(ADDR_Config_FICD, 1, 0xdf, &fbk_addr, &fbk_size);
						if (res != 0)
						{
							BL_EnduranceTest.Endurance_BL_State = BL_SWITCH_TO_ONS_FAULT;
							break;
						}
						res = AuxBL_ReadConfigReg_Message(ADDR_Config_FICD, 1, &fbk_addr, &config_reg, &fbk_size);
						if (res != 0)
						{
							BL_EnduranceTest.Endurance_BL_State = BL_SWITCH_TO_ONS_FAULT;
							break;
						}
						// printf("AuxBL Read Configuration Register: addr:0x%08x, size[%d], reg:0x%02x\n", fbk_addr, fbk_size, config_reg);
						if (config_reg != 0xdf)
						{
							BL_EnduranceTest.Endurance_BL_State = BL_SWITCH_TO_ONS_FAULT;
							break;
						}
						res = AuxBL_Reset_Message(ADDR_MASK, 0, &fbk_addr, &fbk_size);
						if (res != 0)
						{
							BL_EnduranceTest.Endurance_BL_State = BL_SWITCH_TO_ONS_FAULT;
							break;
						}
						// printf("AuxBL RESET:addr:0x%08x, size[%d]\n", fbk_addr, fbk_size);

						sprintf(str_endu_log, "%s[%d]Switch_ONS, Config:0x%02x", __func__, __LINE__, config_reg);
						tmp_string.clear();
						tmp_string.assign(str_endu_log);
						goDriverLogger.Log("info", tmp_string);

						BL_EnduranceTest.Endurance_BL_State = BL_SWITCH_SUCCESS;
						break;

					case BL_LOAD_HEXFILE_FAULT:
						BL_EnduranceTest.Endurance_Test_state_error = -11;
						BL_EnduranceTest.Endurance_Test_start = 0;
						break;
					case BL_SEND_HEXFILE_FAULT:
						BL_EnduranceTest.Endurance_Test_state_error = -12;
						BL_EnduranceTest.Endurance_Test_start = 0;
						break;
					case BL_WAIT_WRHEX_FAULT:
						BL_EnduranceTest.Endurance_Test_state_error = -13;
						BL_EnduranceTest.Endurance_Test_start = 0;
						break;
					case BL_COMP_CHECKSUM_FAULT:
						BL_EnduranceTest.Endurance_Test_state_error = -14;
						BL_EnduranceTest.Endurance_Test_start = 0;
						break;
					case BL_SWITCH_TO_ONS_FAULT:
						BL_EnduranceTest.Endurance_Test_state_error = -15;
						BL_EnduranceTest.Endurance_Test_start = 0;
						break;
					}
					comm_wait.tryWait(500);
				}
				break;
			}
			comm_wait.tryWait(800);
		}
		// printf("BL_EnduranceTest.Endurance_Test_start = 0\n");

		sprintf(str_endu_log, "%s[%d]BL_EnduranceTest End, err_no:%d", __func__, __LINE__, BL_EnduranceTest.Endurance_Test_state_error);
		tmp_string.clear();
		tmp_string.assign(str_endu_log);
		goDriverLogger.Log("info", tmp_string);
	}
	printf("BL_EnduranceTest_Terminated\n");
	return 0;
}

int RTC_Device_Switch(string switch_cmd)
{
	int target_device;
	int switch_flow = 0;
	int fbk_status = 0;
	int res = 0, usbcomm_res;
	int fbk_addr;
	unsigned char fbk_size, reg_val, fbk_val;
	unsigned char fbk_reset_mode;
	unsigned int fbk_addr_ui;
	int rtc_base = -1, rtc_major = -1, rtc_minor = -1, prot_base = -1, prot_major = -1, prot_minor = -1;

	if (switch_cmd == "Write_FICD_RSTPRI_Pri")
	{
		while (1)
		{
			if (res < 0)
				break;
			if (switch_flow == 10)
				break;
			switch (switch_flow)
			{
			case 0:
				target_device = BL_USBComm_Get_RTC_Device_Status();
				if (target_device != Device_ABC_RTC)
					res = -1;
				else
					switch_flow++;
				break;
			case 1:
				usbcomm_res = AuxBL_ReadConfigReg_Message(ADDR_Config_FICD, 1, &fbk_addr, &reg_val, &fbk_size);
				if (usbcomm_res != 0)
					res = -2;
				else
					switch_flow++;
				break;
			case 2:
				reg_val |= BIT_RSTPRI;
				usbcomm_res = AuxBL_WriteConfigReg_Message(ADDR_Config_FICD, 1, reg_val, &fbk_addr, &fbk_size);
				if (usbcomm_res != 0)
					res = -3;
				else
					switch_flow++;
				break;
			case 3:
				usbcomm_res = AuxBL_Reset_Message(ADDR_MASK, 0, &fbk_addr, &fbk_size);
				if (usbcomm_res != 0)
					res = -4;
				else
					switch_flow++;
				break;

			case 4:
				switch_flow = 10;
				res = 0;
				break;
			}
		}
	}
	else if (switch_cmd == "Write_FICD_RSTPRI_Aux")
	{
		while (1)
		{
			if (res < 0)
				break;
			if (switch_flow == 10)
				break;

			switch (switch_flow)
			{
			case 0:
				target_device = BL_USBComm_Get_RTC_Device_Status();
				if (target_device != Device_ABC_RTC)
					res = -1;
				else
				{
					printf("ONS_VerionReq ------------------------------\n");
					res = AuxBL_ONS_VerionReq(&rtc_base, &rtc_major, &rtc_minor, &prot_base, &prot_major, &prot_minor);
					if (res == 0)
					{
						printf("RtcVer:[%d,%d,%d], ProtVer:[%d,%d,%d]\n", rtc_base, rtc_major, rtc_minor, prot_base, prot_major, prot_minor);
					}
					else
						printf("ONS_VerionReq Fault\n");

					switch_flow++;
				}
				break;

			case 1:
				usbcomm_res = AuxBL_ONS_Read_ConfigReg_Req(ADDR_Config_FICD, &fbk_addr_ui, &reg_val);
				if (usbcomm_res != 0)
					res = -2;
				else
					switch_flow++;
				break;

			case 2:
				reg_val &= 0xfb;
				usbcomm_res = AuxBL_ONS_Write_ConfigReg_Req(ADDR_Config_FICD, reg_val, &fbk_addr_ui, &fbk_val);
				if (usbcomm_res != 0)
					res = -3;
				else
					switch_flow++;
				break;

			case 3:
				usbcomm_res = AuxBL_ONS_Reset_PIC_Req(1, &fbk_reset_mode);
				if (usbcomm_res != 0)
					res = -4;
				else
					switch_flow++;
				break;

			case 4:
				switch_flow = 10;
				res = 0;
				break;
			}
		}
	}

	// printf("_______________res:%d\n",res);
	return res;
}

int TxMsg_WriteHexThread_Info(string info, int status)
{
	if (WriteHex_32bytes.WriteHex_tx_info_enable == false)
		return -1;
	std::string str_wrhex_info = "{\"bldriv_fbk\":\"writehex_info\",\"value\":" + to_string(status) + "}";
	json jstr_wrhex_info = json::parse(str_wrhex_info);
	jstr_wrhex_info["info"] = info;
	str_wrhex_info.clear();
	str_wrhex_info = jstr_wrhex_info.dump();
	tx_msg_to_Bootloader_UI(str_wrhex_info);
	return 0;
}

void Set_BL_Driver_Run_Terminate(void)
{
	bl_driver_run = false;
}

int Get_BL_Driver_Run_Terminate_Status(void)
{
	return bl_driver_run;
}