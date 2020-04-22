#pragma once

#include <functional>
#include <optional>

#include "./events.h"

namespace mff::window {

/**
 * What should main thread do
 */
enum class ExecutionControl {
    /**
     * Poll for new events
     */
        Poll,

    /**
     * Wait for new events (suspend main thread)
     */
        Wait,

    /**
     * Terminate the program
     */
        Terminate
};

/**
 * Basic event loop (should be easy to extend for parallelization
 */
class EventLoop {
public:
    using RunCallback = std::function<ExecutionControl(events::Event)>;

    EventLoop();

    /**
     * Run the event loop with specified callback
     * @param callback {@link RunCallback} function to be run
     */
    void run(RunCallback callback);

    /**
     * Dispatch new event
     * @param event
     */
    void dispatch(events::Event event);

private:
    std::optional<RunCallback> callback_ = std::nullopt;
    std::optional<ExecutionControl> last_execution_control_ = ExecutionControl::Poll;
};

}