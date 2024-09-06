/*#include "Ws_Prot.h"
#include "notification_pkg.h"
#include "Poco/Net/NetException.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/HTTPMessage.h"
#include "Poco/Net/WebSocket.h"
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/StreamCopier.h"
#include "Poco/Dynamic/Var.h"

#include <iostream>

using namespace Poco::Net;
using namespace Poco::Dynamic;
using namespace Poco;


static WsSendService *instance;


WsSendService::WsSendService(const std::string &name, NotificationQueue &queue)
    : Task(name),
      _queue(queue)
{
    m_b_running = true;
}

void WsSendService::stop()
{
    m_b_running = false;
}

void WsSendService::runTask()
{
    Poco::URI uri(m_c_ws_url);
    std::string path(uri.getPathAndQuery());
    if (path.empty())
        path = "/";

    HTTPClientSession session(uri.getHost(), uri.getPort());
    HTTPRequest request(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);
    HTTPResponse response;

    while (m_b_running)
    {
        Notification::Ptr pNf(_queue.waitDequeueNotification(1000));
        if (pNf)
        {
            NotificationToHost::Ptr pWorkNf = pNf.cast<NotificationToHost>();
            if (pWorkNf)
            {
                try
                {
                    // std::cout << "try ws:" << std::endl;
                    WebSocket *m_psock = new WebSocket(session, request, response);
                    std::string str_msg = pWorkNf->data();
                    const char *cstr = str_msg.c_str();

                    int len = m_psock->sendFrame(cstr, strlen(cstr), WebSocket::FRAME_TEXT);
                    std::cout << "Sent:" << cstr << std::endl;

                    m_psock->close();
                }
                catch (std::exception &e)
                {
                    std::cout << "Exception " << e.what() << endl;
                }
            }
        }
    }
    std::cout << "finish send" << endl;
}

WsRecvService::WsRecvService(const std::string &name, NotificationQueue &queue)
    : Task(name),
      _queue(queue)
{
    m_b_running = true;
}

void WsRecvService::stop()
{
    m_b_running = false;
}

void WsRecvService::runTask()
{
    Poco::URI uri(m_c_ws_url);
    std::string path(uri.getPathAndQuery());
    if (path.empty())
        path = "/";

    HTTPClientSession session(uri.getHost(), uri.getPort());
    HTTPRequest request(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);
    HTTPResponse response;

    bool is_ws_connected = false;
    WebSocket *m_psock;

    while (m_b_running)
    {
        if (!is_ws_connected)
        {
            try
            {
                m_psock = new WebSocket(session, request, response);
                m_psock->setReceiveTimeout(m_c_timeout_threshold);
                is_ws_connected = true;
            }
            catch (std::exception &e)
            {
                std::cout << "Exception " << e.what() << endl;
                is_ws_connected = false;
            }
        }
        else
        {
            try
            {
                char receiveBuff[256];
                int flags = 0;
                int rlen = m_psock->receiveFrame(receiveBuff, 256, flags);
                receiveBuff[rlen] = '\0';
                std::string str_receive(receiveBuff);
                if (rlen > 0) {
                    std::cout << "recv:" << str_receive << ":" << rlen << std::endl;
                    _queue.enqueueNotification(new NotificationFromHost(str_receive));
                }
            }
            catch(Poco::TimeoutException&)
            {
                std::cout << "Recv timeout:" << endl;
                const char *cstr = "ping";
                int len = m_psock->sendFrame(cstr, strlen(cstr), WebSocket::FRAME_TEXT);
            }
            catch (Poco::Net::WebSocketException &e)
            {
                std::cout << "Exception " << e.what() << endl;
                is_ws_connected = false;
                
            }
        }

        if (is_ws_connected == false)
            Task::sleep(5000);
    }

    std::cout << "finish recv" << endl;

    if (is_ws_connected)
        m_psock->close();
}
*/

