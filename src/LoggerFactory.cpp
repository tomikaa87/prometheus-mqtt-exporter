#include "LoggerFactory.h"

#include <spdlog/sinks/stdout_sinks.h>

LoggerFactory::LoggerFactory()
    : _sinks{
        std::make_shared<spdlog::sinks::stdout_sink_mt>()
    }
{}

spdlog::logger LoggerFactory::create(std::string category) const
{
    auto logger = spdlog::logger{ std::move(category), std::begin(_sinks), std::end(_sinks) };
    logger.set_level(spdlog::level::debug);
    return logger;
}