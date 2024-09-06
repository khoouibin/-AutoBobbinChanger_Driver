#include "interface_to_host.h"
#include "notification_pkg.h"
#include "HandleCmd.h"
#include "bl_return_code.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

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
#include "logger_wrapper.h"
#include <stdarg.h>
#include <cstdlib>

TcpConnection::TcpConnection(const StreamSocket &s, NotificationQueue &queue)
    : TCPServerConnection(s),
      _queue(queue)
{
}

void TcpConnection::run()
{
    StreamSocket &ss = socket();
    try
    {
        char recvbuffer[327680]; // 80 * 4096.
        const std::string ack = "ack";

        int n = ss.receiveBytes(recvbuffer, sizeof(recvbuffer));

        recvbuffer[n] = '\0';
        std::string str_recv_msg = std::string(recvbuffer);
        // _queue.enqueueNotification(new NotificationFromHost(str_recv_msg));

        while (n > 0)
        {
            // 接收由HOST傳來的訊息
            std::string str_ack = parseCmdPackage(str_recv_msg);

            const char *sendbuffer = str_ack.c_str();
            int sendbuffer_length = str_ack.length();

            ss.sendBytes(sendbuffer, sendbuffer_length);
            str_ack.clear();

            n = ss.receiveBytes(recvbuffer, sizeof(recvbuffer));
        }
    }
    catch (Poco::Exception &exc)
    {
        std::cerr << "Connection exception: " << exc.displayText() << std::endl;
    }
}

TcpDrvServer::TcpDrvServer(NotificationQueue &queue)
    : _queue(queue)
{
}

/////////////////////// send message to Host /////////////////////////////////////

#include <stdio.h>
#include <sys/socket.h>
#include <mutex>

static std::mutex gmtxSendMessage;
UplinkMessageHandler goUplinkMessageHandler("UplinkMessageHandler");

// 傳送訊息到HOST
int send_message_to_host(std::string strStatus, int iErrorCode, const char *szFormat, ...)
{
    char szMessage[_MAX_LOG_BYTE];
    va_list arg_ptr;

    va_start(arg_ptr, szFormat);
    vsprintf(szMessage, szFormat, arg_ptr);
    va_end(arg_ptr);

    return send_message_to_host(strStatus, iErrorCode, string(szMessage));
}

int send_message_to_host(std::string status, int error_code, std::string message)
{
    /*while (!gmtxSendMessage.try_lock())
    {
        usleep(500);
    }*/
    gmtxSendMessage.lock();

#if 1
    goUplinkMessageHandler.EnqueueMessage(status, error_code, message);
    gmtxSendMessage.unlock();
    return 0;
#else
    goUplinkMessageHandler.SendMessageToHost(status, error_code, message);
    gmtxSendMessage.unlock();
#endif

    //     json notification;
    // 	notification["cmd"] = "notification";
    //     notification["status"] = status;
    //     notification["error_code"] = error_code;
    //     notification["message"] = message;
    //     int iReturn;
    //     int iSockfd = 0;

    // timestamp_t time_delay;
    // time_delay.time_pre = time_stamp();

    //     iSockfd = socket(AF_INET , SOCK_STREAM , 0);

    //     if (iSockfd == -1){
    //         printf("Fail to create a socket.");
    //         return ERR_FAIL_TO_SOCKET;
    //     }

    //     struct sockaddr_in tSockAddr;
    //     struct in_addr;

    //     bzero(&tSockAddr,sizeof(tSockAddr));
    //     tSockAddr.sin_family = PF_INET;
    //     tSockAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    //     tSockAddr.sin_port = htons(8002);

    //     iReturn = connect( iSockfd, (struct sockaddr *)&tSockAddr, sizeof(tSockAddr));
    //     if (iReturn ){
    // time_delay.time_now = time_stamp();
    // time_delay.time_diff= time_delay.time_now - time_delay.time_pre;
    // printf("send_message_to_host:tcp task connect fail. time=%d\n", time_delay.time_diff );
    //         shutdown(iSockfd, SHUT_RDWR);
    //         return ERR_FAIL_TO_CONNECT;
    //     }
    //     string str_msg = notification.dump();

    //     iReturn = send( iSockfd, str_msg.c_str(), str_msg.length(), 0);
    //     if ( iReturn ){
    // time_delay.time_now = time_stamp();
    // time_delay.time_diff= time_delay.time_now - time_delay.time_pre;
    // printf("tcp task success to send %d byte. time=%d\n", iReturn, time_delay.time_diff );
    //         //close(iSockfd);
    //         shutdown(iSockfd, SHUT_RDWR);
    //         close(iSockfd);
    //         return 0;
    //     }
    //     else{
    // time_delay.time_now = time_stamp();
    // time_delay.time_diff= time_delay.time_now - time_delay.time_pre;
    // printf("tcp task fail to send. time=%d\n", time_delay.time_diff );
    //         //close(iSockfd);
    //         shutdown(iSockfd, SHUT_RDWR);
    //         close(iSockfd);
    //         return ERR_FAIL_TO_SEND;
    //     }
}

UplinkMessageHandler::UplinkMessageHandler(const std::string &name)
{
    m_b_is_alive = true;
}

void UplinkMessageHandler::stop()
{
    m_b_is_alive = false;
    m_event.set();
}

void UplinkMessageHandler::run()
{
    int count = 0;
    std::cout << "UplinkMessageHandler start" << endl;
    while (m_b_is_alive)
    {
        if (m_message_queue.size() == 0)
        {
            m_event.wait();
        }
        else
        {
            MESSAGE_PACKAGE_t message_package = m_message_queue.front();
            m_message_queue.pop();
            SendMessageToHost(message_package.status, message_package.error_code, message_package.message);
            if (message_package.status == "exit" && message_package.message == "exit")
            {
                break;
            }
        }
    }
    std::cout << "UplinkMessageHandler terminate" << endl;
}

int UplinkMessageHandler::SendMessageToHost(std::string status, int error_code, std::string message)
{
    json notification;
    notification["cmd"] = "notification";
    notification["status"] = status;
    notification["error_code"] = error_code;
    notification["message"] = message;
    int iReturn;
    int iSockfd = 0;

    timestamp_t time_delay;
    time_delay.time_pre = time_stamp();

    iSockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (iSockfd == -1)
    {
        goDriverLogger.Log("ERROR", "Fail to create a socket for sending message to host.");
        return ERR_FAIL_TO_SOCKET;
    }

    struct sockaddr_in tSockAddr;
    struct in_addr;

    bzero(&tSockAddr, sizeof(tSockAddr));
    tSockAddr.sin_family = PF_INET;
    tSockAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    tSockAddr.sin_port = htons(8002);

    iReturn = connect(iSockfd, (struct sockaddr *)&tSockAddr, sizeof(tSockAddr));
    if (iReturn)
    {
        time_delay.time_now = time_stamp();
        time_delay.time_diff = time_delay.time_now - time_delay.time_pre;
        // printf("send_message_to_host:tcp task connect fail. time=%d. Socket=%d\n", time_delay.time_diff, iSockfd );
        shutdown(iSockfd, SHUT_RDWR);
        close(iSockfd);
        // printf("closed.\n");
        return ERR_FAIL_TO_CONNECT;
    }
    string str_msg = notification.dump();

    iReturn = send(iSockfd, str_msg.c_str(), str_msg.length(), 0);
    if (iReturn)
    {
        // time_delay.time_now = time_stamp();
        // time_delay.time_diff= time_delay.time_now - time_delay.time_pre;
        // printf("tcp task success to send %d byte. time=%d\n", iReturn, time_delay.time_diff );
        // close(iSockfd);
        shutdown(iSockfd, SHUT_RDWR);
        close(iSockfd);
        return 0;
    }
    else
    {
        time_delay.time_now = time_stamp();
        time_delay.time_diff = time_delay.time_now - time_delay.time_pre;
        goDriverLogger.Log("ERROR", "TCP task fail to send message to host. time=%d.", time_delay.time_diff);
        // close(iSockfd);
        shutdown(iSockfd, SHUT_RDWR);
        close(iSockfd);
        return ERR_FAIL_TO_SEND;
    }
}

void UplinkMessageHandler::EnqueueMessage(std::string status, int error_code, std::string message)
{
    MESSAGE_PACKAGE_t message_package;
    message_package.message = message;
    message_package.error_code = error_code;
    message_package.status = status;
    m_message_queue.push(message_package);
    m_event.set();
}

Poco::Thread goUplinkMessageHandlerThread;

void InitUplinkMessageHandler()
{
    goUplinkMessageHandlerThread.start(goUplinkMessageHandler);
}

void StopUplinkMessageHandler()
{
    goUplinkMessageHandler.stop();
    goUplinkMessageHandlerThread.join();
}