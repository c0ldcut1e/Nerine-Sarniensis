#include "http_client.hpp"

#include <curl/curl.h>

constexpr long kRequestTimeoutSeconds = 10;

static size_t WriteCallback(char *data, size_t size, size_t count, void *userData)
{
    std::string *body = (std::string *) userData;
    body->append(data, size * count);
    return size * count;
}

HttpResponse HttpClient::Get(const std::string &url, const std::vector<std::string> &headers)
{
    HttpResponse response;
    response.success    = false;
    response.statusCode = 0;

    CURL *curl = curl_easy_init();
    if (!curl)
    {
        return response;
    }

    curl_slist *headerList = NULL;
    for (const std::string &header : headers)
    {
        headerList = curl_slist_append(headerList, header.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, kRequestTimeoutSeconds);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode result = curl_easy_perform(curl);
    if (result == CURLE_OK)
    {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.statusCode);
        response.success = response.statusCode >= 200 && response.statusCode < 300;
    }

    curl_slist_free_all(headerList);
    curl_easy_cleanup(curl);

    return response;
}

std::string HttpClient::UrlEncode(const std::string &value)
{
    CURL *curl = curl_easy_init();
    if (!curl)
    {
        return value;
    }

    char *encoded      = curl_easy_escape(curl, value.c_str(), (int) value.size());
    std::string result = encoded ? encoded : value;

    curl_free(encoded);
    curl_easy_cleanup(curl);

    return result;
}
