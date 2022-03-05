#pragma once

#include <string>

class MetricsAccumulator;

class MetricsPresenter
{
public:
    explicit MetricsPresenter(
        const MetricsAccumulator& metricsAccumulator
    );

    std::string present() const;

private:
    const MetricsAccumulator& _metricsAccumulator;
};