#include "HandleCmd.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdint.h>
#include <iostream>
#include <string>
#include <vector>

#include <fcntl.h>
#include <termios.h>
#include <errno.h>

#include "Poco/Task.h"
#include "Poco/TaskManager.h"
#include "Poco/TaskNotification.h"
#include "Poco/NotificationCenter.h"
#include "Poco/NotificationQueue.h"
#include "Poco/Exception.h"

#include "interface_to_host.h"
#include "Mqtt_prot.h"

#include "logger_wrapper.h"
#include "notification_pkg.h"
#include "json.hpp"
#include "bl_return_code.h"
#include "BL_USBComm.h"
#include "interface_driver_to_ui.h"
#include "update_sour.h"

using namespace std;
using namespace Poco;

using json = nlohmann::json;

static HostCmdHandler *instance = NULL;

bool b_simulation_en;
bool gbIgnoreDriverStMode = false;

string g_str_imgfile_path;
string g_str_dest_path;
char *g_decrypt_path = NULL;

hex_sourcefile_t handle_hexfile_hexsour;

std::string parseCmdPackage(const std::string &pkg)
{
	json json_msg = json::parse(pkg);
	// cout << "json msg = " << json_msg << endl;

	if (json_msg.find("UI_cmd") == json_msg.end())
		return "Cannot find specific key";
	cout << pkg << endl;
	std::string ui_cmd = json_msg["UI_cmd"];

	json json_ack;
	json_ack["id"] = json_msg["id"];
	json_ack["cmd"] = "ack";

	if (ui_cmd == "test")
	{
		printf("not thing to do.\n");
	}
	else if (ui_cmd == "get_status")
	{
		std::string request_target = json_msg["target"];
		if (request_target == "bl_driver")
		{
			std::string bl_driver_alive = "{\"bldriv_fbk\": \"status_fbk\",\"target\":\"bl_driver\",\"value\":" + to_string(0) + "}";
			tx_msg_to_Bootloader_UI(bl_driver_alive);
		}
		else if (request_target == "rtc_device")
		{
			int target_device = BL_USBComm_Get_RTC_Device_Status();
			std::string rtc_device_alive = "{\"bldriv_fbk\": \"status_fbk\",\"target\":\"rtc_device\",\"value\":" + to_string(target_device) + "}";
			tx_msg_to_Bootloader_UI(rtc_device_alive);
		}
		else if (request_target == "update_img")
		{
			/*
			string str_imgfile_path;
			int img_file_status = Update_Sour_Get_ImgFile_Status(&str_imgfile_path);

			if (img_file_status == 0)
			{
				g_str_imgfile_path.clear();
				g_str_imgfile_path = str_imgfile_path;
			}
			std::string str_img_alive = "{\"bldriv_fbk\":\"status_fbk\",\"target\":\"update_img\",\"value\":" + to_string(img_file_status) + "}";
			json j_str_img_alive = json::parse(str_img_alive);
			j_str_img_alive["imgfile_path"] = str_imgfile_path;
			str_img_alive.clear();
			str_img_alive = j_str_img_alive.dump();
			tx_msg_to_Bootloader_UI(str_img_alive);
			*/

			string str_imgfile_path;
			if (Get_Update_Sour_ImgPath_Status(&str_imgfile_path) == 0)
			{
				printf("inner_imgpath:%s\n", str_imgfile_path.c_str());
				g_str_imgfile_path.clear();
				g_str_imgfile_path = str_imgfile_path;
			}

			bool b_img_exist = fileExists(g_str_imgfile_path);

			std::string str_img_alive = "{\"bldriv_fbk\":\"status_fbk\",\"target\":\"update_img\"}";
			json j_str_img_alive = json::parse(str_img_alive);

			if (b_img_exist == true)
			{
				j_str_img_alive["value"] = 0;
				j_str_img_alive["imgfile_path"] = g_str_imgfile_path;
			}
			else
			{
				j_str_img_alive["value"] = -1;
				j_str_img_alive["imgfile_path"] = "";
			}
			str_img_alive.clear();
			str_img_alive = j_str_img_alive.dump();

			cout << str_img_alive << endl
				 << endl
				 << endl;

			tx_msg_to_Bootloader_UI(str_img_alive);
		}
		else if (request_target == "update_info")
		{
			/*
			string str_info_path = json_msg["target_path"];
			string str_info_msg;
			int info_status = Update_Sour_find_info(str_info_path, &str_info_msg);
			std::string str_info_alive = "{\"bldriv_fbk\":\"status_fbk\",\"target\":\"update_info\",\"value\":" + to_string(info_status) + "}";
			json jstr_info_alive = json::parse(str_info_alive);
			jstr_info_alive["info_msg"] = str_info_msg;
			str_info_alive.clear();
			str_info_alive = jstr_info_alive.dump();
			tx_msg_to_Bootloader_UI(str_info_alive);
			*/

			string str_ini_path;
			if (Get_Update_Sour_IniPath_Status(&str_ini_path) == 0)
				printf("inner_inipath:%s\n", str_ini_path.c_str());

			bool b_ini_exist = fileExists(str_ini_path);
			int info_status;

			std::string str_info_alive = "{\"bldriv_fbk\":\"status_fbk\",\"target\":\"update_info\"}";
			json jstr_info_alive = json::parse(str_info_alive);

			if (b_ini_exist == true)
			{
				string str_info_msg;
				info_status = Update_Sour_find_info(str_ini_path, &str_info_msg);
				cout << "info_msg:" << str_info_msg << endl;

				jstr_info_alive["info_msg"] = str_info_msg;
				jstr_info_alive["value"] = info_status;
				jstr_info_alive["inifile_path"] = str_ini_path;
			}
			else
			{
				info_status = -1;
				jstr_info_alive["value"] = info_status;
			}

			str_info_alive.clear();
			str_info_alive = jstr_info_alive.dump();
			tx_msg_to_Bootloader_UI(str_info_alive);
		}
	}
	else if (ui_cmd == "rtc_dev_switch_cmd")
	{
		std::string str_action = json_msg["action"];
		int res = RTC_Device_Switch(str_action);

		std::string str_Wr_configFICD_RSTPRI = "{\"bldriv_fbk\":\"rtc_dev_switch_fbk\",\"value\":" + to_string(res) + "}";
		json jstr_Wr_configFICD_RSTPRI = json::parse(str_Wr_configFICD_RSTPRI);
		jstr_Wr_configFICD_RSTPRI["action"] = str_action;
		str_Wr_configFICD_RSTPRI.clear();
		str_Wr_configFICD_RSTPRI = jstr_Wr_configFICD_RSTPRI.dump();
		tx_msg_to_Bootloader_UI(str_Wr_configFICD_RSTPRI);
	}
	else if (ui_cmd == "bl_drv_task")
	{
		std::string str_action = json_msg["action"];
		if (str_action == "LoadHexFile")
		{
			timetag_t t_load_hex;
			time_tag(tag_all, &t_load_hex);

			int res;
			if (!g_str_imgfile_path.empty())
			{
				cout << "img_path:" << g_str_imgfile_path << endl;

				// std::vector<char> v_imgfile_path(g_str_imgfile_path.begin(), g_str_imgfile_path.end());
				// v_imgfile_path.push_back('\0');
				// char *p_imgfile_path = &v_imgfile_path[0];
				// int res_decrypt = rc5_decryptfile(p_imgfile_path);
				// printf("rc5 decrypt file, res:%d\n",res);

				// std::vector<char> v_file_path(g_str_imgfile_path.begin(), g_str_imgfile_path.end());
				// v_file_path.push_back('\0');
				// char *p_file_path = &v_file_path[0];
				// res = bl_LoadHexSour(p_file_path, &handle_hexfile_hexsour);

				// copy encrypted file to temp folder -> decode -> load de-hex file.
				g_str_dest_path.clear();
				if (Copy_ImgFile_To_Folder(g_str_imgfile_path, &g_str_dest_path) == 0)
				{
					std::vector<char> char_name(g_str_dest_path.begin(), g_str_dest_path.end());
					char_name.push_back('\0');
					char *img_filename = &char_name[0];

					if (rc5_decryptfile(img_filename, &g_decrypt_path) == 0)
					{
						printf("deco-file path:%s\n", g_decrypt_path);
						res = bl_LoadHexSour(g_decrypt_path, &handle_hexfile_hexsour);
					}
					else
						res = -11;
				}
				else
					res = -10;
			}
			else
				res = -1;

			std::string str_load_hexfile = "{\"bldriv_fbk\":\"bl_drv_task\",\"action\":\"LoadHexFile\",\"value\":" + to_string(res) + "}";
			tx_msg_to_Bootloader_UI(str_load_hexfile);

			time_tag(tag_delta, &t_load_hex);
			printf("LoadHexFile, execute time:%dms. \n", t_load_hex.delta_tn);
		}
		else if (str_action == "Set_WriteHexThread")
		{
			int res = bl_WrHexRow_Wakeup(&handle_hexfile_hexsour, true);
			std::string str_wrHexThread = "{\"bldriv_fbk\":\"bl_drv_task\",\"action\":\"Set_WriteHexThread\",\"value\":" + to_string(res) + "}";
			tx_msg_to_Bootloader_UI(str_wrHexThread);
		}
		else if (str_action == "Set_WriteHexHalt")
		{
			int res = bl_WrHexRow_Abort();
			std::string str_wrHexAbort = "{\"bldriv_fbk\":\"bl_drv_task\",\"action\":\"Set_WriteHexHalt\",\"value\":" + to_string(res) + "}";
			tx_msg_to_Bootloader_UI(str_wrHexAbort);
		}
		else if (str_action == "GetONSVer_byAux")
		{
			int res = -1;
			int fbk_addr;
			unsigned char fbk_data[48];
			unsigned char fbk_size;
			string ons_vernum, ons_vermsg;
			int ver_temp;

			if (AuxBL_Read_ONS_VersionNumber_Message(0x200, 0, &fbk_addr, fbk_data, &fbk_size) == 0)
			{
				ons_vernum = " RTC-[";
				ver_temp = (0x00ff & (unsigned short)fbk_data[0]);
				ver_temp |= (0xff00 & (unsigned short)fbk_data[1] << 8);
				ons_vernum += to_string(ver_temp) + ",";
				ver_temp = (0x00ff & (unsigned short)fbk_data[2]);
				ver_temp |= (0xff00 & (unsigned short)fbk_data[3] << 8);
				ons_vernum += to_string(ver_temp) + ",";
				ver_temp = (0x00ff & (unsigned short)fbk_data[4]);
				ver_temp |= (0xff00 & (unsigned short)fbk_data[5] << 8);
				ons_vernum += to_string(ver_temp) + "].";

				ons_vernum += " MsgProtocol-[";
				ver_temp = (0x00ff & (unsigned short)fbk_data[6]);
				ver_temp |= (0xff00 & (unsigned short)fbk_data[7] << 8);
				ons_vernum += to_string(ver_temp) + ",";
				ver_temp = (0x00ff & (unsigned short)fbk_data[8]);
				ver_temp |= (0xff00 & (unsigned short)fbk_data[9] << 8);
				ons_vernum += to_string(ver_temp) + ",";
				ver_temp = (0x00ff & (unsigned short)fbk_data[10]);
				ver_temp |= (0xff00 & (unsigned short)fbk_data[11] << 8);
				ons_vernum += to_string(ver_temp) + "].";

				// ons_vernum+= "Bootloader Version[";
				// ver_temp = (0x00ff & (unsigned short)fbk_data[12]);
				// ver_temp |= (0xff00 & (unsigned short)fbk_data[13] << 8);
				// ons_vernum += "[" + to_string(ver_temp) + ",";
				// ver_temp = (0x00ff & (unsigned short)fbk_data[14]);
				// ver_temp |= (0xff00 & (unsigned short)fbk_data[15] << 8);
				// ons_vernum += to_string(ver_temp) + ",";
				// ver_temp = (0x00ff & (unsigned short)fbk_data[16]);
				// ver_temp |= (0xff00 & (unsigned short)fbk_data[17] << 8);
				// ons_vernum += to_string(ver_temp) + "]";
			}

			if (AuxBL_Read_ONS_VersionNumber_Message(0x200, 1, &fbk_addr, fbk_data, &fbk_size) == 0)
			{
				// printf("Verion Message: %s\n",fbk_data);
				// ons_vermsg.assign(&fbk_data[0],48);
				// string s((char*)fbk_data);
				ons_vermsg = ((char *)fbk_data);
				res = 0;
			}

			std::string str_get_onsvernum = "{\"bldriv_fbk\":\"bl_drv_task\",\"action\":\"GetONSVer_byAux\",\"value\":" + to_string(res) + "}";
			json jstr_get_onsvernum = json::parse(str_get_onsvernum);
			jstr_get_onsvernum["ons_vernum"] = ons_vernum;
			jstr_get_onsvernum["ons_vermsg"] = ons_vermsg;
			str_get_onsvernum.clear();
			str_get_onsvernum = jstr_get_onsvernum.dump();
			tx_msg_to_Bootloader_UI(str_get_onsvernum);
		}
	}
	else if (ui_cmd == "bl_drv_terminate")
	{
		int res = 0;
		std::string str_set_bl_driver_terminate = "{\"bldriv_fbk\":\"bl_drv_terminate\",\"value\":" + to_string(res) + "}";
		tx_msg_to_Bootloader_UI(str_set_bl_driver_terminate);
		Set_BL_Driver_Run_Terminate();
	}
	else if (ui_cmd == "bl_drv_update_path")
	{
		int res = 0;
		std::string img_path = json_msg["img_path"];
		std::string ini_path = json_msg["ini_path"];

		Set_Update_Sour_ImgPath(img_path);
		Set_Update_Sour_IniPath(ini_path);
		std::string str_set_bl_driver_update_path = "{\"bldriv_fbk\":\"bl_drv_update_path\",\"value\":" + to_string(res) + "}";
		tx_msg_to_Bootloader_UI(str_set_bl_driver_update_path);

		string inner_imgpath, inner_inipath;
		res = Get_Update_Sour_ImgPath_Status(&inner_imgpath);
		if (res == 0)
			printf("inner img_path:%s\n", inner_imgpath.c_str());

		res = Get_Update_Sour_IniPath_Status(&inner_inipath);
		if (res == 0)
			printf("inner ini_path:%s\n", inner_inipath.c_str());
	}

	std::string str_ack = json_ack.dump();

	return str_ack;
}

HostCmdHandler *get_host_cmd_handler(const std::string &name, NotificationQueue &queue_in, NotificationQueue &queue_out)
{
	if (instance == NULL)
	{
		instance = new HostCmdHandler(name, queue_in, queue_out);
	}
	return instance;
}

void HostCmdHandler::runTask()
{
	while (m_b_running)
	{
		Notification::Ptr pNf(_queue_in.waitDequeueNotification(5));
		if (pNf)
		{
			NotificationFromHost::Ptr pWorkNf = pNf.cast<NotificationFromHost>();
			if (pWorkNf)
			{
				std::string str_msg = pWorkNf->data();

				std::string str_ack = parseCmdPackage(str_msg);

				_queue_out.enqueueNotification(new NotificationToHost(str_ack));
			}
		}
	}
	goDriverLogger.Log("info", "exit command handler");
}
