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
