#pragma once

#include "LoggerFactory.h"

#include <chrono>
#include <map>
#include <string>

class MetricsAccumulator
{
public:
    explicit MetricsAccumulator(const LoggerFactory& loggerFactory);

    void add(const std::string& key, std::string value);

private:
    spdlog::logger _log;

    struct Metric
    {
        std::string value;
        std::chrono::system_clock::time_point timestamp;
    };

    std::map<std::string, Metric> _metrics;
};