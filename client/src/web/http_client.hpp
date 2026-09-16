#ifndef HTTP_CLIENT_HPP
#define HTTP_CLIENT_HPP

#include <string>
#include <vector>

struct HttpResponse
{
    bool success;
    long statusCode;
    std::string body;
};

class HttpClient
{
public:
    static HttpResponse Get(const std::string &url, const std::vector<std::string> &headers);
    static std::string UrlEncode(const std::string &value);
};

#endif
