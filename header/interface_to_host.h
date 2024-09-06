#ifndef TCP_PROT_H_
#define TCP_PROT_H_

#include <iostream>
#include <string>
#include <queue>

#include "Poco/Task.h"
#include "Poco/TaskManager.h"
#include "Poco/TaskNotification.h"
#include "Poco/NotificationCenter.h"
#include "Poco/Thread.h"
#include "Poco/Event.h"
#include "Poco/Runnable.h"
#include "Poco/Observer.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/URI.h"
#include "Poco/JSON/Object.h"
#include "Poco/Exception.h"
#include "Poco/Net/WebSocket.h"
#include "Poco/JSON/Object.h"
#include "Poco/TaskNotification.h"
#include "Poco/NotificationQueue.h"
#include "Poco/Mutex.h"
#include "Poco/Net/TCPServer.h"

using namespace std;
using namespace Poco;
using namespace Poco::JSON;
using namespace Poco::Net;

class TcpConnection : public TCPServerConnection
{
private:
    NotificationQueue &_queue;

public:
    TcpConnection(const StreamSocket &s, NotificationQueue &_queue);
    void run();
};

class TcpDrvServer : public TCPServerConnectionFactory
{
private:
    NotificationQueue &_queue;

public:
    TcpDrvServer(NotificationQueue &queue);
    inline TCPServerConnection *createConnection(const StreamSocket &socket)
    {
        return new TcpConnection(socket, _queue);
    }
};

int send_message_to_host(std::string status, int error_code, std::string message);

typedef struct
{
    std::string status;
    int error_code;
    std::string message;
} MESSAGE_PACKAGE_t;

class UplinkMessageHandler : public Poco::Runnable
{
private:
    std::queue<MESSAGE_PACKAGE_t> m_message_queue;
    Poco::Event m_event;
    bool m_b_is_alive;

public:
    UplinkMessageHandler(const std::string &name);
    virtual void run();
    void stop();
    int SendMessageToHost(std::string status, int error_code, std::string message);
    void EnqueueMessage(std::string status, int error_code, std::string message);
};

#endif

void InitUplinkMessageHandler();
void StopUplinkMessageHandler();
