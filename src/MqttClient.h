#pragma once

#include "LoggerFactory.h"

#include <cstdint>
#include <string>

class TaskQueue;

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
        TaskQueue& taskQueue,
        Configuration config
    );

    void start();
    void stop();

private:
    spdlog::logger _log;
    TaskQueue& _taskQueue;
    const Configuration _config;
};

std::string toString(const MqttClient::Configuration& config);