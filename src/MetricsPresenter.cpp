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

    static constexpr auto Prefix = "mqtt";

    for (const auto& [key, metric] : _metricsAccumulator.metrics()) {
        const auto& [value, timestamp] = metric;

        content
            << fmt::format("# TYPE {}_{} untyped\n", Prefix, key)
            << fmt::format(
                "{0}_{1} {2} {3:d}\n",
                Prefix,
                key,
                value,
                timestamp.time_since_epoch().count()
            )
            ;
    }

    return content.str();
}