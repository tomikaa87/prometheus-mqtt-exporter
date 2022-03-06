#pragma once

#include "LoggerFactory.h"

#include <chrono>
#include <deque>
#include <map>
#include <string>

class MetricsAccumulator
{
public:
    explicit MetricsAccumulator(const LoggerFactory& loggerFactory);

    void add(const std::string& key, std::string value);

    struct Metric
    {
        std::string value;
        std::chrono::system_clock::time_point timestamp;
    };

    const auto& metrics() const
    {
        return _metrics;
    }

private:
    spdlog::logger _log;

    std::map<std::string, std::deque<Metric>> _metrics;
};