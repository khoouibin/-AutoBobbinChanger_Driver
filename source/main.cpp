#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdint.h>
//#include <iostream>
#include <string>
#include <vector>

#include <fcntl.h> // serial needs.
#include <termios.h>
#include <errno.h>
#include "Poco/Task.h"
#include "Poco/TaskManager.h"

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

#include "USBComm.h"
#include "USBComm_Driver.h"
#include "interface_to_host.h"
#include "Mqtt_prot.h"
#include "ReceivMsg.h"
#include "cli.h"
#include "HandleCmd.h"

#include "logger_wrapper.h"
#include "notification_pkg.h"
#include "json.hpp"
//#include "return_code.h"
#include "BL_USBComm.h"

#include "bl_ctrl.h"
#include "update_sour.h"

using namespace std;
using namespace Poco;
using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;
using json = nlohmann::json;
using boost::shared_ptr;

int debug_communication()
{
	while (true)
	{
		sleep(5);
	}
	return 0;
}

int main(int argc, char *argv[])
{
	bool b_en_cli = false;
	// bool b_check_usb_target = false;
	int iRtcId = 0;
	char szIp[32];
	char szPort[16];
	char *str_prodct = NULL;
	char *str_manufature = NULL;
	int res_libusb_init, res_usbpthread_init, res_open_dev;
	int i_device_num, res_target_device;
	int iState;

	// 啟動 LoggerWrapper
	InitLoggerWrapper();
	// InitUplinkMessageHandler();

	if (argc >= 2)
	{
		for (int i = 1; i < argc; i++)
		{
			if (std::string(argv[i]) == "-cli")
			{
				b_en_cli = true;
				continue;
			}

			if (std::string(argv[i]) == "-img")
			{
				Set_Update_Sour_ImgPath(std::string(argv[i+1]));
				i++;
			}

			if (std::string(argv[i]) == "-ini")
			{
				Set_Update_Sour_IniPath(std::string(argv[i+1]));
				i++;
			}
		}
	}

	std::string inner_imgpath;
	std::string inner_inipath;
	int res = Get_Update_Sour_ImgPath_Status(&inner_imgpath);
	if (res == 0)
		printf("inner img_path:%s\n", inner_imgpath.c_str());

	res = Get_Update_Sour_IniPath_Status(&inner_inipath);
	if (res == 0)
		printf("inner ini_path:%s\n", inner_inipath.c_str());



	NotificationQueue recv_queue;

	Poco::Net::ServerSocket serverSocket(8003);
	Poco::Net::TCPServerParams::Ptr param = new Poco::Net::TCPServerParams;
	param->setMaxQueued(100);
	param->setMaxThreads(100);
	Poco::Net::TCPServer ServerForHost(new TcpDrvServer(recv_queue), serverSocket);
	ServerForHost.start();

	res_libusb_init = BL_USBComm_Driver_Init_LIBUSB();
	res_usbpthread_init = BL_USBComm_Driver_Init_Threads();
	if (res_libusb_init != 0 || res_usbpthread_init != 0)
	{
		printf("LIBUSB_init() or USB_Pthread_init() Fault\n");
		exit(1);
	}

	bl_WrHexRow_Thread_Init();
	bl_EnduranceTest_Thread_Init();
	Update_Sour_Thread_Init();

	if (b_en_cli)
	{
		CommandLineInterface();
		std::cout << "end cli" << endl;
	}
	else
	{
		while (1)
		{
			sleep(1);
			printf("bl_driver while...\n");
			if (Get_BL_Driver_Run_Terminate_Status() == false)
			{
				printf("BL_Driver_Run_Terminate\n");
				break;
			}
		}
	}

	iState = bl_WrHexRow_Thread_Terminate();
	// printf("bl_WrHexRow_Thread_Terminate res=%d\n", iState);
	iState = bl_EnduranceTest_Thread_Terminate();
	// printf("bl_EnduranceTest_Thread_Terminate res=%d\n", iState);
	iState = Update_Sour_Thread_Terminate();
	// printf("Update_Sour_Thread_Terminate res=%d\n", iState);
	iState = BL_USBComm_Rx_Terminate();
	// printf("BL_USBComm_Rx_Terminate res=%d\n", iState);
	iState = BL_USBComm_Tx_Terminate();
	// printf("BL_USBComm_Tx_Terminate res=%d\n", iState);
	iState = BL_USBComm_Task_Terminate();
	// printf("BL_USBComm_Task_Terminate res=%d\n", iState);
	iState = BL_USBComm_AutoReConnect_Terminate();
	// printf("BL_USBComm_AutoReConnect_Terminate res=%d\n", iState);

	ServerForHost.stop();
	StopLoggerWrapper();
	// StopUplinkMessageHandler();
	printf("BL_Driver finish program\n");
	exit(1);
}
