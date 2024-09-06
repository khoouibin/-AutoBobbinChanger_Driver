#include "httpinterface.h"


int Httpinterface::Get(std::string &result, std::string str_uri)
{
    const char *cstr = str_uri.c_str();
    try
    {
        Poco::URI uri(cstr);
        std::string path(uri.getPathAndQuery());
        if (path.empty())
            path = "/";

        HTTPClientSession session(uri.getHost(), uri.getPort());
        HTTPRequest request(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);
        HTTPResponse response;

        request.setContentType("application/json");
        std::ostream &o = session.sendRequest(request);
        std::istream &s = session.receiveResponse(response);

        if (response.getStatus() != Poco::Net::HTTPResponse::HTTP_OK)
            return -1;

        else {
            std::istreambuf_iterator<char> eos;
            std::string temp(std::istreambuf_iterator<char>(s), eos);
            result = temp;
            return  0;
        }
    }
    catch (Poco::Exception &ex)
    {
        result = ex.displayText();
        return -1;
    }
    return 0;
}


int Httpinterface::Post(std::string &result,
                        std::string str_uri,
                        map<std::string, std::string> headers,
                        std::string body)
{
    std::string str_uri_full = str_uri;
    str_uri_full = str_uri_full + "?";

    int i_param_num = 0;

    for (map<string, string>::iterator it = headers.begin(); it != headers.end(); it++)
    {
        if (i_param_num == 0)
            str_uri_full = str_uri_full + it->first + "=" + it->second;
        else
            str_uri_full = str_uri_full + "&" + it->first + "=" + it->second;
        i_param_num++;
    }
    const char *cstr = str_uri_full.c_str();

    try
    {
        Poco::URI uri(cstr);
        std::string path(uri.getPathAndQuery());
        if (path.empty())
            path = "/";

        HTTPClientSession session(uri.getHost(), uri.getPort());
        HTTPRequest request(HTTPRequest::HTTP_POST, path, HTTPMessage::HTTP_1_1);
        HTTPResponse response;

        request.setContentType("application/x-www-form-urlencoded");
        request.setContentLength(body.length());

        std::ostream &o = session.sendRequest(request) << body;

        std::istream &s = session.receiveResponse(response);

        if (response.getStatus() != Poco::Net::HTTPResponse::HTTP_OK)
            return -1;
        else {
            std::istreambuf_iterator<char> eos;
            std::string temp(std::istreambuf_iterator<char>(s), eos);
            result = temp;
        }
    }
    catch (Poco::Exception &ex)
    {
        result = ex.displayText();
        return -1;
    }
    return 0;
}

void Httpinterface::FormatJsonString(std::string &result, map<std::string, std::string> body){
    int count = 0;
    string content = "", key, value;
    for (map<string, string>::iterator it = body.begin(); it != body.end(); ++it){
        key = it->first;
        value = it->second;
        if(count == body.size()-1){
            FormatContentString(content, key, value, true);
        }
        else{
            FormatContentString(content, key, value, false);
        }
        count++;
    }
    result = "{"+ content +"}";
}
void Httpinterface::FormatContentString(std::string &result, string key, string value, bool last){
    string temp = "";
    if (last == true){
        temp = "\"" + key + "\" : \"" + value + "\"";
    }
    else{
        temp = "\"" + key + "\" : \"" + value + "\",";
    }
    result = result + temp;
}