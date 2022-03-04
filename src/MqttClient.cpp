#include "MqttClient.h"
#include "TaskQueue.h"

#include <chrono>

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

    _stateMachine.dispatch(SM::Events::Connect{});
}

void MqttClient::stop()
{
    _log.info("Stopping");
    
    _stateMachine.dispatch(SM::Events::Disconnect{});
}

bool MqttClient::onConnect()
{
    _log.debug("onConnect");
    
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

    _mosquitto = mosquitto_new("PrometheusMqttExporter", true, this);

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

        return false;
    }

    return true;
}

void MqttClient::onConnected()
{
    _log.debug("onConnected");
}

bool MqttClient::onDisconnect()
{
    _log.debug("onDisconnect");
    
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
    _log.debug("onDisconnected");

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
}

std::string toString(const MqttClient::Configuration& config)
{
    return fmt::format(
        "{{brokerAddress={},brokerPort={}}}",
        config.brokerAddress,
        config.brokerPort
    );
}

/*

int major, minor, patch;
    mosquitto_lib_version(&major, &minor, &patch);
    std::cout << "libmosquitto version: " << major << '.' << minor << '.' << patch << '\n';

    std::cout << "Initializing libmosquitto\n";
    if (mosquitto_lib_init() != MOSQ_ERR_SUCCESS) {
        std::cout << "Failed to initialize libmosquitto\n";
        return 1;
    }

    struct State
    {
        std::promise<bool> connected;
        std::promise<bool> published;
        std::promise<std::string> received;
    } state;

    std::unique_ptr<struct mosquitto, decltype(&mosquitto_destroy)> client{
        mosquitto_new(nullptr, true, &state),
        &mosquitto_destroy
    };

    bool forceStop = false;

    std::cout << "Starting Mosquitto loop\n";
    if (mosquitto_loop_start(client.get()) != MOSQ_ERR_SUCCESS) {
        std::cout << "Failed to start Mosquitto loop\n";
        return 1;
    }

    mosquitto_connect_callback_set(client.get(), [](auto* mcli, auto* obj, auto rc) {
        std::cout << "ConnectCB: rc=" << rc << ", thread=" << std::this_thread::get_id() << '\n';
        reinterpret_cast<State*>(obj)->connected.set_value(rc == 0);
    });

    mosquitto_publish_callback_set(client.get(), [](auto* mcli, auto* obj, auto mid) {
        std::cout << "PublishCB: mid=" << mid << ", thread=" << std::this_thread::get_id() << '\n';
        reinterpret_cast<State*>(obj)->published.set_value(mid > 0);
    });

    mosquitto_message_callback_set(client.get(), [](auto* mcli, auto* obj, const auto* msg) {
        std::string payload{ reinterpret_cast<const char*>(msg->payload), static_cast<std::size_t>(msg->payloadlen) };
        std::cout << "MessageCB: payload=" << payload << '\n';
        reinterpret_cast<State*>(obj)->received.set_value(std::move(payload));
    });

    std::cout << "Connecting\n";
    if (const auto result = mosquitto_connect(client.get(), "naspi.home", 1883, 10); result != MOSQ_ERR_SUCCESS) {
        std::cout << "Failed to connect: result=" << result << "\n";
    }

    std::cout << "Main thread ID: " << std::this_thread::get_id() << '\n';

    std::cout << "Waiting for connection\n";
    auto connectedFuture = state.connected.get_future();
    if (connectedFuture.wait_for(5s) == std::future_status::ready) {
        bool connected = connectedFuture.get();
        std::cout << "Future ready: connected=" << connected << '\n';

        if (connected) {
            std::cout << "Publishing test message\n";
            std::string payload{ "almafa123" };
            mosquitto_publish(client.get(), nullptr, "test/mosquitto/exporter", payload.size(), payload.c_str(), 0, 1);

            auto publishedFuture = state.published.get_future();
            if (publishedFuture.wait_for(5s) == std::future_status::ready) {
                bool published = publishedFuture.get();
                std::cout << "Future ready: published=" << published << '\n';
            }

            std::cout << "Subscribing to test topic\n";
            if (mosquitto_subscribe(client.get(), nullptr, "home/temperature/bedroom", 0) != MOSQ_ERR_SUCCESS) {
                std::cout << "Failed to subscribe to test topic\n";
                state.received.set_value("failure");
            }

            auto receivedFuture = state.received.get_future();
            if (receivedFuture.wait_for(60s) == std::future_status::ready) {
                const auto payload = receivedFuture.get();
                std::cout << "Received message: payload=" << payload << '\n';
            }
        }
    }

    std::cout << "Disconnecting\n";
    if (mosquitto_disconnect(client.get()) != MOSQ_ERR_SUCCESS) {
        std::cout << "Disconnect failed\n";
        forceStop = true;
    }

    std::cout << "Stopping Mosquitto loop\n";
    if (mosquitto_loop_stop(client.get(), forceStop) != MOSQ_ERR_SUCCESS) {
        std::cout << "Failed to stop Mosquitto loop\n";
    }

    std::cout << "Cleaning up libmosquitto\n";
    mosquitto_lib_cleanup();

*/