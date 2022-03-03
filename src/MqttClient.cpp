#include "MqttClient.h"

MqttClient::MqttClient(
    const LoggerFactory& loggerFactory,
    Configuration config
)
    : _log{ loggerFactory.create("MqttClient") }
    , _config{ std::move(config) }
{
    _log.info("Created: config={}", toString(_config));
}

std::string toString(const MqttClient::Configuration& config)
{
    return fmt::format(
        "{{brokerAddress={},brokerPort={}}}",
        config.brokerAddress,
        config.brokerPort
    );
}