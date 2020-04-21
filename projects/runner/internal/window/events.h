#pragma once

#include <variant>

#include "../../utils/utils.h"

namespace mff::window::events {

namespace window {

struct Resized {
    // TODO: new dimensions
};
struct CloseRequested {};
struct KeyboardInput {};
struct CursorMoved {};
struct MouseInput {};

using Event = std::variant<
    Resized,
    CloseRequested,
    KeyboardInput,
    CursorMoved,
    MouseInput
>;

}

struct WindowEvent {
    window::Event event;
};

struct PollingCompleted {};

using Event = std::variant<
    WindowEvent,
    PollingCompleted
>;

};