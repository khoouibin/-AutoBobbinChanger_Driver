#include "cli.h"
#include <iostream>
#include <string>
#include <vector>
#include <termios.h>
#include <errno.h>
#include <entity_io.h>
#include <bitset>
#include <iomanip>
//#include "USBComm.h"
//#include "USBComm_Driver.h"

#include "interface_to_host.h"
#include "interface_driver_to_ui.h"
#include "Mqtt_prot.h"
#include "notification_pkg.h"
#include "httpinterface.h"
#include "json.hpp"
//#include "return_code.h"
#include "USBComm.h"
#include "USBComm_Driver.h"
#include "BL_USBComm.h"
#include "update_sour.h"

using namespace std;
hex_sourcefile_t hexfile_hexsour;
vector<unsigned char> v_rc5_crypt, v_rc5_uncrypt;
string str_imgfile_path;
string str_dest_path;
string g_inner_imgpath, g_inner_inipath;
char *decrypt_path = NULL;

vector<string> split(string s, string delimiter)
{
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

vector<string> helper_doc(void)
{
    vector<string> doc;
    doc.push_back(" common command:");
    doc.push_back(" -----------------------------------------------------------------");
    doc.push_back(" 'usb list'                   : get usb device list. ");
    doc.push_back(" 'usb auto_open'              : open UsbDevice ONS_RTC/BL_RTC auto-select. ");
    doc.push_back(" ");
    doc.push_back(" 'onscmd ver'                 : get ons version number and set ons to uninstalled mode.");
    doc.push_back(" 'onscmd init'                : set ons to ready mode.");
    doc.push_back(" 'onscmd rdconfig'            : read ons config regitster @FICD");
    doc.push_back(" 'onscmd rdconfig addr'       : read ons config reg @ addr.");
    doc.push_back(" 'onscmd wrconfig'            : write ons config reg to AuxSeg @FICD");
    doc.push_back(" 'onscmd wrconfig addr value' : write ons config reg value @addr");
    doc.push_back(" 'onscmd resetpic'            : reset pic with verify temp config reg");
    doc.push_back(" 'onscmd resetpic 0/1'        : reset pic with 0:direct / 1:verify config reg");
    doc.push_back(" ");
    doc.push_back(" 'blcmd nop'                  : bootloader protocol test.");
    doc.push_back(" 'blcmd nop addr size'        : protocol test, addr:0x00~0xff ff ff, size:1 legal, others illegal.");
    doc.push_back(" 'blcmd nop addr size'        : protocol test, addr:0x00~0xff ff ff, size:1 legal, others illegal.");
    doc.push_back(" 'blcmd rdmem'                : read pic flash memory addr: 0x0557C0, size:32bytes");
    doc.push_back(" 'blcmd rdmem addr size'      : read pic flash memory addr: 0x00~0xff ff ff, size: 4~32");
    doc.push_back(" 'blcmd wrmem'                : write pic flash memory addr: 0x0557C0, size:32bytes");
    doc.push_back(" 'blcmd wrmem addr size'      : write pic flash memory addr: 0x00~0xff ff ff, size: 4~32");
    doc.push_back("        (clear flash memory before writing) ");
    doc.push_back(" 'blcmd clrmem'               : clear general segment flash memory in pic");
    doc.push_back(" 'blcmd rdconfig'             : read config regitster @FICD");
    doc.push_back(" 'blcmd rdconfig addr'        : read config regitster @addr");
    doc.push_back(" 'blcmd wrconfig'             : write config reg to GenSeg @FICD");
    doc.push_back(" 'blcmd wrconfig addr'        : write config regitster @addr");
    doc.push_back(" 'blcmd resetpic'             : reset pic directly");
    doc.push_back(" 'blcmd rdonsver'             : read pic flash memory @200h shows ons version number");
    doc.push_back(" 'blcmd rdchksum'             : get chksum of pic general segment flash memory");
    doc.push_back(" 'hex load file.hex'          : load hex file to temporary");
    doc.push_back(" 'hex send'                   : wake up send hex thread");
    doc.push_back(" 'hex chksum'                 : calculate chksum gen/aux from hex temporary");
    doc.push_back(" Press 'q' to quit.");

    for (int i = 0; i < doc.size(); i += 1)
        cout << doc[i] << endl;

    return doc;
}

int kbhit(void)
{
    struct timeval tv;
    struct termios old_termios, new_termios;
    int error;
    int count = 0;
    tcgetattr(0, &old_termios);
    new_termios = old_termios;
    new_termios.c_lflag &= ~ICANON;
    new_termios.c_lflag &= ~ECHO;
    new_termios.c_cc[VMIN] = 1;
    new_termios.c_cc[VTIME] = 0;
    error = tcsetattr(0, TCSANOW, &new_termios);
    tv.tv_sec = 0;
    tv.tv_usec = 100;
    select(1, NULL, NULL, NULL, &tv);
    error += ioctl(0, FIONREAD, &count);
    error += tcsetattr(0, TCSANOW, &old_termios);
    return error == 0 ? count : -1;
}

extern std::string parseCmdPackage(const std::string &pkg);

void CommandLineInterface()
{
    int iUsbStatus;
    int cli_break = 0;
    std::string command_line;
    string delimiter = " ";
    std::cout << endl;
    std::cout << " press 'help' to read A simple guide. " << endl;
    std::cout << ">>";

    while (cli_break == 0)
    {
        getline(cin, command_line);

        vector<string> v = split(command_line, delimiter);
        std::string cmd;
        if (v.size() == 0)
        {
            break;
        }
        else
        {
            cmd = v[0];
        }

        if (cmd == "q")
        {
            cli_break = 1;
        }
        else if (cmd == "help")
        {
            helper_doc();
        }
        else if (cmd == "usb")
        {
            if (v.size() > 1)
            {
                if (v.size() >= 2)
                {
                    if (v[1] == "list")
                    {
                        int dev_num, target_device;
                        dev_num = BL_USBComm_Driver_GetDeviceList(&target_device, 1);

                        if (target_device == -1)
                            printf("device_number:%d,target device: DeviceNull \n", dev_num);
                        else
                        {
                            // res_open_dev = BL_USBComm_Driver_openTargetDevice((UsbDevice) res_target_device);
                            printf("device_number:%d,target device: %d\n", dev_num, target_device );
                        }
                    }
                    else if (v[1] == "show_target")
                    {
                        int usb_target_dev = BL_USBComm_Driver_getTargetDevice();
                        printf("usb_target_dev:%d\n",usb_target_dev);
                    }
                    else if (v[1] == "open_device")
                    {
                        if (v.size() != 3)
                        {
                            printf("usb open_device input size wrong, try again.\n");
                            continue;
                        }

                        int res;
                        if (v[2] == "abc")
                        {
                            res = BL_USBComm_Driver_openTargetDevice(Device_ABC_RTC);
                            printf("usb open_device ons:%d\n", res);
                        }
                        else
                        {
                            printf("usb open_device command Fault, please try again\n");
                            continue;
                        }
                    }
                    else if (v[1] == "auto_open")
                    {
                        int dev_num, target_device;
                        int res_open_dev;
                        dev_num = BL_USBComm_Driver_GetDeviceList(&target_device, 1);

                        if (target_device == -1)
                            printf("device_number:%d,target device: DeviceNull \n", dev_num);
                        else
                        {
                            res_open_dev = BL_USBComm_Driver_openTargetDevice((UsbDevice)target_device);
                            printf("device_number:%d,target device: %d, open_device:%d\n", dev_num, target_device , res_open_dev);
                        }
                    }
                    else
                        printf("usb cmd  wrong, try again.\n");
                }
            }
        }
        // else if (cmd == "onscmd")
        // {
        //     int res;
        //     int rtc_base = -1, rtc_major = -1, rtc_minor = -1, prot_base = -1, prot_major = -1, prot_minor = -1;
        //     int rtc_init_status;

        //     unsigned int reg_addr, req_size;
        //     unsigned int fbk_addr;
        //     unsigned char wr_byte, fbk_val;

        //     unsigned char reset_mode, fbk_reset_mode;

        //     unsigned char rd_seg_chksum;
        //     unsigned short res_chksum;

        //     if (v.size() >= 2)
        //     {
        //         if (v[1] == "ver")
        //         {
        //             res = AuxBL_ONS_VerionReq(&rtc_base, &rtc_major, &rtc_minor, &prot_base, &prot_major, &prot_minor);
        //             if (res == 0)
        //             {
        //                 printf("RtcVer:[%d,%d,%d], ProtVer:[%d,%d,%d]\n", rtc_base, rtc_major, rtc_minor, prot_base, prot_major, prot_minor);
        //             }
        //             else
        //                 printf("ONS_VerionReq Fault\n");
        //         }
        //         else if (v[1] == "init")
        //         {
        //             if (AuxBL_ONS_InitReq(&rtc_init_status) == 0)
        //             {
        //                 printf("RtcInitStatus:%d\n", rtc_init_status);
        //             }
        //             else
        //                 printf("ONS_InitReq Fault\n");
        //         }
        //         else if (v[1] == "rdconfig")
        //         {
        //             if (v.size() == 2) // onscmd rdconfig
        //             {
        //                 reg_addr = 0x00F8000E;
        //                 req_size = 1;
        //             }
        //             else if (v.size() == 3) // onscmd rdconfig addr
        //             {
        //                 req_size = 1;
        //                 reg_addr = std::stoi(v[2], 0, 16);
        //             }
        //             if (AuxBL_ONS_Read_ConfigReg_Req(reg_addr, &fbk_addr, &fbk_val) == 0)
        //             {
        //                 printf("Fbk_Addr:0x%06X, ConfigReg:0x%02X\n", fbk_addr, fbk_val);
        //             }
        //             else
        //                 printf("ONS_Read_ConfigRegReq Fault\n");
        //         }
        //         else if (v[1] == "wrconfig")
        //         {                      // usb tx ons wrconfig addr XX
        //             if (v.size() == 2) // onscmd wrconfig
        //             {                  // write configuration register FICD - RSTPRI.
        //                 req_size = 1;
        //                 reg_addr = 0x00F8000E;
        //                 wr_byte = 0xdb;
        //             }
        //             else if (v.size() == 4) // onscmd wrconfig addr wr_byte
        //             {
        //                 req_size = 1;
        //                 reg_addr = std::stoi(v[2], 0, 16);
        //                 wr_byte = std::stoi(v[3], 0, 16);
        //             }
        //             if (AuxBL_ONS_Write_ConfigReg_Req(reg_addr, wr_byte, &fbk_addr, &fbk_val) == 0)
        //             {
        //                 printf("Fbk_Addr:0x%06X, ConfigReg:0x%02X\n", fbk_addr, fbk_val);
        //             }
        //             else
        //                 printf("ONS_Write_ConfigRegReq Fault\n");
        //         }
        //         else if (v[1] == "resetpic")
        //         {
        //             if (v.size() == 2) // onscmd resetpic
        //             {                  // reset_mode 0/1
        //                 reset_mode = 1;
        //             }
        //             else if (v.size() == 3) // onscmd resetpic 0/1
        //             {
        //                 reset_mode = (unsigned char)std::stoi(v[2]);
        //             }
        //             if (AuxBL_ONS_Reset_PIC_Req(reset_mode, &fbk_reset_mode) == 0)
        //             {
        //                 printf("Reset PIC mode:%d\n", fbk_reset_mode);
        //             }
        //             else
        //                 printf("ONS_Reset_Req Fault\n");
        //         }
        //         else if (v[1] == "rdchksum")
        //         {
        //             if (v.size() == 2)
        //             {
        //                 rd_seg_chksum = 0;
        //             }
        //             else if (v.size() == 3)
        //             {
        //                 rd_seg_chksum = (unsigned char)std::stoi(v[2]);
        //             }
        //             else
        //             {
        //                 printf("ONS Read Segment Checksum command error, try again.\n");
        //                 continue;
        //             }

        //             res = AuxBL_ONS_Segment_Checksum_Req(rd_seg_chksum, &res_chksum);
        //             if (res == 0)
        //             {
        //                 printf("ONS Read Segment Checksum: 0x%04x\n", res_chksum);
        //             }
        //             else
        //                 printf("ONS Read Segment Checksum Fault\n");
        //         }
        //         else
        //         {
        //             printf("ONS Command not match\n");
        //         }
        //     }
        // }
        // else if (cmd == "blcmd")
        // {
        //     int res;
        //     int cmd_addr, cmd_size;
        //     int fbk_addr;
        //     unsigned char fbk_size, fbk_chksum;
        //     unsigned char fbk_data[48];
        //     unsigned char wrdata[32] = {0x00, 0x01, 0x02, 0x00, 0x04, 0x05, 0x06, 0x00,
        //                                 0x10, 0x11, 0x12, 0x00, 0x14, 0x15, 0x16, 0x00,
        //                                 0x20, 0x21, 0x22, 0x00, 0x24, 0x25, 0x26, 0x00,
        //                                 0x30, 0x31, 0x32, 0x00, 0x34, 0x35, 0x36, 0x00};
        //     unsigned char config_reg;
        //     unsigned char rd_seg_chksum;
        //     unsigned short res_chksum;

        //     unsigned char rd_ons_version_sel;
        //     ONS_VersionNum_t version_number;
        //     unsigned char ons_version_msg[48];

        //     if (v[1] == "nop")
        //     {
        //         if (v.size() == 2)
        //         {
        //             cmd_addr = ADDR_MASK;
        //             cmd_size = 0;
        //         }
        //         else if (v.size() == 4)
        //         {
        //             cmd_addr = std::stoi(v[2], 0, 16);
        //             cmd_size = std::stoi(v[3]);
        //         }
        //         else
        //         {
        //             printf("AuxBL NOP command error, try again.\n");
        //             continue;
        //         }
        //         res = AuxBL_NOP_Message(cmd_addr, cmd_size, &fbk_addr, &fbk_size);
        //         if (res == 0)
        //         {
        //             printf("AuxBL NOP:addr:0x%08x, size[%d]\n", fbk_addr, fbk_size);
        //         }
        //         else
        //             printf("AuxBL NOP Fault\n");
        //     }
        //     else if (v[1] == "rdmem")
        //     {
        //         if (v.size() == 2)
        //         {
        //             cmd_addr = ADDR_GenSeg_Test1; // 0x0557C0
        //             cmd_size = 32;
        //         }
        //         else if (v.size() == 4)
        //         {
        //             cmd_addr = std::stoi(v[2], 0, 16);
        //             cmd_size = std::stoi(v[3]);
        //         }
        //         else
        //         {
        //             printf("AuxBL_Read_PIC_FlashMemory command error, try again.\n");
        //             continue;
        //         }
        //         res = AuxBL_ReadFlashMemory_Message(cmd_addr, cmd_size, &fbk_addr, fbk_data, &fbk_size);
        //         if (res == 0)
        //         {
        //             printf("AuxBL_Read_PIC_FlashMemory:addr:0x%08x, size[%d]\n", fbk_addr, fbk_size);
        //             for (int i = 0; i < fbk_size; i++)
        //             {
        //                 printf("0x%02x", fbk_data[i]);
        //                 if ((i + 1) == 8 || (i + 1) == 16 || (i + 1) == 24 || (i + 1) == 32)
        //                     printf("\n");
        //                 else
        //                     printf(", ");
        //             }
        //             printf("\n");
        //         }
        //         else
        //         {
        //             printf("AuxBL Read Memory Fault\n");
        //         }
        //     }
        //     else if (v[1] == "wrmem")
        //     {
        //         if (v.size() == 2)
        //         {
        //             cmd_addr = ADDR_GenSeg_Test1;
        //             cmd_size = 32;
        //         }
        //         else if (v.size() == 4)
        //         {
        //             cmd_addr = std::stoi(v[2], 0, 16);
        //             cmd_size = std::stoi(v[3]);
        //         }
        //         else
        //         {
        //             printf("AuxBL_Write_PIC_FlashMemory command error, try again.\n");
        //             continue;
        //         }

        //         res = AuxBL_WriteFlashMemory_Message(cmd_addr, cmd_size, wrdata, &fbk_addr, &fbk_size, &fbk_chksum);
        //         if (res == 0)
        //         {
        //             printf("AuxBL_Write_PIC_FlashMemory:addr:0x%08x, size[%d], 0x%02x\n", fbk_addr, fbk_size, fbk_chksum);
        //         }
        //         else
        //         {
        //             printf("AuxBL Write Memory Fault\n");
        //         }
        //     }
        //     else if (v[1] == "clrmem")
        //     {
        //         if (v.size() == 2)
        //         {
        //             cmd_addr = ADDR_MASK;
        //             cmd_size = 0;
        //         }
        //         else if (v.size() == 4)
        //         {
        //             cmd_addr = std::stoi(v[2], 0, 16);
        //             cmd_size = std::stoi(v[3]);
        //         }
        //         else
        //         {
        //             printf("AuxBL Clear General Segment Memory command error, try again.\n");
        //             continue;
        //         }

        //         res = AuxBL_Clear_GenFlash_Message(cmd_addr, cmd_size, &fbk_addr, &fbk_size);
        //         if (res == 0)
        //         {
        //             printf("AuxBL Clear General Segment Memory:addr:0x%08x, size[%d]\n", fbk_addr, fbk_size);
        //         }
        //         else
        //         {
        //             printf("AuxBL Clear General Segment Fault\n");
        //         }
        //     }
        //     else if (v[1] == "rdconfig")
        //     {
        //         if (v.size() == 2)
        //         {
        //             cmd_addr = ADDR_Config_FICD;
        //             cmd_size = 1;
        //         }
        //         else if (v.size() == 3)
        //         {
        //             cmd_addr = std::stoi(v[2], 0, 16);
        //             // cmd_size = std::stoi(v[3]);
        //             cmd_size = 1;
        //         }
        //         else
        //         {
        //             printf("AuxBL Read Configuration Register command error, try again.\n");
        //             continue;
        //         }

        //         res = AuxBL_ReadConfigReg_Message(cmd_addr, cmd_size, &fbk_addr, &config_reg, &fbk_size);
        //         if (res == 0)
        //         {
        //             printf("AuxBL Read Configuration Register: addr:0x%08x, size[%d], reg:0x%02x\n", fbk_addr, fbk_size, config_reg);
        //         }
        //     }
        //     else if (v[1] == "wrconfig")
        //     {
        //         if (v.size() == 2)
        //         {
        //             cmd_addr = ADDR_Config_FICD;
        //             cmd_size = 1;
        //             config_reg = 0xdb;
        //             config_reg |= BIT_RSTPRI;
        //         }
        //         else if (v.size() == 4)
        //         {
        //             cmd_addr = std::stoi(v[2], 0, 16);
        //             cmd_size = 1;
        //             config_reg = std::stoi(v[4], 0, 16);
        //         }
        //         else
        //         {
        //             printf("AuxBL Write Configuration Register command error, try again.\n");
        //             continue;
        //         }

        //         res = AuxBL_WriteConfigReg_Message(cmd_addr, cmd_size, config_reg, &fbk_addr, &fbk_size);
        //         if (res == 0)
        //         {
        //             printf("AuxBL Write Configuration Register: addr:0x%08x, size[%d]\n", fbk_addr, fbk_size);
        //         }
        //     }
        //     else if (v[1] == "resetpic")
        //     {
        //         if (v.size() == 2)
        //         {
        //             cmd_addr = ADDR_MASK;
        //             cmd_size = 0;
        //         }
        //         else if (v.size() == 4)
        //         {
        //             cmd_addr = std::stoi(v[2], 0, 16);
        //             cmd_size = std::stoi(v[3]);
        //         }
        //         else
        //         {
        //             printf("AuxBL RESET command error, try again.\n");
        //             continue;
        //         }
        //         res = AuxBL_Reset_Message(cmd_addr, cmd_size, &fbk_addr, &fbk_size);
        //         if (res == 0)
        //         {
        //             printf("AuxBL RESET:addr:0x%08x, size[%d]\n", fbk_addr, fbk_size);
        //         }
        //         else
        //             printf("AuxBL RESET Fault\n");
        //     }
        //     else if (v[1] == "rdchksum")
        //     {
        //         if (v.size() == 2)
        //         {
        //             rd_seg_chksum = 0;
        //         }
        //         else if (v.size() == 3)
        //         {
        //             rd_seg_chksum = (unsigned char)std::stoi(v[2]);
        //         }
        //         else
        //         {
        //             printf("AuxBL Read Segment Checksum command error, try again.\n");
        //             continue;
        //         }

        //         res = AuxBL_Read_Segment_Checksum_Message(rd_seg_chksum, &res_chksum);
        //         if (res == 0)
        //         {
        //             printf("AuxBL Read Segment Checksum: 0x%04x\n", res_chksum);
        //         }
        //         else
        //             printf("AuxBL Read Segment Checksum Fault\n");
        //     }
        //     else if (v[1] == "rdonsver")
        //     {
        //         if (v.size() == 2)
        //         {
        //             cmd_addr = 0x200;
        //             unsigned short ver_temp;
        //             res = AuxBL_Read_ONS_VersionNumber_Message(cmd_addr, 0, &fbk_addr, fbk_data, &fbk_size);
        //             if (res == 0)
        //             {
        //                 ver_temp = (0x00ff & (unsigned short)fbk_data[0]);
        //                 ver_temp |= (0xff00 & (unsigned short)fbk_data[1] << 8);
        //                 version_number.RTC_Base = ver_temp;
        //                 ver_temp = (0x00ff & (unsigned short)fbk_data[2]);
        //                 ver_temp |= (0xff00 & (unsigned short)fbk_data[3] << 8);
        //                 version_number.RTC_Major = ver_temp;
        //                 ver_temp = (0x00ff & (unsigned short)fbk_data[4]);
        //                 ver_temp |= (0xff00 & (unsigned short)fbk_data[5] << 8);
        //                 version_number.RTC_Minor = ver_temp;
        //                 ver_temp = (0x00ff & (unsigned short)fbk_data[6]);
        //                 ver_temp |= (0xff00 & (unsigned short)fbk_data[7] << 8);
        //                 version_number.MsgProt_Base = ver_temp;
        //                 ver_temp = (0x00ff & (unsigned short)fbk_data[8]);
        //                 ver_temp |= (0xff00 & (unsigned short)fbk_data[9] << 8);
        //                 version_number.MsgProt_Major = ver_temp;
        //                 ver_temp = (0x00ff & (unsigned short)fbk_data[10]);
        //                 ver_temp |= (0xff00 & (unsigned short)fbk_data[11] << 8);
        //                 version_number.MsgProt_Minor = ver_temp;
        //                 ver_temp = (0x00ff & (unsigned short)fbk_data[12]);
        //                 ver_temp |= (0xff00 & (unsigned short)fbk_data[13] << 8);
        //                 version_number.BL_Base = ver_temp;
        //                 ver_temp = (0x00ff & (unsigned short)fbk_data[14]);
        //                 ver_temp |= (0xff00 & (unsigned short)fbk_data[15] << 8);
        //                 version_number.BL_Major = ver_temp;
        //                 ver_temp = (0x00ff & (unsigned short)fbk_data[16]);
        //                 ver_temp |= (0xff00 & (unsigned short)fbk_data[17] << 8);
        //                 version_number.BL_Minor = ver_temp;
        //                 printf("Addr: 0x0x%08x, size:%d, RtcVer:[%d,%d,%d], ProtVer:[%d,%d,%d], BootloaderVer:[%d,%d,%d]\n", fbk_addr, fbk_size,
        //                        version_number.RTC_Base, version_number.RTC_Major, version_number.RTC_Minor,
        //                        version_number.MsgProt_Base, version_number.MsgProt_Major, version_number.MsgProt_Minor,
        //                        version_number.BL_Base, version_number.BL_Major, version_number.BL_Minor);
        //             }

        //             res = AuxBL_Read_ONS_VersionNumber_Message(cmd_addr, 1, &fbk_addr, fbk_data, &fbk_size);
        //             if (res == 0)
        //             {
        //                 printf("Verion Message: %s\n", fbk_data);
        //             }
        //         }
        //         else
        //         {
        //             printf("AuxBL Read ONS Version Number command error, try again.\n");
        //             continue;
        //         }
        //     }
        //     else
        //     {
        //         printf("BL Command not match\n");
        //     }
        // }
        else if (cmd == "abc")
        {
            if (v[1] == "nop")
            {
                if (v.size() < 3)
                {
                    msg_nop(0x55);
                    continue;
                }
                else
                {
                    char sub_func = std::stoi(v[2], 0, 16);
                    msg_nop(sub_func);
                }
            }
        }
        else if (cmd == "noptest")
        {
            for( int i = 0; i < 500; i++)
            {
                if(i%2 == 0)
                    msg_nop(0x55);
                else
                    msg_nop((char)i);

                usleep(10 *1000);
            }
        }
        else if (cmd == "img")
        {
            if (v.size() < 2)
            {
                cout << "try again" << endl;
                continue;
            }

            if (v[1] == "find")
            { // if absolute path from host, there is no need to find.
                // just check path correct, and copy to the string.
                str_imgfile_path.clear();
                int res = Update_Sour_Get_ImgFile_Status(&str_imgfile_path);
                if (res == 0)
                    printf("img path:%s\n", str_imgfile_path.c_str());
                else
                    printf("img path fault. res:%d\n", res);
            }
            else if (v[1] == "path") // update_file path from host.
            {
                if (Get_Update_Sour_ImgPath_Status(&g_inner_imgpath) == 0)
                    printf("inner_imgpath:%s\n", g_inner_imgpath.c_str());
                else
                    printf("inner_imgpath empty\n");

                if (Get_Update_Sour_IniPath_Status(&g_inner_inipath) == 0)
                    printf("inner_inipath:%s\n", g_inner_inipath.c_str());
                else
                    printf("inner_inipath empty\n");
            }
            else if (v[1] == "check")
            {
                bool b_img_exist = fileExists(g_inner_imgpath);
                bool b_ini_exist = fileExists(g_inner_inipath);
                printf("inner path exists status, img:%d, ini:%d.\n", b_img_exist, b_ini_exist);

                if (b_img_exist == true)
                {
                    str_imgfile_path.clear();
                    str_imgfile_path.append(g_inner_imgpath);

                    // copy to temp folder
                    str_dest_path.clear();
                    if (!str_imgfile_path.empty())
                    {
                        int res = Copy_ImgFile_To_Folder(str_imgfile_path, &str_dest_path);
                        if (res == 0)
                            printf("dest:%s\n", str_dest_path.c_str());
                        else
                            printf("copy img to temp_img fault, res:%d\n", res);
                    }

                    // decrypt to temp folder
                    if (!str_dest_path.empty())
                    {
                        std::vector<char> char_name(str_dest_path.begin(), str_dest_path.end());
                        char_name.push_back('\0');
                        char *img_filename = &char_name[0];

                        if (rc5_decryptfile(img_filename, &decrypt_path) == 0)
                        {
                            printf("deco-file path:%s\n", decrypt_path);
                            // free(decrypt_path);
                        }
                        else
                            printf("decryptfile fault\n");
                    }
                    else
                        printf("img path empty\n");

                    // load decrypted hex file
                    if ((decrypt_path == NULL) || (decrypt_path[0] == '\0'))
                        printf("deco-file path is empty\n");
                    else
                    {
                        if (bl_LoadHexSour(decrypt_path, &hexfile_hexsour) == 0)
                            printf("load hex file from deco-file okay. \n");
                    }
                }

                if (b_ini_exist == true)
                {
                    string info_pkg;
                    Update_Sour_find_info(g_inner_inipath, &info_pkg);
                    cout << "info_pkg:" << info_pkg << endl;
                }
            } /*
             else if( v[1] == "ini" )
             {
                 vector<string>vec_imgpath;
                 vec_imgpath.clear();
                 string info_path;
                 string info_pkg;

                 if(!str_imgfile_path.empty())
                 {
                     vec_imgpath = string_split(str_imgfile_path,".");

                     if(vec_imgpath.size()==2)
                     {
                         info_path = vec_imgpath[0]+".ini";
                         Update_Sour_find_info(info_path, &info_pkg);
                         cout << "info_pkg:" << info_pkg << endl;
                     }
                     else
                         cout<<"find ini file fault, size != 1"<<endl;
                 }
                 else
                     cout<<"find ini file fault"<<endl;
             }
             else if( v[1] == "copy" )
             {
                 str_dest_path.clear();

                 if(!str_imgfile_path.empty())
                 {
                     int res = Copy_ImgFile_To_Folder(str_imgfile_path,&str_dest_path);
                     if( res == 0 )
                         printf("dest:%s\n",str_dest_path.c_str());
                     else
                         printf("copy img to temp_img fault, res:%d\n",res);
                 }
                 else
                     printf("img path empty\n");
             }
             else if( v[1] == "deco" )
             {
                 if(!str_dest_path.empty())
                 {
                     std::vector<char> char_name(str_dest_path.begin(), str_dest_path.end());
                     char_name.push_back('\0');
                     char *img_filename = &char_name[0];

                     if(rc5_decryptfile(img_filename,&decrypt_path) == 0 )
                     {
                         printf("deco-file path:%s\n",decrypt_path);
                         //free(decrypt_path);
                     }
                     else
                         printf("decryptfile fault\n");
                 }
                 else
                     printf("img path empty\n");
             }
             else if( v[1] == "loadhex" )
             {
                 if ((decrypt_path == NULL) || (decrypt_path[0] == '\0'))
                     printf("deco-file path is empty\n");
                 else
                 {
                     if (bl_LoadHexSour(decrypt_path, &hexfile_hexsour) == 0)
                         printf("load hex file from deco-file okay. \n");
                 }
             }*/
            else
            {
                cout << "wrong command, try again" << endl;
                continue;
            }
            /*else if( v[1] == "deco" )
            {
                std::string str_target(v[2]);
                std::vector<char> char_name(str_target.begin(), str_target.end());
                char_name.push_back('\0');
                char *img_filename = &char_name[0];

                int res = rc5_decryptfile(img_filename);
                printf("rc5 decrypt file, res:%d\n",res);
            }*/
        }
        else if (cmd == "hex")
        {
            if (v.size() < 2)
            {
                cout << "try again" << endl;
                continue;
            }

            if (v[1] == "load")
            {
                if (v.size() < 3)
                {
                    cout << "hex load hexfile_name1, try again" << endl;
                    continue;
                }
                std::string str_target1(v[2]);
                std::vector<char> char_name1(str_target1.begin(), str_target1.end());
                char_name1.push_back('\0');
                char *hex_filename1 = &char_name1[0];
                if (bl_LoadHexSour(hex_filename1, &hexfile_hexsour) == 0)
                {
                    // bl_WriteHexRowThread_Init(&hexfile_hexsour);
                    printf("load hex file done. \n");
                }
            }
            else if (v[1] == "chksum")
            {
                unsigned short gen_chksum, aux_chksum;
                bl_Read_HexFile_Range_Checksum(&hexfile_hexsour, 0x0000, 0x55800 * 2, &gen_chksum);
                bl_Read_HexFile_Range_Checksum(&hexfile_hexsour, 0x7FC000 * 2, 0x800000 * 2, &aux_chksum);
                printf("file name:%s, GenSeg|AuxSeg checksum: [0x%04x, 0x%04x]\n", hexfile_hexsour.hex_filename, gen_chksum, aux_chksum);
            }
            else if (v[1] == "send")
            {
                bl_WrHexRow_Wakeup(&hexfile_hexsour, false);
            }
            else
            {
                printf("BL Command not match\n");
            }
        }
        else if (cmd == "hh")
        {
            bl_WrHexRow_Abort();
        }
        else if (cmd == "bltest")
        {
            int test_cycle;
            if (v.size() > 1)
            {
                test_cycle = std::stoi(v[1]);
                bl_EnduranceTest_Wakeup(test_cycle);
            }
            else
            {
                printf("BL Command not match\n");
            }
        }
        else if (cmd == "usbtest")
        {
            // std::string read_part;
            //            read_part = v[3];
            int msg_num = std::stoi(v[1]);
            testing_TransData(msg_num);
        }
        else if (cmd == "checkrtc")
        {
            string str_shellres;
            std::string shell_cmd = "pgrep rtc_driver";
            int shell_res, iPID;
            shell_res = shellCmd(shell_cmd, str_shellres);

            printf("shcmd_res:%d\n", shell_res);
            if (shell_res == 0)
            {
                iPID = stoi(str_shellres);
                printf("iPID:%d\n", iPID);
            }
        }
        else if (cmd == "killrtc")
        {
            string str_shellres;
            std::string shell_cmd = "sudo killall rtc_driver";
            int shell_res;
            shell_res = shellCmd(shell_cmd, str_shellres);

            printf("shcmd_res:%d\n", shell_res);
            cout << str_shellres << endl;
        }
        else if (cmd == "tcp")
        {
            int res;
            std::string strCmd = "{\"bldriv_fbk\": \"test\",\"int\": 654321}";
            res = tx_msg_to_Bootloader_UI(strCmd);
        }
        else if (cmd == "shellcmd")
        {
            if (v.size() < 2)
            {
                printf("shellcmd not correct, try again\n");
                continue;
            }

            if (v.size() == 2)
            {
                string str_cmd = v[1];
                string str_res;

                vector<string> str_sp;
                if (shellcmd(str_cmd, str_res) == 0)
                    cout << str_res << endl;
                else
                    printf("shellcmd return -1\n");
            }
        }
        // else if (cmd == "findpath")
        //{
        //     vector<string> str_path = find_usb_path2();
        //     printf("str_path size:%ld\n", str_path.size());
        //     for (int i = 0; i < str_path.size(); i++)
        //         cout << str_path[i] << endl;
        // }
        else if (cmd == "")
        {
        }
        else
        {
            cout << "unsupported command:" << command_line << endl;
        }
        printf("\n>>");
        fflush(stdout); // 因kbhit的影響, 此處未清出stdout的話, 畫面顯示會有問題.
    }
    cout << "quit cli ..." << endl;
}
