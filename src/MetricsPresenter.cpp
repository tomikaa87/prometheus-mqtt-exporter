#include "MetricsAccumulator.h"
#include "MetricsPresenter.h"

#include <sstream>

MetricsPresenter::MetricsPresenter(
    const MetricsAccumulator& metricsAccumulator
)
    : _metricsAccumulator{ metricsAccumulator }
{}

std::string MetricsPresenter::present() const
{
    std::stringstream content;

    for (const auto& [key, metric] : _metricsAccumulator.metrics()) {
        const auto& [value, timestamp] = metric;

        content
            << fmt::format("# TYPE {} untyped\n", key)
            << fmt::format(
                "mqtt_{0} {1} {2:d}\n",
                key,
                value,
                timestamp.time_since_epoch().count()
            )
            ;
    }

    return content.str();
}