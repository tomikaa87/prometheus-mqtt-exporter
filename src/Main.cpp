#include "HttpServer.h"
#include "LoggerFactory.h"
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
}

void shutdown()
{
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

    taskQueue = std::make_unique<TaskQueue>(loggerFactory);

    httpServer = std::make_unique<HttpServer>(
        loggerFactory,
        *taskQueue,
        HttpServer::Configuration{
            .port = 8888
        }
    );

    mqttClient = std::make_unique<MqttClient>(
        loggerFactory,
        *taskQueue,
        MqttClient::Configuration{}
    );

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