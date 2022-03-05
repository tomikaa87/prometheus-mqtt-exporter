#pragma once

#include "LoggerFactory.h"

#include <cstdint>
#include <functional>
#include <future>
#include <map>
#include <string>
#include <vector>

#include <microhttpd.h>

class TaskQueue;

class HttpServer
{
public:
    struct Configuration
    {
        uint16_t port = 80;
        std::vector<std::string> allowedEndpoints{
            "/metrics"
        };
    };
    
    HttpServer(
        const LoggerFactory& loggerFactory,
        TaskQueue& taskQueue,
        Configuration config
    );

    void start();
    void stop();

    struct Request
    {
        const auto& endpoint() const
        {
            return _endpoint;
        }

        const auto& requestHeaders() const
        {
            return _requestHeaders;
        }

        void addResponseHeader(const std::string& key, std::string value)
        {
            _responseHeaders[key] = std::move(value);
        }

        void setResponseContent(std::string content)
        {
            _responseContent = std::move(content);
        }

        void setResponseCode(const int code)
        {
            _responseCode = code;
        }

        void finish()
        {
            _promise.set_value();
        }

    private:
        friend class HttpServer;

        Request(
            std::string endpoint,
            std::map<std::string, std::string>&& requestHeaders = {}
        )
            : _endpoint{ std::move(endpoint) }
            , _requestHeaders{ std::move(requestHeaders) }
        {}

        std::string _endpoint;
        std::map<std::string, std::string> _requestHeaders;
        std::promise<void> _promise;
        std::map<std::string, std::string> _responseHeaders;
        std::string _responseContent;
        int _responseCode = 200;
    };

    using RequestHandler = std::function<void (Request&)>;

    void setRequestHandler(RequestHandler&& handler)
    {
        _requestHandler = std::move(handler);
    }

private:
    spdlog::logger _log;
    TaskQueue& _taskQueue;
    const Configuration _config;
    struct MHD_Daemon* _mhdHandle = nullptr;
    RequestHandler _requestHandler;

    MHD_Result onMhdAccessPolicy(
        const struct sockaddr* addr,
        socklen_t addrlen
    );

    MHD_Result onMhdDefaultAccessHandler(
        struct MHD_Connection* connection,
        const char* url,
        const char* method,
        const char* version,
        const char* uploadData,
        size_t *uploadDataSize,
        void** conCls
    );
};

std::string toString(const HttpServer::Configuration& config);