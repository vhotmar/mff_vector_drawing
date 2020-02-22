#pragma once

#include <string>
#include <string_view>
#include <tuple>

#include <mff/parser_combinator/parser_result_inner.h>

namespace mff::parser_combinator::traits {

template <typename T>
class input_take_trait {
    static_assert(sizeof(T) == -1, "You have to have specialization for input_take");
};

template <>
class input_take_trait<std::string_view> {
public:
    static std::string_view take(const std::string_view& input, size_t count) {
        return input.substr(0, count);
    }

    static std::string_view slice(const std::string_view& input, size_t from, size_t to) {
        return input.substr(from, to);
    }

    static parser_result_inner<std::string_view, std::string_view>
    take_length(const std::string_view& input, size_t count) {
        return parser_result_inner(input.substr(count), input.substr(0, count));
    }
};

template <>
class input_take_trait<std::string> {
public:
    static std::string take(const std::string& input, size_t count) {
        return input.substr(0, count);
    }

    static std::string slice(const std::string& input, size_t from, size_t to) {
        return input.substr(from, to);
    }

    static parser_result_inner<std::string, std::string> take_length(const std::string& input, size_t count) {
        return parser_result_inner(input.substr(count), input.substr(0, count));
    }
};

namespace input {

template <typename T>
T take(const T& input, size_t count) {
    return input_take_trait<T>::take(input, count);
}

template <typename T>
T slice(const T& input, size_t from, size_t to) {
    return input_take_trait<T>::slice(input, from, to);
}

template <typename T>
parser_result_inner<T, T> take_length(const T& input, size_t count) {
    return input_take_trait<T>::take_length(input, count);
}

}

}
