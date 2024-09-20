#include "cli.h"
#include <iostream>
#include <string>
#include <vector>
#include <termios.h>
#include <errno.h>
#include <entity_io.h>
#include <bitset>
#include <iomanip>
#include "interface_to_host.h"
#include "interface_driver_to_ui.h"
#include "Mqtt_prot.h"
#include "notification_pkg.h"
#include "httpinterface.h"
#include "json.hpp"
//#include "USBComm.h"
#include "USBComm_Driver.h"
#include "USBComm_Abc.h"
#include "update_sour.h"

using namespace std;

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
                        dev_num = USBComm_Driver_GetDeviceList(&target_device, 1);

                        if (target_device == -1)
                            printf("device_number:%d,target device: DeviceNull \n", dev_num);
                        else
                        {
                            // res_open_dev = USBComm_Driver_openTargetDevice((UsbDevice) res_target_device);
                            printf("device_number:%d,target device: %d\n", dev_num, target_device );
                        }
                    }
                    else if (v[1] == "show_target")
                    {
                        int usb_target_dev = USBComm_Driver_getTargetDevice();
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
                            res = USBComm_Driver_openTargetDevice(Device_ABC_RTC);
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
                        dev_num = USBComm_Driver_GetDeviceList(&target_device, 1);

                        if (target_device == -1)
                            printf("device_number:%d,target device: DeviceNull \n", dev_num);
                        else
                        {
                            res_open_dev = USBComm_Driver_openTargetDevice((UsbDevice)target_device);
                            printf("device_number:%d,target device: %d, open_device:%d\n", dev_num, target_device , res_open_dev);
                        }
                    }
                    else
                        printf("usb cmd  wrong, try again.\n");
                }
            }
        }
        else if (cmd == "echo")
        {
            if (v.size() < 2)
            {
                usb_message_echo(0x55);
                continue;
            }
            else
            {
                try
                {
                    char sub_func = std::stoi(v[1], 0, 16);
                    usb_message_echo(sub_func);
                }
                catch (const std::exception &e)
                {
                    std::cout << "echo message, exception:" << e.what() << '\n';
                }
            }
        }
        else if (cmd == "reset")
        {
            if (v.size() < 2)
            {
                usb_message_reset(0x01);
                continue;
            }
            else
            {
                try
                {
                    int delay_time = std::stoi(v[1]);
                    usb_message_reset(01,(unsigned short)delay_time);
                }
                catch (const std::exception &e)
                {
                    std::cout << "echo message, exception:" << e.what() << '\n';
                }
            }
        }
        else if (cmd == "get-profile01")
        {
            RTC_Profile_01_t res_profile01;
            int res = usb_get_profile_01(&res_profile01);
            printf("res_profile01-01:%d\n",res_profile01.p01_01);
            printf("res_profile01-02:%d\n",res_profile01.p01_02);
            printf("res_profile01-03:%d\n",res_profile01.p01_03);
            printf("res_profile01-04:%d\n",res_profile01.p01_04);
            printf("res_profile01-05:%d\n",res_profile01.p01_05);
            printf("res_profile01-06:%d\n",res_profile01.p01_06);
        }
        else if (cmd == "set-profile01")
        {
            RTC_Profile_01_t set_profile01={11,-12,12345,-32700,1147000012,-2112345678};
            int res = usb_set_profile_01(&set_profile01);
        }
        else if (cmd == "get-profile02")
        {
            RTC_Profile_02_t res_profile02;
            int res = usb_get_profile_02(&res_profile02);
            printf("res_profile02-01:%d\n", res_profile02.p02_01);
            printf("res_profile02-02:%d\n", res_profile02.p02_02);
            printf("res_profile02-03:%d\n", res_profile02.p02_03);
            printf("res_profile02-04:%d\n", res_profile02.p02_04);
            printf("res_profile02-05:%d\n", res_profile02.p02_05);
            printf("res_profile02-06:%d\n", res_profile02.p02_06);
            printf("res_profile02-07:%d\n", res_profile02.p02_07);
            printf("res_profile02-08:%d\n", res_profile02.p02_08);
            printf("res_profile02-09:%d\n", res_profile02.p02_09);
            printf("res_profile02-10:%d\n", res_profile02.p02_10);
            printf("res_profile02-11:%d\n", res_profile02.p02_11);
        }
        else if (cmd == "set-profile02")
        {
            RTC_Profile_02_t set_profile02={05,-5,6666,-6666,111222333,-111222333,444222333,-111,-2,-3,-4};
            int res = usb_set_profile_02(&set_profile02);
        }
        else if (cmd == "get-loglevel")
        {
            usb_message_log_level_get();
            continue;
        }
        else if (cmd == "set-loglevel")
        {
            usb_message_log_level_set(Critial_Lev);
            continue;
        }
        else if (cmd == "entab-mode")
        {
            if (v.size() >= 2)
            {
                try
                {
                    int sub_func = std::stoi(v[1]);
                    usb_message_set_entity_table_reply_mode(sub_func);
                }
                catch (const std::exception &e)
                {
                    std::cout << "echo message, exception:" << e.what() << '\n';
                }
            }
            else
            {
                 printf("usb cmd  wrong, try again.\n");
            }
        }
        else if (cmd == "tcp")
        {
            int res;
            std::string strCmd = "{\"bldriv_fbk\": \"test\",\"int\": 654321}";
            res = tx_msg_to_Bootloader_UI(strCmd);
        }
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
