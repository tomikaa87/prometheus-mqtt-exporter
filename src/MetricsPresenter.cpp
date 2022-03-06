#include "MetricsAccumulator.h"
#include "MetricsPresenter.h"

#include <chrono>
#include <sstream>

MetricsPresenter::MetricsPresenter(
    const MetricsAccumulator& metricsAccumulator
)
    : _metricsAccumulator{ metricsAccumulator }
{}

std::string MetricsPresenter::present() const
{
    std::stringstream content;

    static constexpr auto Prefix = "mqtt";

    for (const auto& [key, metricQueue] : _metricsAccumulator.metrics()) {
        content << fmt::format("# TYPE {}_{} gauge\n", Prefix, key);

#if 1
        content
            << fmt::format(
                "{0}_{1} {2}\n",
                Prefix,
                key,
                metricQueue.front().value
            );
#endif

#if 0
        for (auto it = std::cbegin(metricQueue); it != std::cend(metricQueue); ++it) {
            const auto& [value, timestamp] = *it;

            content
                << fmt::format(
                    "{0}_{1} {2} {3:d}\n",
                    Prefix,
                    key,
                    value,
                    std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch()).count()
                );

            // Present the current value without a timestamp

            const auto last = (it != std::cend(metricQueue)) && (it + 1 == std::cend(metricQueue));

            if (last) {
                content
                    << fmt::format(
                        "{0}_{1} {2}\n",
                        Prefix,
                        key,
                        value
                    );    
            }
        }
#endif
    }

    return content.str();
}