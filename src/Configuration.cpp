#include "Configuration.h"

#include <fstream>

#include <nlohmann/json.hpp>

namespace Objects
{
    static constexpr auto Http = "http";
    static constexpr auto Mqtt = "mqtt";
}

namespace Fields::Http
{
    static constexpr auto ServerPort = "serverPort";
}

namespace Fields::Mqtt
{
    static constexpr auto ClientId = "clientId";
    static constexpr auto BrokerAddress = "brokerAddress";
    static constexpr auto BrokerPort = "brokerPort";
    static constexpr auto Topics = "topics";
}

Configuration::Configuration(const LoggerFactory& loggerFactory)
    : _log{ loggerFactory.create("Configuration") }
{
    _log.info("Created");
}

bool Configuration::readFromFile(const std::string& path)
{
    _log.info("Reading from file: path={}", path);

    std::ifstream ifs{ path };
    auto json = nlohmann::json::parse(ifs, {}, false);

    if (!json.is_object()) {
        _log.warn("Can't read JSON configuration from file");

        return false;
    }

    processJson(json);

    return true;
}

void Configuration::processJson(const nlohmann::json& json)
{
    _log.debug("{}", __func__);

    if (json.contains(Objects::Http)) {
        processHttp(json[Objects::Http]);
    }

    if (json.contains(Objects::Mqtt)) {
        processMqtt(json[Objects::Mqtt]);
    }
}

void Configuration::processHttp(const nlohmann::json& json)
{
    _log.debug("{}", __func__);

    if (!json.is_object()) {
        _log.warn("'{}' is missing or not an object", Objects::Http);
        return;
    }

    if (
        json.contains(Fields::Http::ServerPort)
        && json[Fields::Http::ServerPort].is_number_integer()
    ) {
        _http.serverPort = json[Fields::Http::ServerPort];
        _log.info("{}.{}={}", Objects::Http, Fields::Http::ServerPort, _http.serverPort);
    }
}

void Configuration::processMqtt(const nlohmann::json& json)
{
    _log.debug("{}", __func__);

    if (!json.is_object()) {
        _log.warn("'{}' is missing or not an object", Objects::Mqtt);
        return;
    }

    if (
        json.contains(Fields::Mqtt::ClientId)
        && json[Fields::Mqtt::ClientId].is_string()
    ) {
        _mqtt.clientId = json[Fields::Mqtt::ClientId];
        _log.info("{}.{}={}", Objects::Mqtt, Fields::Mqtt::ClientId, _mqtt.clientId);
    }

    if (
        json.contains(Fields::Mqtt::BrokerAddress)
        && json[Fields::Mqtt::BrokerAddress].is_string()
    ) {
        _mqtt.brokerAddress = json[Fields::Mqtt::BrokerAddress];
        _log.info("{}.{}={}", Objects::Mqtt, Fields::Mqtt::BrokerAddress, _mqtt.brokerAddress);
    }

    if (
        json.contains(Fields::Mqtt::BrokerPort)
        && json[Fields::Mqtt::BrokerPort].is_number_integer()
    ) {
        _mqtt.brokerPort = json[Fields::Mqtt::BrokerPort];
        _log.info("{}.{}={}", Objects::Mqtt, Fields::Mqtt::BrokerPort, _mqtt.brokerPort);
    }

    if (
        json.contains(Fields::Mqtt::Topics)
        && json[Fields::Mqtt::Topics].is_array()
    ) {
        for (const auto& topic : json[Fields::Mqtt::Topics]) {
            if (!topic.is_string()) {
                continue;
            }

            _mqtt.topics.push_back(topic);
            _log.info("{}.{}={}", Objects::Mqtt, Fields::Mqtt::Topics, static_cast<std::string>(topic));
        }
    }
}