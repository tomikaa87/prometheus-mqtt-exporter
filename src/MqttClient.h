#pragma once

#include "LoggerFactory.h"

#include <cstdint>
#include <string>

class MqttClient final
{
public:
    struct Configuration
    {
        std::string brokerAddress = "localhost";
        uint16_t brokerPort = 1883;
    };

    MqttClient(
        const LoggerFactory& loggerFactory,
        Configuration config
    );

private:
    spdlog::logger _log;
    const Configuration _config;
};

std::string toString(const MqttClient::Configuration& config);