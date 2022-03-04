#pragma once

#include "LoggerFactory.h"

#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <string>
#include <mutex>
#include <queue>

class TaskQueue
{
public:
    TaskQueue(const LoggerFactory& loggerFactory);

    void exec();

    struct TaskOptions
    {
        bool reQueue = false;
        bool reQueued = false;
        std::chrono::milliseconds after = std::chrono::milliseconds::zero();
    };

    using Task = std::function<void (TaskOptions& options)>;

    void push(std::string id, Task task, int priority = 0);

    void shutdown();

private:
    spdlog::logger _log;

    struct QueueElement
    {
        std::string id;
        Task task;
        int priority = 0;
        bool reQueued = false;

        bool operator<(const QueueElement& o) const;
    };

    struct WaitQueueElement
    {
        QueueElement queueElement;
        std::chrono::steady_clock::time_point reQueueTime;
    };

    std::mutex _mutex;
    std::priority_queue<QueueElement> _queue;
    std::deque<WaitQueueElement> _waitQueue;
    std::atomic_bool _shutdown{ false };
};