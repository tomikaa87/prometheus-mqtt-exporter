#include "MqttClient.h"
#include "TaskQueue.h"

MqttClient::MqttClient(
    const LoggerFactory& loggerFactory,
    TaskQueue& taskQueue,
    Configuration config
)
    : _log{ loggerFactory.create("MqttClient") }
    , _taskQueue{ taskQueue }
    , _config{ std::move(config) }
{
    _log.info("Created: config={}", toString(_config));
}

void MqttClient::start()
{
    _log.info("Starting");
}

void MqttClient::stop()
{
    _log.info("Stopping");
}

std::string toString(const MqttClient::Configuration& config)
{
    return fmt::format(
        "{{brokerAddress={},brokerPort={}}}",
        config.brokerAddress,
        config.brokerPort
    );
}