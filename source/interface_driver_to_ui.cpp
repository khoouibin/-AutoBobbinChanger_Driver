
#include "interface_driver_to_ui.h"
#include "bl_return_code.h"

#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "global.h"

#include "Poco/Task.h"
#include "Poco/TaskManager.h"
#include "Poco/TaskNotification.h"
#include "Poco/NotificationCenter.h"
#include "Poco/NotificationQueue.h"
#include "Poco/Exception.h"
#include "Poco/Mutex.h"
#include "Poco/Condition.h"
#include "Poco/Event.h"
#include "Poco/Timestamp.h"

/////////////////////// send message UI-> RTC driver//////////////////////////////
#include <stdio.h>
#include <sys/socket.h>

// 傳送訊息到ui
int send_message_to_uiBL(std::string message)
{
    json bldriv_msg;
    json json_msg = json::parse(message);
    std::string response_msg = json_msg["bldriv_fbk"];

    if (response_msg == "test")
    {
        bldriv_msg["bldriv_status"] = "Ready";
        bldriv_msg["cmd"] = "ToGo";
        bldriv_msg["num"] = json_msg["int"];
    }

    //----------------------------------------------------------------
    int iReturn;
    int iSockfd = 0;

    timestamp_t time_delay;
    time_delay.time_pre = time_stamp();

    iSockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (iSockfd == -1)
    {
        printf("Fail to create a socket.");
        return ERR_FAIL_TO_SOCKET;
    }

    struct sockaddr_in tSockAddr;
    struct in_addr;

    bzero(&tSockAddr, sizeof(tSockAddr));
    tSockAddr.sin_family = PF_INET;
    tSockAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    tSockAddr.sin_port = htons(8004);

    iReturn = connect(iSockfd, (struct sockaddr *)&tSockAddr, sizeof(tSockAddr));
    if (iReturn)
    {
        time_delay.time_now = time_stamp();
        time_delay.time_diff = time_delay.time_now - time_delay.time_pre;
        printf("send_message_to_host:tcp task connect fail. time=%d\n", time_delay.time_diff);
        shutdown(iSockfd, SHUT_RDWR);
        return ERR_FAIL_TO_CONNECT;
    }
    string str_msg = bldriv_msg.dump();

    iReturn = send(iSockfd, str_msg.c_str(), str_msg.length(), 0);
    if (iReturn)
    {
        time_delay.time_now = time_stamp();
        time_delay.time_diff = time_delay.time_now - time_delay.time_pre;
        printf("tcp task success to send %d byte. time=%d\n", iReturn, time_delay.time_diff);
        // close(iSockfd);
        shutdown(iSockfd, SHUT_RDWR);
        close(iSockfd);
        return 0;
    }
    else
    {
        time_delay.time_now = time_stamp();
        time_delay.time_diff = time_delay.time_now - time_delay.time_pre;
        printf("tcp task fail to send. time=%d\n", time_delay.time_diff);
        // close(iSockfd);
        shutdown(iSockfd, SHUT_RDWR);
        close(iSockfd);
        return ERR_FAIL_TO_SEND;
    }
}

int tx_msg_to_Bootloader_UI(std::string message)
{
    cout << "msg:" << message << endl;
    json bldriv_msg;
    json json_msg = json::parse(message);
    std::string response_msg = json_msg["bldriv_fbk"];

    if (response_msg == "test")
    {
        bldriv_msg["BLdriv_info"] = "Ready";
        bldriv_msg["cmd"] = "ToGo";
        bldriv_msg["num"] = json_msg["int"];
    }
    else if (response_msg == "status_fbk")
    {
        std::string target = json_msg["target"];
        if (target == "bl_driver")
        {
            bldriv_msg["BLdriv_info"] = response_msg;
            bldriv_msg["target"] = target;
            bldriv_msg["value"] = json_msg["value"];
        }
        else if (target == "rtc_device")
        {
            bldriv_msg["BLdriv_info"] = response_msg;
            bldriv_msg["target"] = target;
            bldriv_msg["value"] = json_msg["value"];
        }
        else if (target == "update_img")
        {
            bldriv_msg["BLdriv_info"] = response_msg;
            bldriv_msg["target"] = target;
            bldriv_msg["value"] = json_msg["value"];
            bldriv_msg["imgfile_path"] = json_msg["imgfile_path"];
        }
        else if (target == "update_info")
        {
            bldriv_msg["BLdriv_info"] = response_msg;
            bldriv_msg["target"] = target;
            bldriv_msg["value"] = json_msg["value"];
            bldriv_msg["info_msg"] = json_msg["info_msg"];
            bldriv_msg["inifile_path"] = json_msg["inifile_path"];
        }
    }
    else if (response_msg == "rtc_dev_switch_fbk")
    {
        std::string action = json_msg["action"];
        bldriv_msg["BLdriv_info"] = response_msg;
        bldriv_msg["action"] = action;
        bldriv_msg["value"] = json_msg["value"];
    }
    else if (response_msg == "bl_drv_task")
    {
        std::string action = json_msg["action"];
        bldriv_msg["BLdriv_info"] = response_msg;
        bldriv_msg["action"] = action;
        bldriv_msg["value"] = json_msg["value"];

        if (action == "GetONSVer_byAux")
        {
            bldriv_msg["action"] = action;
            bldriv_msg["ons_vernum"] = json_msg["ons_vernum"];
            bldriv_msg["ons_vermsg"] = json_msg["ons_vermsg"];
        }
    }
    else if (response_msg == "writehex_info")
    {
        std::string info = json_msg["info"];
        bldriv_msg["BLdriv_info"] = response_msg;
        bldriv_msg["info"] = info;
        bldriv_msg["value"] = json_msg["value"];
    }
    else if (response_msg == "bl_drv_terminate")
    {
        bldriv_msg["BLdriv_info"] = response_msg;
        bldriv_msg["value"] = json_msg["value"];
    }
    else if (response_msg == "bl_drv_update_path")
    {
        bldriv_msg["BLdriv_info"] = response_msg;
        bldriv_msg["value"] = json_msg["value"];
    }
    //----------------------------------------------------------------
    int iReturn;
    int iSockfd = 0;

    timestamp_t time_delay;
    time_delay.time_pre = time_stamp();

    iSockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (iSockfd == -1)
    {
        printf("Fail to create a socket.");
        return ERR_FAIL_TO_SOCKET;
    }

    struct sockaddr_in tSockAddr;
    struct in_addr;

    bzero(&tSockAddr, sizeof(tSockAddr));
    tSockAddr.sin_family = PF_INET;
    tSockAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    tSockAddr.sin_port = htons(8004);

    iReturn = connect(iSockfd, (struct sockaddr *)&tSockAddr, sizeof(tSockAddr));
    if (iReturn)
    {
        time_delay.time_now = time_stamp();
        time_delay.time_diff = time_delay.time_now - time_delay.time_pre;
        printf("send_message_to_host:tcp task connect fail. time=%d\n", time_delay.time_diff);
        shutdown(iSockfd, SHUT_RDWR);
        return ERR_FAIL_TO_CONNECT;
    }
    string str_msg = bldriv_msg.dump();

    iReturn = send(iSockfd, str_msg.c_str(), str_msg.length(), 0);
    if (iReturn)
    {
        time_delay.time_now = time_stamp();
        time_delay.time_diff = time_delay.time_now - time_delay.time_pre;
        printf("tcp task success to send %d byte. time=%d\n", iReturn, time_delay.time_diff);
        // close(iSockfd);
        shutdown(iSockfd, SHUT_RDWR);
        close(iSockfd);
        return 0;
    }
    else
    {
        time_delay.time_now = time_stamp();
        time_delay.time_diff = time_delay.time_now - time_delay.time_pre;
        printf("tcp task fail to send. time=%d\n", time_delay.time_diff);
        // close(iSockfd);
        shutdown(iSockfd, SHUT_RDWR);
        close(iSockfd);
        return ERR_FAIL_TO_SEND;
    }
}

int time_stamp(void)
{
    struct timeval tv;
    int _time_now = 0;
    gettimeofday(&tv, NULL);
    _time_now = (((int)tv.tv_sec) % 1000) * 1000;
    _time_now += ((int)tv.tv_usec / 1000);
    return _time_now;
}