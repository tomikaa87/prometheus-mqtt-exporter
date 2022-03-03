#pragma once

#include "LoggerFactory.h"

#include <cstdint>
#include <string>

class HttpServer
{
public:
    struct Configuration
    {
        uint16_t port = 80;
    };
    
    HttpServer(
        const LoggerFactory& loggerFactory,
        Configuration config
    );

private:
    spdlog::logger _log;
    const Configuration _config;
};

std::string toString(const HttpServer::Configuration& config);