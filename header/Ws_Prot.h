/*#include <iostream>
#include <string>

#include "Poco/Logger.h"
#include "Poco/Task.h"
#include "Poco/TaskManager.h"
#include "Poco/TaskNotification.h"
#include "Poco/NotificationCenter.h"
#include "Poco/Observer.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/URI.h"
#include "Poco/JSON/Object.h"
#include "Poco/Exception.h"
#include "Poco/Net/WebSocket.h"
#include "Poco/JSON/Object.h"
#include "Poco/NotificationQueue.h"
#include "Poco/Mutex.h"


using namespace std;
using namespace Poco;
using namespace Poco::JSON;
using namespace Poco::Net;


class WsSendService : public Poco::Task
{

private:
    const char *m_c_ws_url = "ws://127.0.0.1:8000/ws/drv/recv/";
    bool m_b_running;
    NotificationQueue& _queue;

public:
    WsSendService(const std::string &name, NotificationQueue& queue);
    void runTask();
    void stop();
};

class WsRecvService : public Poco::Task
{

private:
    const int m_c_timeout_threshold = 1000000;
    const char *m_c_ws_url = "ws://127.0.0.1:8000/ws/drv/send/";
    bool m_b_running;
    NotificationQueue& _queue;

public:
    WsRecvService(const std::string &name, NotificationQueue& queue);
    void runTask();
    void stop();
};
*/
