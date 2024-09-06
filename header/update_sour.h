#ifndef _UPDATE_SOUR_H_
#define _UPDATE_SOUR_H_

#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string>
#include <iostream>
#include <stdexcept>
#include <vector>
#include "Poco/Event.h"

using namespace std;

typedef enum
{
	USB_MSD = 0,
	LOCAL_FOLDER = 1,
	OTA = 2
} update_path_select_t;

typedef enum
{
	FIND_ONE_IMG_FILE_OKAY = 0,
	NONE_USB_MSD = -1,
	NONE_FOLDER = -2,
	NONE_IMG_FILE = -3,
	MULTI_IMG_FILE = -4,
} update_imgfile_status_t;

typedef struct
{
	update_path_select_t sour_select;
	char usb_foldername[64];
	char local_foldername[64];
	char OTA_foldername[128];
	char completed_path[128];
	char image_ext_name[64];
	pthread_t update_sour_check_thread; // check every 0.3sec.
	Poco::Event g_update_sour_check_idle;
	int check_success;
} Update_Souce_Elment_t;

typedef struct
{
	update_imgfile_status_t img_status;
	string imafile_path;
} Update_USB_MSD_Status_t;

typedef struct
{
	string img_path;
	string ini_path;
	int img_status;
	int ini_status;
} Update_Path_Status_t;

int Set_Update_Sour_ImgPath(string img_path);
int Set_Update_Sour_IniPath(string ini_path);
int Get_Update_Sour_ImgPath_Status(string* inner_img_path);
int Get_Update_Sour_IniPath_Status(string* inner_ini_path);

int Update_Sour_Thread_Init(void);
void *Update_Sour_Check_Main(void *p);
int Update_Sour_Check_Wakeup(update_path_select_t sour_select, char* folder_name);
int Update_Sour_Thread_Terminate(void);

int shellcmd(const string &cmd, string &result);

vector<string> string_split(string s,string delimiter);
vector<string> find_usb_path(void);
vector<string> find_usb_path2(void);
int find_usb_folder(string usb_path, string folder_name);
int find_update_img_filepath_with_foldername(string usb_path, string folder_name, string ext_file, string* file_name);
int Update_Sour_Get_ImgFile_Status(string* imgfile_name);

int Update_Sour_find_info(string info_path,string* info_pkg);

int Copy_ImgFile_To_Folder(string sour_path,string* dest_path);
bool copyFile(const char *SRC, const char* DEST);
bool dirExists(const std::string &path);
bool fileExists(const std::string &path);

#endif //_LOGIN_H_
