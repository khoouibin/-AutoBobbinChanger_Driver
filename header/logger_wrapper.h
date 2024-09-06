#include <iostream>
#include <string>
#include <queue>
#include <sys/stat.h>
#include "Logger.h"
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include "Poco/Task.h"
#include "Poco/TaskManager.h"
#include "Poco/TaskNotification.h"
#include "Poco/Thread.h"
#include "Poco/Event.h"
#include "Poco/Runnable.h"

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using boost::shared_ptr;
using std::string;
using namespace std;
using namespace Poco;
// using namespace Poco::JSON;
// using namespace Poco::Net;

#define _MAX_LOG_BYTE 512

typedef struct
{
    std::string strLevel;
    std::string strMessage;
    std::string strTimestamp;
} LOG_DATA_t;

class LoggerWrapper : public Poco::Runnable
{
private:
    bool m_bRemoteLogger;
    bool m_bLocalLogger;

    string m_strLocalLogPath;
    string m_strLocalLogPathFilename;
    string m_strModuleName;

    bool m_bFileDebug;      // 檔案是否儲存 Debug 層級.
    bool m_bFileInfo;       // 檔案是否儲存 Info 層級.
    bool m_bFileWarning;    // 檔案是否儲存 Warning 層級.
    bool m_bFileError;      // 檔案是否儲存 Error 層級.
    bool m_bDisplayDebug;   // Terminal 是否顯示 Debug 層級.
    bool m_bDisplayInfo;    // Terminal 是否顯示 Info 層級.
    bool m_bDisplayWarning; // Terminal 是否顯示 Warning 層級.
    bool m_bDisplayError;   // Terminal 是否顯示 Error 層級.

    int m_iRemotePort;
    std::queue<LOG_DATA_t> m_LogQueue;
    Poco::Event m_evStoreLog;

    int DisplayAndQueueLog(string strLevel, string strMessage);
    bool m_bIsAlive;

public:
    LoggerWrapper(const std::string &strName);
    virtual void run();
    void stop();
    int Log(string strLevel, string strMessage);
    int Log(string strLevel, const char *szFormat, ...);
    int LogToRemote(string strLevel, const char *szFormat, ...);
    int LogToLocal(string strLevel, const char *szFormat, ...);

    void SetLogLevel(bool bFileDebug, bool bFileInfo, bool bFileWarning, bool bFileError, bool bDisplayDebug, bool bDisplayInfo, bool bDisplayWarning, bool bDisplayError);
    //    void Close();
};

extern LoggerWrapper goDriverLogger;
void InitLoggerWrapper();
void StopLoggerWrapper();
