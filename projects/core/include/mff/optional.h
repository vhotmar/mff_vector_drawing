#pragma once

namespace mff::optional {

/**
 * Map optional (if the optional has value apply f, otherwise return nullopt)
 */
template <typename T1, typename F>
std::optional<typename std::result_of<F(T1)>::type> map(std::optional<T1> a, F f) {
    if (a.has_value()) return f(a.value());
    return std::nullopt;
}

}