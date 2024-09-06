// update sour.
// Jan10, 2019.
#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cstring>
#include "update_sour.h"
#include "iniparser.h"
#include "dictionary.h"
#include "json.hpp"
#include <sys/stat.h>
#include <iostream>
#include <sys/types.h>
#include <dirent.h>
#include <iostream>

using namespace std;
using json = nlohmann::json;


char str_updatesour_log[256];
Update_Souce_Elment_t update_sour_elememt; //={USB_MSD,"RTC_UPDATE","LOCAL_UPDATE"};
Update_USB_MSD_Status_t update_usbmsd_status;
Update_Path_Status_t update_path_status;

Poco::Event g_get_imgfile_status_idle;
bool g_b_Update_Sour_run = true;

int Set_Update_Sour_ImgPath(string img_path)
{
	update_path_status.img_path.clear();
	update_path_status.img_path = img_path;
	return 0;
}

int Set_Update_Sour_IniPath(string ini_path)
{
	update_path_status.ini_path.clear();
	update_path_status.ini_path = ini_path;
	return 0;
}

int Get_Update_Sour_ImgPath_Status(string* inner_img_path)
{
	if( !update_path_status.img_path.empty() )
	{
		*inner_img_path = update_path_status.img_path;
		return 0;
	}
	else
		return -1;
}

int Get_Update_Sour_IniPath_Status(string* inner_ini_path)
{
	if( !update_path_status.ini_path.empty() )
	{
		*inner_ini_path = update_path_status.ini_path;
		return 0;
	}
	else
		return -1;
}

int Update_Sour_Thread_Init(void)
{
	int res_update_check_pth = pthread_create(&update_sour_elememt.update_sour_check_thread, NULL, Update_Sour_Check_Main, NULL);
	if (res_update_check_pth != 0)
	{
		sprintf(str_updatesour_log, "%s[%d] create update_sour_check_thread, errno:%d,%s", __func__, __LINE__, res_update_check_pth, strerror(res_update_check_pth));
		string tmp_string(str_updatesour_log);
		printf("%s\n", str_updatesour_log);
	}
	update_sour_elememt.g_update_sour_check_idle.reset();

	update_sour_elememt.sour_select = USB_MSD;
	strcpy(update_sour_elememt.usb_foldername, "RTC_UPDATE");
	strcpy(update_sour_elememt.local_foldername, "LOCAL_UPDATE");
	strcpy(update_sour_elememt.image_ext_name, "img");

	return res_update_check_pth;
}

int Update_Sour_Thread_Terminate(void)
{
	g_b_Update_Sour_run = false;
	update_sour_elememt.g_update_sour_check_idle.set();
	if (pthread_join(update_sour_elememt.update_sour_check_thread, NULL))
        return 0;
    else
        return -1;
}

void *Update_Sour_Check_Main(void *p)
{
	int res;
	int trywait_TIMEOUT = 1000;
	int retry_count = 1;
	int retry;
	int i_usbmsd_num;
	int i_imgfile_num;
	vector<string> vec_usbmsd_path;
	vector<string> vec_imgfile_path;
	vector<string> vec_usbmsd_path_valid;
	string str_imgfile_path;
	string str_temp;
	int imgfile_res;
	int usb_imgfile_res;

	(void)p;
	Poco::Event wait_ms;
	pthread_detach(pthread_self());

	while (g_b_Update_Sour_run==true)
	{
		update_sour_elememt.g_update_sour_check_idle.wait();
		update_sour_elememt.check_success = -1;
		retry = 0;

		if(g_b_Update_Sour_run == false)
			break;

		while (1)
		{
			if (update_sour_elememt.check_success == 0)
				break;
			if (retry > retry_count)
				break;

			vec_usbmsd_path.clear();
			vec_usbmsd_path = find_usb_path2();

			vec_usbmsd_path_valid.clear();
			i_usbmsd_num = vec_usbmsd_path.size();

			vec_imgfile_path.clear();

			if (i_usbmsd_num == 0)
			{
				update_usbmsd_status.img_status = NONE_USB_MSD;
				break;
			}
			else if (i_usbmsd_num > 0)
			{
				i_imgfile_num = 0;
				for (int i = 0; i < i_usbmsd_num; i++)
				{
					// printf("USB_MSD_path[%d]:%s\n",i,vec_usbmsd_path[i].c_str());
					str_temp.clear();
					imgfile_res = find_update_img_filepath_with_foldername(vec_usbmsd_path[i], update_sour_elememt.usb_foldername, update_sour_elememt.image_ext_name, &str_temp);
					if (imgfile_res == 0)
					{
						i_imgfile_num++;
						vec_imgfile_path.push_back(str_temp);
						vec_usbmsd_path_valid.push_back(vec_usbmsd_path[i]);
					}
				}
			}

			if (i_imgfile_num == 0)
				update_usbmsd_status.img_status = NONE_IMG_FILE;
			else if (i_imgfile_num > 1)
				update_usbmsd_status.img_status = MULTI_IMG_FILE;
			if (i_imgfile_num == 1)
			{
				str_imgfile_path = vec_usbmsd_path_valid[0];
				str_imgfile_path += "/";
				str_imgfile_path += string(update_sour_elememt.usb_foldername);
				str_imgfile_path += "/";
				str_imgfile_path += vec_imgfile_path[0];
				update_usbmsd_status.img_status = FIND_ONE_IMG_FILE_OKAY;
				update_usbmsd_status.imafile_path.clear();
				update_usbmsd_status.imafile_path = str_imgfile_path;
				update_sour_elememt.check_success = 0;
			}
			break;
			retry++;
		}
		// printf("update_img_status:%d\n",update_usbmsd_status.img_status);
		if (update_usbmsd_status.img_status == 0)
		{
			if (update_usbmsd_status.imafile_path.empty())
				cout << "cannot found" << endl;
		}
		else
			cout << "cannot found..." << endl;

		g_get_imgfile_status_idle.set();
	}
	printf("Update_Sour_Check_Main_Terminated\n");
	pthread_exit(NULL);
}

int Update_Sour_Check_Wakeup(update_path_select_t sour_select, char *folder_name)
{ // trigger from UI.
	strcpy(update_sour_elememt.completed_path, "");
	update_sour_elememt.g_update_sour_check_idle.set();
	return 0;
}

int Copy_ImgFile_To_Folder(string sour_path,string* dest_path)
{
	int res;
	vector<string> vec_imgpath;
	string str_imgtemp_folder="img_temp";
	string str_rm_cmd_img = "rm img_temp/*.img";
	string str_rm_cmd_deh = "rm img_temp/*.deh";
	string str_mkdir_cmd = "mkdir img_temp";
	string str_cp_cmd = "cp ";
	string str_specific_filename = "img_temp";
	string line;
	string str_shell_res;
	string str_dest_path;
	if(sour_path.empty())
		return -1;
	printf("sour_path:%s\n",sour_path.c_str());

	vec_imgpath.clear();
	vec_imgpath = string_split(sour_path, "/");

	//if( vec_imgpath.size() != 2 )
	//	return -2;

	//for(int i = 0;i<vec_imgpath.size();i++)
	//	printf("%s\n",vec_imgpath[i].c_str());

	if( dirExists(str_imgtemp_folder) )
	{
		shellcmd(str_rm_cmd_img, str_shell_res); //remove files.
		shellcmd(str_rm_cmd_deh, str_shell_res); //remove files.
	}
	else
		shellcmd(str_mkdir_cmd, str_shell_res); // mkdir

	str_cp_cmd+=sour_path+" ";
	str_cp_cmd+=str_imgtemp_folder+"/"+vec_imgpath[vec_imgpath.size()-1];
	
	str_dest_path=str_imgtemp_folder+"/"+vec_imgpath[vec_imgpath.size()-1];
	*dest_path=str_dest_path;

	//cout<<"cp cmd:"<<str_cp_cmd<<endl;
	//cout<<"dest:"<<str_dest_path<<endl;
	
	shellcmd(str_cp_cmd, str_shell_res); // copy img file to 'img_temp' folder.
	return 0;
}

// copy in binary mode
bool copyFile(const char *SRC, const char* DEST)
{
    std::ifstream src(SRC, std::ios::binary);
    std::ofstream dest(DEST, std::ios::binary);
    dest << src.rdbuf();
    return src && dest;
}

int Update_Sour_Get_ImgFile_Status(string *imgfile_name)
{
	int res;
	int wait_res;
	g_get_imgfile_status_idle.reset();

	Update_Sour_Check_Wakeup(USB_MSD, (char *)"");
	wait_res = g_get_imgfile_status_idle.tryWait(50);
	if (wait_res == 1)
	{
		if (update_usbmsd_status.img_status == 0)
			*imgfile_name = update_usbmsd_status.imafile_path;
		res = update_usbmsd_status.img_status;
	}
	else
	{
		res = -10;
		printf("Get_ImgFile timeout\n");
	}
	return res;
}

int shellcmd(const string &cmd, string &result)
{
	char buffer[512];
	result = "";
	// printf("shell_cmd:%s\n",cmd.c_str() );

	FILE *pipe = popen(cmd.c_str(), "r");
	if (!pipe)
		return -1;

	while (!feof(pipe))
	{
		if (fgets(buffer, sizeof(buffer), pipe) != NULL)
			result += buffer;
	}
	pclose(pipe);
	return 0;
}

vector<string> string_split(string s, string delimiter)
{
	// string delimiter ="\n";
	size_t pos_start = 0, pos_end, delim_len = delimiter.length();
	string token;
	vector<string> res;

	while ((pos_end = s.find(delimiter, pos_start)) != string::npos)
	{
		token = s.substr(pos_start, pos_end - pos_start);
		pos_start = pos_end + delim_len;
		res.push_back(token);
	}

	res.push_back(s.substr(pos_start));
	return res;
}

vector<string> find_usb_path(void)
{ // cannot find " "(space) path.
	string str_lsblk_cmd;
	string str_lsblk_res;

	vector<string> vec_line;
	vector<string> vec_line_part;
	vector<string> vec_line_media;
	vector<string> vec_res;

	string str_temp;
	vector<string> vec_temp;
	shellcmd("lsblk -l", str_lsblk_res);

	vec_line.clear();
	vec_line = string_split(str_lsblk_res, "\n");

	for (int i = 0; i < vec_line.size(); i++)
	{
		vec_line_part.clear();
		vec_line_part = string_split(vec_line[i], " ");

		for (int j = 0; j < vec_line_part.size(); j++)
		{
			if (vec_line_part[j] == "part")
			{
				str_temp.clear();
				str_temp = vec_line_part[j + 1];

				vec_temp.clear();
				vec_temp = string_split(str_temp, "/");

				if (vec_temp.size() > 2)
				{
					if (vec_temp[1] == "media")
						vec_res.push_back(vec_line_part[j + 1]);
				}
				j++;
			}
		}
	}

	return vec_res;
}

vector<string> find_usb_path2(void)
{ // cannot find " "(space) path.
	string str_lsblk_cmd;
	string str_lsblk_res;

	vector<string> vec_line;
	vector<string> vec_line_part;
	vector<string> vec_line_disk;
	vector<string> vec_line_media;
	vector<string> vec_res;

	string str_temp;
	vector<string> vec_temp;
	shellcmd("lsblk -l", str_lsblk_res);
	// cout<<str_lsblk_res<<endl;

	vec_line.clear();
	vec_line = string_split(str_lsblk_res, "\n");

	for (int i = 0; i < vec_line.size(); i++)
	{
		vec_line_part.clear();
		vec_line_part = string_split(vec_line[i], "part ");

		if (vec_line_part.size() > 1)
		{
			if (!vec_line_part[1].empty())
			{
				vec_temp.clear();
				vec_temp = string_split(vec_line_part[1], "/");

				if (vec_temp[1] == "media")
					vec_res.push_back(vec_line_part[1]);
			}
		}

		vec_line_disk.clear();
		vec_line_disk = string_split(vec_line[i], "disk ");

		if (vec_line_disk.size() > 1)
		{
			if (!vec_line_disk[1].empty())
			{
				vec_temp.clear();
				vec_temp = string_split(vec_line_disk[1], "/");

				if (vec_temp[1] == "media")
					vec_res.push_back(vec_line_disk[1]);
			}
		}
	}
	return vec_res;
}

int find_usb_folder(string usb_path, string folder_name)
{
	int res = -1;
	vector<string> vec_path_temp = string_split(usb_path, " "); // if size > 1, the path with " "(space).
	string str_path_temp;
	string shell_cmd = "ls -F ";
	string str_folder_res;
	string str_folder_comp = folder_name + "/";
	vector<string> vec_folder_temp;
	// cout<<"usb_path::"<<usb_path<<endl;
	// cout<<"pathsize:"<<vec_path_temp.size()<<endl;

	if (vec_path_temp.size() == 1)
		shell_cmd += usb_path;
	else
	{
		for (int i = 0; i < vec_path_temp.size(); i++)
		{
			shell_cmd += vec_path_temp[i];
			if (i == vec_path_temp.size() - 1)
				break;
			shell_cmd += "\\ "; // if path with space, added "\".
		}
	}

	shellcmd(shell_cmd, str_folder_res);
	vec_folder_temp = string_split(str_folder_res, "\n");
	for (int i = 0; i < vec_folder_temp.size(); i++)
	{
		cout << vec_folder_temp[i] << ",";
		if (vec_folder_temp[i] == str_folder_comp)
			res = 0;
	}
	cout << endl;

	return res;
}

int find_update_img_filepath_with_foldername(string usb_path, string folder_name, string ext_file, string *file_name)
{
	int res = -1;
	// res = -1; cannot find specific folder 'folder_name'.
	// res = -2; cannot find image file in the specific folder.
	// res = -3; find out more than 1 image file in the specific folder.
	// res =  0; only one image file in in the specific folder.

	int find_case = 0;
	int extfile_num = 0;
	vector<string> vec_path_temp = string_split(usb_path, " "); // if size > 1, the path with " "(space).
	string cmd_findfolder = "ls -F ";
	string cmd_findfile = "ls ";
	string str_folder_res;
	string str_filename_res;
	string str_folder_comp = folder_name + "/";
	string str_extfile_comp = "." + ext_file;
	string str_update_img_file_name;
	vector<string> vec_folder_temp;
	vector<string> vec_file_temp;
	vector<string> vec_extfile_temp;
	vector<string> vec_imgfile_temp;
	std::size_t extfile_found;

	if (vec_path_temp.size() == 1)
	{
		cmd_findfolder += usb_path;
		cmd_findfile += usb_path;
	}
	else
	{
		for (int i = 0; i < vec_path_temp.size(); i++)
		{
			cmd_findfolder += vec_path_temp[i];
			cmd_findfile += vec_path_temp[i];
			if (i == vec_path_temp.size() - 1)
				break;
			cmd_findfolder += "\\ "; // if path with space, added "\".
			cmd_findfile += "\\ ";
		}
	}

	while (1)
	{
		if (find_case < 0)
			break;
		switch (find_case)
		{
		case 0: // find the specific folder name.
			shellcmd(cmd_findfolder, str_folder_res);
			vec_folder_temp = string_split(str_folder_res, "\n");

			for (int i = 0; i < vec_folder_temp.size(); i++)
			{
				if (vec_folder_temp[i] == str_folder_comp)
					find_case++;
			}

			if (find_case == 0)
			{
				find_case = -1;
				res = -1;
			}
			break;

		case 1: // find finds in the specific folder.
			cmd_findfile += "/";
			cmd_findfile += folder_name;
			shellcmd(cmd_findfile, str_filename_res);
			vec_file_temp = string_split(str_filename_res, "\n");

			if (vec_file_temp.size() > 1)
				find_case++;
			else
			{
				find_case = -2;
				res = -2;
			}
			break;

		case 2: // find specific ext-file name in the specific folder.
			extfile_num = 0;
			for (int i = 0; i < vec_file_temp.size(); i++)
			{
				vec_extfile_temp.clear();
				vec_extfile_temp = string_split(vec_file_temp[i], ".");

				if (vec_extfile_temp.size() > 1)
				{
					if (vec_extfile_temp[1] == ext_file)
					{
						vec_imgfile_temp.push_back(vec_file_temp[i]);
						extfile_num++;
					}
				}
			}

			if (extfile_num == 0)
			{
				find_case = -2;
				res = -2;
			}
			else if (extfile_num > 1)
			{
				find_case = -3;
				res = -3;
			}
			else
			{
				find_case++;
				*file_name = vec_imgfile_temp[0];
				res = 0;
			}
			break;

		default:
			find_case = -1;
			break;
		}
	}
	return res;
}

int Update_Sour_find_info(string info_path, string *info_pkg)
{
	string str_info;
	dictionary *info;
	info = iniparser_load(info_path.c_str());
	if (info == NULL)
	{
		printf("iniparser_load is NULL\n");
		return -1;
	}

	iniparser_dump(info, stdout);
	printf("\n\n");

	json j_info_pkg;

	if (iniparser_find_entry(info, "rtc_update:img_file_brief"))
		j_info_pkg["img_file_brief"] = iniparser_getstring(info, "rtc_update:img_file_brief", "");

	if (iniparser_find_entry(info, "rtc_update:img_file_description"))
		j_info_pkg["img_file_description"] = iniparser_getstring(info, "rtc_update:img_file_description", "");

	if (iniparser_find_entry(info, "rtc_update:img_file_revise_date"))
		j_info_pkg["img_file_revise_date"] = iniparser_getstring(info, "rtc_update:img_file_revise_date", "");

	if (iniparser_find_entry(info, "rtc_update:img_file_version"))
		j_info_pkg["img_file_version"] = iniparser_getstring(info, "rtc_update:img_file_version", "");

	str_info = j_info_pkg.dump();
	*info_pkg = str_info;

	// img_file_brief = RTC bootloader test file.
	// img_file_description = 1.bootloader API in auxiliary segment, 2.bootloader hex include with ONS application API.
	// img_file_revise_date = Feb 21, 2021.
	// img_file_version     = rtc_update.21.02.a
	iniparser_freedict(info);
	return 0;
}

bool dirExists(const std::string &path)
{
    struct stat info;
    if (stat(path.c_str(), &info) == 0 && info.st_mode & S_IFDIR) {
        return true;
    }
    return false;
}

bool fileExists(const std::string &path)
{
	bool res = false;
	if( FILE *file = fopen(path.c_str(),"r"))
	{
		fclose(file);
		res = true;
	}
	else
		res = false;
	return res;
}