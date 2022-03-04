#pragma once

#include "TaskQueue.h"

#include <variant>

template <typename TEvent, typename TState, typename TTransitions>
class StateMachine
{
public:
    StateMachine(
        TState initial,
        TTransitions transitions,
        TaskQueue& taskQueue
    )
        : _state{ std::move(initial) }
        , _transitions{ std::move(transitions) }
        , _taskQueue{ taskQueue }
    {}

    void dispatch(TEvent event)
    {
        _taskQueue.push(
            "StateMachineDispatch",
            [this, event{ std::move(event) }]([[maybe_unused]] auto& options) {
                auto newState = std::visit(_transitions, _state, event);
                if (newState) {
                    _state = std::move(*newState);
                }
            }
        );
    }

    const TState& state() const
    {
        return _state;
    }

private:
    TState _state;
    TTransitions _transitions;
    TaskQueue& _taskQueue;
};