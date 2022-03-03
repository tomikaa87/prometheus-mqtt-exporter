#include <fmt/core.h>
#include <spdlog/sinks/stdout_sinks.h>

int main()
{
    const auto consoleSink = std::make_shared<spdlog::sinks::stdout_sink_mt>();
    consoleSink->set_level(spdlog::level::debug);

    const spdlog::sinks_init_list sinks{ consoleSink };

    spdlog::logger logger{ "main", sinks };
    spdlog::logger logger2{ "main2", sinks };

    logger.debug("test");
    logger2.debug("test2");

    return 0;
}