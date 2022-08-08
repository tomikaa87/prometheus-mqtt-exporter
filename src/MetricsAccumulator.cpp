#include "MetricsAccumulator.h"

MetricsAccumulator::MetricsAccumulator(const LoggerFactory& loggerFactory)
    : _log{ loggerFactory.create("MetricsAccumulator") }
{
    _log.info("Created");
}

void MetricsAccumulator::add(const std::string& key, std::string value)
{
    if (value.find_first_not_of("0123456789.") != std::string::npos) {
        _log.debug("Add: ignoring non-numeric value, key={}, value={}", key, value);
        return;
    }

    const auto timestamp = std::chrono::system_clock::now();

    const auto keyTransformer = [](const char c) {
        if (c == '/') {
            return '_';
        }

        return c;
    };

    std::string metricKey;
    std::transform(
        std::cbegin(key),
        std::cend(key),
        std::back_inserter(metricKey),
        keyTransformer
    );

    _log.debug(
        "Add: key={}, metricKey={}, value={}, timestamp={:d}",
        key,
        metricKey,
        value,
        std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch()).count()
    );

    _metrics[metricKey] = Metric{
        .value = std::move(value),
        .timestamp = std::move(timestamp)
    };
}