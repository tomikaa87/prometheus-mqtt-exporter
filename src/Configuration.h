#pragma once

#include "LoggerFactory.h"

#include <cstdint>
#include <string>
#include <vector>

#include <nlohmann/json_fwd.hpp>

class Configuration
{
public:
    Configuration(const LoggerFactory& loggerFactory);

    bool readFromFile(const std::string& path);

    struct Http
    {
        uint16_t serverPort = 8888;
    };

    struct Mqtt
    {
        std::string clientId = "PrometheusMqttExporter";
        std::string brokerAddress = "localhost";
        uint16_t brokerPort = 1883;
        std::vector<std::string> topics;
        std::string username;
        std::string password;
    };

    const Http& http() const
    {
        return _http;
    }

    const Mqtt& mqtt() const
    {
        return _mqtt;
    }

private:
    spdlog::logger _log;
    Http _http;
    Mqtt _mqtt;

    void processJson(const nlohmann::json& json);
    void processHttp(const nlohmann::json& json);
    void processMqtt(const nlohmann::json& json);
};