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

    for (const auto& [key, metric] : _metricsAccumulator.metrics()) {
        content << fmt::format("# TYPE {}_{} gauge\n", Prefix, key);

        content
            << fmt::format(
                "{0}_{1} {2}\n",
                Prefix,
                key,
                metric.value
            );
    }

    return content.str();
}