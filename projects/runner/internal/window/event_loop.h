#pragma once

#include <functional>
#include <optional>

#include "./events.h"

namespace mff::window {

enum class ExecutionControl {
    Poll,
    Wait,
    Terminate
};

class EventLoop {
public:
    using RunCallback = std::function<ExecutionControl(events::Event)>;

    EventLoop();

    void run(RunCallback callback);

    void dispatch(events::Event event);

private:
    std::optional<RunCallback> callback_;
    std::optional<ExecutionControl> last_execution_control_ = ExecutionControl::Poll;
};

}