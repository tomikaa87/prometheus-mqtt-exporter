#include "HttpServer.h"

HttpServer::HttpServer(
    const LoggerFactory& loggerFactory,
    Configuration config
)
    : _log{ loggerFactory.create("HttpServer") }
    , _config{ std::move(config) }
{
    _log.info(
        "Created: config={}",
        toString(_config)
    );
}

std::string toString(const HttpServer::Configuration& config)
{
    return fmt::format(
        "{{port={}}}",
        config.port
    );
}