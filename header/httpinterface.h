#ifndef HTTPINTERFACE_H_
#define HTTPINTERFACE_H_

#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <stdio.h>
#include <assert.h>

#include "Poco/Task.h"
#include "Poco/TaskManager.h"
#include "Poco/TaskNotification.h"
#include "Poco/Observer.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTMLForm.h"
#include "Poco/URI.h"
#include "Poco/JSON/Object.h"
#include "Poco/Exception.h"

using namespace std;
using namespace Poco::Net;
using namespace Poco::JSON;

class Httpinterface
{
public:
    static int Get(std::string &result,
                   std::string str_uri);

    static int Post(std::string &result,
                    std::string str_uri,
                    map<std::string, std::string> headers,
                    std::string body);

    static void FormatJsonString(std::string &result, 
                                map<std::string, std::string> body);
                                
    static void FormatContentString(std::string &result, 
                                    string key, 
                                    string value, 
                                    bool last);
};

#endif
