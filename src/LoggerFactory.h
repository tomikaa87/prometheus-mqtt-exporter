#pragma once

#include <spdlog/common.h>
#include <spdlog/logger.h>

#include <vector>

class LoggerFactory final
{
public:
    LoggerFactory();

    [[nodiscard]] spdlog::logger create(std::string category) const;

private:
    const std::vector<spdlog::sink_ptr> _sinks;
};