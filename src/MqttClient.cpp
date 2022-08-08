#include "MqttClient.h"
#include "TaskQueue.h"

#include <chrono>
#include <numeric>

using namespace std::chrono_literals;

MqttClient::MqttClient(
    const LoggerFactory& loggerFactory,
    TaskQueue& taskQueue,
    Configuration config
)
    : _log{ loggerFactory.create("MqttClient") }
    , _taskQueue{ taskQueue }
    , _config{ std::move(config) }
    , _stateMachine{
        SM::States::Disconnected{},
        SM::Transitions{
            .self = *this
        },
        _taskQueue
    }
{
    _log.info("Created: config={}", toString(_config));
}

void MqttClient::start()
{
    _log.info("Starting");

    _reconnect = true;

    _stateMachine.dispatch(SM::Events::Connect{});
}

void MqttClient::stop()
{
    _log.info("Stopping");

    _reconnect = false;
    
    _stateMachine.dispatch(SM::Events::Disconnect{});
}

void MqttClient::subscribe(std::string topic)
{
    _log.info("Subscribing: topic={}", topic);

    if (std::holds_alternative<SM::States::Connected>(_stateMachine.state())) {
        if (const auto error = mosquitto_subscribe(_mosquitto, nullptr, topic.c_str(), 0); error != MOSQ_ERR_SUCCESS) {
            _log.warn(
                "Can't subscribe to topic: error={}",
                mosquitto_strerror(error)
            );
        }
    }

    _topics.push_back(std::move(topic));
}

void MqttClient::setMessageReceivedHandler(MessageReceivedHandler&& handler)
{
    _messageReceivedHandler = std::move(handler);
}

void MqttClient::reconnect()
{
    _log.debug("{}", __func__);

    _taskQueue.push("MqttReconnect", [this](auto& options) {
        // Avoid reconnecting if the client is already stopped
        if (!_reconnect) {
            return;
        }
        if (!options.reQueued) {
            options.reQueue = true;
            options.after = 5s;
            _log.info("Reconnecting in {}s", options.after.count() / 1000);
        } else {
            _log.info("Reconnecting");
            _stateMachine.dispatch(SM::Events::Connect{});
        }
    });
}

bool MqttClient::onConnect()
{
    _log.debug("{}", __func__);
    
    if (const auto error = mosquitto_lib_init(); error != MOSQ_ERR_SUCCESS) {
        _log.error(
            "Can't initialize Mosquitto lib: error={}",
            mosquitto_strerror(error)
        );

        return false;
    }

    if (_mosquitto) {
        mosquitto_destroy(_mosquitto);
    }

    _mosquitto = mosquitto_new(_config.clientId.c_str(), true, this);

    if (!_config.username.empty()) {
        mosquitto_username_pw_set(_mosquitto, _config.username.c_str(), _config.password.c_str());
    }

    mosquitto_connect_callback_set(_mosquitto, [](auto*, void* obj, const int rc) {
        auto* self = reinterpret_cast<MqttClient*>(obj);

        if (rc != MOSQ_ERR_SUCCESS) {
            self->_log.warn(
                "Can't connect to MQTT broker: error={}",
                mosquitto_strerror(rc)
            );

            self->_stateMachine.dispatch(MqttClient::SM::Events::Disconnected{});
        } else {
            self->_stateMachine.dispatch(MqttClient::SM::Events::Connected{});
        }
    });

    mosquitto_disconnect_callback_set(_mosquitto, [](auto*, void* obj, const int rc) {
        auto* self = reinterpret_cast<MqttClient*>(obj);

        self->_log.info(
            "Disconnected from MQTT broker: reason={}",
            mosquitto_strerror(rc)
        );

        self->_stateMachine.dispatch(MqttClient::SM::Events::Disconnected{});
    });

    mosquitto_publish_callback_set(_mosquitto, [](auto*, void* obj, const int mid) {
        auto* self = reinterpret_cast<MqttClient*>(obj);

        self->_taskQueue.push("MqttOnPublish", [self, mid](auto&) {
            self->onPublish(mid);
        });
    });

    mosquitto_subscribe_callback_set(
        _mosquitto,
        [](auto*, void* obj, const int mid, const int qosCount, const int* grantedQos) {
            auto* self = reinterpret_cast<MqttClient*>(obj);

            std::vector<int> v;
            v.resize(qosCount);
            std::memcpy(v.data(), grantedQos, qosCount * sizeof(int));

            self->_taskQueue.push("MqttOnSubscribe", [self, mid, grantedQos{ std::move(v) }](auto&) {
                self->onSubscribe(mid, std::move(grantedQos));
            });
        }
    );

    mosquitto_message_callback_set(_mosquitto, [](auto*, void* obj, const auto* msg) {
        auto* self = reinterpret_cast<MqttClient*>(obj);

        std::vector<uint8_t> payload;
        payload.resize(msg->payloadlen);
        std::memcpy(payload.data(), msg->payload, msg->payloadlen);

        std::string topic{ msg->topic };

        self->_taskQueue.push(
            "MqttOnMessage",
            [
                self,
                payload{ std::move(payload) },
                topic{std::move(topic) },
                mid{ msg->mid },
                qos{ msg->qos },
                retain{ msg->retain }
            ](auto&) {
                self->onMessage(mid, std::move(topic), std::move(payload), qos, retain);
            }
        );
    });

    if (const auto error = mosquitto_loop_start(_mosquitto); error != MOSQ_ERR_SUCCESS) {
        _log.warn(
            "Can't start Mosquitto loop: error={}",
            mosquitto_strerror(error)
        );

        return false;
    }

    if (
        const auto error = mosquitto_connect_async(
            _mosquitto,
            _config.brokerAddress.c_str(),
            _config.brokerPort,
            _config.keepAlive
        ); error != MOSQ_ERR_SUCCESS
    ) {
        _log.warn(
            "Can't connect to MQTT broker: error={}",
            mosquitto_strerror(error)
        );

        if (_reconnect) {
            reconnect();
        }

        return false;
    }

    return true;
}

void MqttClient::onConnected()
{
    _log.debug("{}", __func__);

    for (const auto& topic : _topics) {
        _log.info("Subscribing: topic={}", topic);

        if (!std::holds_alternative<SM::States::Connected>(_stateMachine.state())) {
            if (const auto error = mosquitto_subscribe(_mosquitto, nullptr, topic.c_str(), 0); error != MOSQ_ERR_SUCCESS) {
                _log.warn(
                    "Can't subscribe to topic: error={}",
                    mosquitto_strerror(error)
                );
            }
        }
    }
}

bool MqttClient::onDisconnect()
{
    _log.debug("{}", __func__);
    
    if (const auto error = mosquitto_disconnect(_mosquitto); error != MOSQ_ERR_SUCCESS) {
        _log.warn(
            "Can't disconnect from MQTT broker: error={}",
            mosquitto_strerror(error)
        );

        if (error == MOSQ_ERR_INVAL) {
            return false;
        }
    }

    _taskQueue.push("MqttDisconnectCheck", [this](auto& options) {
        if (!std::holds_alternative<SM::States::Disconnected>(_stateMachine.state())) {
            options.reQueue = true;
            options.after = 500ms;
        }
    });

    return true;
}

void MqttClient::onDisconnected()
{
    _log.debug("{}", __func__);

    if (const auto error = mosquitto_disconnect(_mosquitto); error != MOSQ_ERR_SUCCESS) {
        if (error != MOSQ_ERR_NO_CONN) {
            _log.warn(
                "Can't disconnect from MQTT broker: error={}",
                mosquitto_strerror(error)
            );
        }
    }

    if (const auto error = mosquitto_loop_stop(_mosquitto, false) != MOSQ_ERR_SUCCESS) {
        _log.warn(
            "Can't stop Mosquitto's loop: error={}",
            mosquitto_strerror(error)
        );
    }

    if (_mosquitto) {
        mosquitto_destroy(_mosquitto);
        _mosquitto = nullptr;
    }

    mosquitto_lib_cleanup();

    if (_reconnect) {
        reconnect();
    }
}

void MqttClient::onPublish(const int messageId)
{
    _log.debug("{}: messageId={}", __func__, messageId);
}

void MqttClient::onSubscribe(const int messageId, std::vector<int> grantedQos)
{
    _log.debug(
        "{}: messageId={}, grantedQos={}",
        __func__,
        messageId,
        std::accumulate(
            std::cbegin(grantedQos),
            std::cend(grantedQos),
            std::string{},
            [](const auto& s, const auto& qos) {
                return s + (!s.empty() ? "," : "") + std::to_string(qos);
            }
        )
    );
}

void MqttClient::onMessage(
    const int messageId,
    std::string topic,
    std::vector<uint8_t> payload,
    const int qos,
    const bool retain
) {
    _log.debug(
        "{}: messageId={}, topic={}, payload={}, qos={}, retain={}",
        __func__,
        messageId,
        topic,
        std::accumulate(
            std::cbegin(payload),
            std::cend(payload),
            std::string{},
            [](const auto& s, const auto& v) {
                return s + (!s.empty() ? " " : "") + fmt::format("{:02X}", v);
            }
        ),
        qos,
        retain
    );

    if (_messageReceivedHandler) {
        _messageReceivedHandler(std::move(topic), std::move(payload));
    }
}

std::string toString(const MqttClient::Configuration& config)
{
    return fmt::format(
        "{{brokerAddress={},brokerPort={}}}",
        config.brokerAddress,
        config.brokerPort
    );
}