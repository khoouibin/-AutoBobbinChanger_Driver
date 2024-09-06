#ifndef MQTT_PROT_H_
#define MQTT_PROT_H_
/*
#include "Poco/Task.h"
#include "Poco/TaskManager.h"
#include "Poco/TaskNotification.h"
#include "Poco/NotificationCenter.h"
#include "Poco/NotificationQueue.h"
#include "Poco/Observer.h"
#include "Poco/Exception.h"

#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <cctype>
#include <thread>
#include <atomic>
#include <chrono>
#include "mqtt/async_client.h"
#include "json.hpp"

using namespace std;
using namespace Poco;
using json = nlohmann::json;


class DrvDownlinkService : public Poco::Task
{
private:
    const std::string SERVER_ADDRESS { "tcp://localhost:1883" };
    const std::string CLIENT_ID		 { "async_consume" };
    const std::string TOPIC 		 { "/uhf/drv/cmd/" };
    const int QOS = 1;

	bool m_b_running;
	NotificationQueue &_queue;

public:
	DrvDownlinkService(const std::string &name, NotificationQueue &queue)
		: Task(name),
		  _queue(queue)
	{ m_b_running = true; }
	
    void runTask();

	void stop() { m_b_running = false; }
};


class DrvUplinkService : public Poco::Task
{
private:
    const std::string SERVER_ADDRESS { "tcp://localhost:1883" };
    const std::string CLIENT_ID		 { "rtc_ack" };
    const std::string TOPIC 		 { "/uhf/drv/" };
    const int QOS = 1;

	bool m_b_running;
	NotificationQueue &_queue;

public:
	DrvUplinkService(const std::string &name, NotificationQueue &queue)
		: Task(name),
		  _queue(queue)
	{ m_b_running = true; }
    void runTask();
	void stop() { m_b_running = false; }
};

void publish_uplink_message2(std::string status, int error_code, std::string message);
*/
#endif
