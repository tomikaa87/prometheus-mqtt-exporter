#pragma once

#include "LoggerFactory.h"

#include <cstdint>
#include <string>

#include <microhttpd.h>

class TaskQueue;

class HttpServer
{
public:
    struct Configuration
    {
        uint16_t port = 80;
    };
    
    HttpServer(
        const LoggerFactory& loggerFactory,
        TaskQueue& taskQueue,
        Configuration config
    );

    void start();
    void stop();

private:
    spdlog::logger _log;
    TaskQueue& _taskQueue;
    const Configuration _config;
    struct MHD_Daemon* _mhdHandle = nullptr;

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