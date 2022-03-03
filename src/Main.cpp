#include "HttpServer.h"
#include "LoggerFactory.h"
#include "MqttClient.h"

int main()
{
    LoggerFactory loggerFactory;

    auto log{ loggerFactory.create("PrometheusMqttExporter") };

    log.info("Starting");

    HttpServer httpServer{ loggerFactory, HttpServer::Configuration{} };
    MqttClient mqttClient{ loggerFactory, MqttClient::Configuration{} };

    log.info("Exiting");

    return 0;
}