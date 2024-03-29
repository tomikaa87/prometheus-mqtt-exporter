#include "Configuration.h"
#include "HttpServer.h"
#include "LoggerFactory.h"
#include "MetricsAccumulator.h"
#include "MetricsPresenter.h"
#include "MqttClient.h"
#include "TaskQueue.h"

#include <csignal>
#include <memory>

namespace
{
    LoggerFactory loggerFactory;
    auto logger = loggerFactory.create("PrometheusMqttExporter");
    std::unique_ptr<TaskQueue> taskQueue;
    std::unique_ptr<HttpServer> httpServer;
    std::unique_ptr<MqttClient> mqttClient;
    std::unique_ptr<MetricsAccumulator> metricsAccumulator;
    std::unique_ptr<MetricsPresenter> metricsPresenter;
    bool shutdownInitiated = false;
}

void shutdown()
{
    if (shutdownInitiated) {
        logger.warn("Forcing shutdown");
        exit(1);
    }

    logger.info("Shutting down");

    if (!taskQueue) {
        return;
    }

    taskQueue->push("Shutdown", [](auto&) {
        taskQueue->shutdown();
    }, 1);

    if (httpServer) {
        taskQueue->push("HttpServerStop", [](auto&) {
            httpServer->stop();
        });
    }

    if (mqttClient) {
        taskQueue->push("MqttClientStop", [](auto&) {
            mqttClient->stop();
        });
    }

    shutdownInitiated = true;
}

void signalHandler(const int signal)
{
    if (signal == SIGTERM || signal == SIGINT) {
        shutdown();
    }
}

int main()
{
    signal(SIGTERM, signalHandler);
    signal(SIGINT, signalHandler);

    logger.info("Starting");

    Configuration configuration(loggerFactory);
    
    if (!configuration.readFromFile("config.json")) {
        logger.error("Can't read configuration file, exiting");
        return 1;
    }

    taskQueue = std::make_unique<TaskQueue>(loggerFactory);

    httpServer = std::make_unique<HttpServer>(
        loggerFactory,
        *taskQueue,
        HttpServer::Configuration{
            .port = configuration.http().serverPort
        }
    );

    mqttClient = std::make_unique<MqttClient>(
        loggerFactory,
        *taskQueue,
        MqttClient::Configuration{
            .clientId = configuration.mqtt().clientId,
            .brokerAddress = configuration.mqtt().brokerAddress,
            .brokerPort = configuration.mqtt().brokerPort,
            .username = configuration.mqtt().username,
            .password = configuration.mqtt().password
        }
    );

    metricsAccumulator = std::make_unique<MetricsAccumulator>(
        loggerFactory
    );

    metricsPresenter = std::make_unique<MetricsPresenter>(
        *metricsAccumulator
    );

    for (const auto& topic : configuration.mqtt().topics) {
        mqttClient->subscribe(topic);
    }

    mqttClient->setMessageReceivedHandler([](const std::string& topic, const std::vector<uint8_t>& payload) {
        if (!metricsAccumulator) {
            return;
        }

        metricsAccumulator->add(
            topic,
            std::string{
                reinterpret_cast<const char*>(payload.data()),
                payload.size()
            }
        );
    });

    httpServer->setRequestHandler([](HttpServer::Request& request) {
        if (request.endpoint() == "/metrics") {
            if (metricsPresenter) {
                request.setResponseContent(metricsPresenter->present());
            } else {
                request.setResponseCode(500);
                request.setResponseContent("MetricsPresenter is not initialized");
            }
        } else {
            // No content
            request.setResponseCode(204);
        }

        request.finish();
    });

    taskQueue->push("HttpServerStart", [](auto&) {
        httpServer->start();
    });

    taskQueue->push("MqttClientStart", [](auto&) {
        mqttClient->start();
    });

    taskQueue->exec();

    logger.info("Exiting");

    return 0;
}