#pragma once

#include "LoggerFactory.h"
#include "StateMachine.h"

#include <cstdint>
#include <string>
#include <optional>
#include <type_traits>
#include <vector>

#include <mosquitto.h>

class TaskQueue;

class MqttClient final
{
public:
    struct Configuration
    {
        std::string clientId;
        std::string brokerAddress;
        uint16_t brokerPort = 0;
        int keepAlive = 5;
    };

    MqttClient(
        const LoggerFactory& loggerFactory,
        TaskQueue& taskQueue,
        Configuration config
    );

    void start();
    void stop();
    void subscribe(std::string topic);

    using MessageReceivedHandler = std::function<void (std::string topic, std::vector<uint8_t> payload)>;
    void setMessageReceivedHandler(MessageReceivedHandler&& handler);

private:
    spdlog::logger _log;
    TaskQueue& _taskQueue;
    const Configuration _config;
    struct mosquitto* _mosquitto = nullptr;
    bool _reconnect = false;
    std::vector<std::string> _topics;
    MessageReceivedHandler _messageReceivedHandler;
    
    struct SM
    {
        struct Events
        {
            struct Connect {};
            struct Connected {};
            struct Disconnect {};
            struct Disconnected {};
        };

        struct States
        {
            struct Connecting {};
            struct Connected {};
            struct Disconnecting {};
            struct Disconnected {};
        };

        using Event = std::variant<
            Events::Connect,
            Events::Connected,
            Events::Disconnect,
            Events::Disconnected
        >;

        using State = std::variant<
            States::Connecting,
            States::Connected,
            States::Disconnecting,
            States::Disconnected
        >;

        struct Transitions
        {
            MqttClient& self;

            std::optional<State> operator()(const States::Disconnected&, const Events::Connect&)
            {
                if (self.onConnect()) {
                    return States::Connecting{};
                }
                return std::nullopt;
            }

            std::optional<State> operator()(const States::Connecting&, const Events::Connected&)
            {
                self.onConnected();
                return States::Connected{};
            }

            std::optional<State> operator()(const auto& state, const Events::Disconnect&)
            {
                using TState = std::decay_t<decltype(state)>;

                if constexpr (!std::is_same_v<TState, States::Disconnected>) {
                    if (self.onDisconnect()) {
                        return States::Disconnecting{};
                    }
                    return std::nullopt;
                } else {
                    return std::nullopt;
                }
            }

            std::optional<State> operator()(const auto& state, const Events::Disconnected&)
            {
                using TState = std::decay_t<decltype(state)>;

                if constexpr (!std::is_same_v<TState, States::Disconnected>) {
                    self.onDisconnected();
                    return States::Disconnected{};
                } else {
                    return std::nullopt;
                }
            }

            std::optional<State> operator()(const auto& event, const auto& state)
            {
                self._log.warn(
                    "Unsupported SM transition: event={}, state={}",
                    typeid(decltype(event)).name(),
                    typeid(decltype(state)).name()
                );
                return std::nullopt;
            }
        };
    };

    StateMachine<SM::Event, SM::State, SM::Transitions> _stateMachine;

    void reconnect();

    bool onConnect();
    void onConnected();
    bool onDisconnect();
    void onDisconnected();
    void onPublish(int messageId);
    void onSubscribe(int messageId, std::vector<int> grantedQos);
    void onMessage(
        int messageId,
        std::string topic,
        std::vector<uint8_t> payload,
        int qos,
        bool retain
    );
};

std::string toString(const MqttClient::Configuration& config);