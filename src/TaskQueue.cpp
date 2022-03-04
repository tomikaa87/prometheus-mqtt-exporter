#include "TaskQueue.h"

#include <thread>

using namespace std::chrono_literals;

TaskQueue::TaskQueue(const LoggerFactory& loggerFactory)
    : _log{ loggerFactory.create("TaskQueue") }
{
    _log.info("Created");
}

void TaskQueue::exec()
{
    _log.debug("{} started", __func__);

    while (!_shutdown) {
        while (!_queue.empty()) {
            auto elem = [this] {
                std::lock_guard lock{ _mutex };
                auto task = _queue.top();
                _queue.pop();
                return task;
            }();

            if (elem.task) {
                _log.debug(
                    "executing: id={}, priority={}, remaining={}",
                    elem.id,
                    elem.priority,
                    _queue.size()
                );

                TaskOptions taskOptions;
                elem.task(taskOptions);

                if (taskOptions.reQueue) {
                    _log.debug("re-queuing: id={}", elem.id);

                    if (taskOptions.after != std::chrono::milliseconds::zero()) {
                        _log.debug(
                            "moving task to wait queue: id={}, after={}ms",
                            elem.id,
                            taskOptions.after.count()
                        );

                        _waitQueue.push_back(
                            WaitQueueElement{
                                .queueElement = std::move(elem),
                                .reQueueTime = std::chrono::steady_clock::now() + taskOptions.after
                            }
                        );
                    } else {
                        std::lock_guard lock{ _mutex };
                        _queue.push(std::move(elem));
                    }
                }
            }
        }

        // Skip execution of delayed task in shutdown state
        if (_shutdown) {
            break;
        }

        const auto now = std::chrono::steady_clock::now();
        for (auto it = std::begin(_waitQueue); it != std::end(_waitQueue);) {
            if (now >= it->reQueueTime) {
                auto elem = std::move(it->queueElement);
                it = _waitQueue.erase(it);

                _log.debug(
                    "re-queuing delayed task: id={}, remaining={}",
                    elem.id,
                    _waitQueue.size()
                );

                std::lock_guard lock{ _mutex };
                _queue.push(std::move(elem));
            } else {
                ++it;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds{ 1 });
    }

    _log.debug("{} finished", __func__);
}

void TaskQueue::push(std::string id, Task task, const int priority)
{
    _log.debug("{}: id={}, priority={}", __func__, id, priority);

    std::lock_guard lock{ _mutex };
    _queue.push(
        QueueElement{
            .id = std::move(id),
            .task = std::move(task),
            .priority = priority
        }
    );
}

void TaskQueue::shutdown()
{
    _log.info("Shutting down");
    _shutdown = true;
}

bool TaskQueue::QueueElement::operator<(const QueueElement& o) const
{
    return priority > o.priority;
}