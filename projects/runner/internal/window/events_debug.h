#pragma once

#include "./events.h"

namespace mff::internal::window::events {

namespace window {

template<typename TOStream>
TOStream &operator<<(TOStream &os, const CloseRequested &ev) {
    os << "CloseRequested()";
    return os;
}

template<typename TOStream>
TOStream &operator<<(TOStream &os, const Resized &ev) {
    os << "Resized()";
    return os;
}

template<typename TOStream>
TOStream &operator<<(TOStream &os, const KeyboardInput &ev) {
    os << "KeyboardInput()";
    return os;
}

template<typename TOStream>
TOStream &operator<<(TOStream &os, const CursorMoved &ev) {
    os << "CursorMoved()";
    return os;
}

template<typename TOStream>
TOStream &operator<<(TOStream &os, const MouseInput &ev) {
    os << "MouseInput()";
    return os;
}

template<typename TOStream>
TOStream &operator<<(TOStream &os, const Event &ev) {
    std::visit([&](auto&& arg) { os << arg; }, ev);
    return os;
}

}


template<typename TOStream>
TOStream &operator<<(TOStream &os, const WindowEvent &ev) {
    os << "WindowEvent(" << ev.event << ")";
    return os;
}

template<typename TOStream>
TOStream &operator<<(TOStream &os, const PollingCompleted &ev) {
    os << "PollingCompleted()";
    return os;
}

template<typename TOStream>
TOStream &operator<<(TOStream &os, const Event &ev) {
    std::visit([&](auto&& arg) { os << arg; }, ev);
    return os;
}

}