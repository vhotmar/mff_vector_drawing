#include "event_loop.h"

#include "../glfw.h"

namespace mff::window {

EventLoop::EventLoop() = default;

void EventLoop::run(RunCallback callback) {
    callback_ = callback;

    while (true) {
        auto processing = *last_execution_control_;

        if (processing == ExecutionControl::Terminate) {
            return;
        }

        if (processing == ExecutionControl::Poll) {
            glfwPollEvents();

            if (*last_execution_control_ != processing) continue;
        }

        if (processing == ExecutionControl::Wait) {
            glfwWaitEvents();

            if (*last_execution_control_ != processing) continue;
        }

        last_execution_control_ = callback(events::Event(events::PollingCompleted{}));
    }
}

void EventLoop::dispatch(events::Event event) {
    if (!callback_) return;
    if (last_execution_control_ == ExecutionControl::Terminate) return;

    last_execution_control_ = (*callback_)(event);
}

}
