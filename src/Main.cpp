#include "HttpServer.h"
#include "LoggerFactory.h"
#include "MqttClient.h"

#include <chrono>
#include <csignal>
#include <thread>

using namespace std::chrono_literals;

bool shutdownMainLoop = false;

void signalHandler(const int signal)
{
    if (signal == SIGTERM || signal == SIGINT) {
        shutdownMainLoop = true;
    }
}

int main()
{
    signal(SIGTERM, signalHandler);
    signal(SIGINT, signalHandler);

    LoggerFactory loggerFactory;

    auto log{ loggerFactory.create("PrometheusMqttExporter") };

    log.info("Starting");

    HttpServer httpServer{
        loggerFactory,
        HttpServer::Configuration{
            .port = 8888
        }
    };

    MqttClient mqttClient{ loggerFactory, MqttClient::Configuration{} };

    httpServer.start();

    while (!shutdownMainLoop) {
        std::this_thread::sleep_for(10ms);
    }

    httpServer.stop();

    log.info("Exiting");

    return 0;
}