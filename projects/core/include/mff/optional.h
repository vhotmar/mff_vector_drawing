#pragma once

namespace mff::optional {

template <typename T1, typename T2>
std::optional<T2> bind(std::optional<T1> a, std::function<std::optional<T2>(T1)> f) {
    if (a.has_value()) return f(a.value());
    return std::optional<T2>{};
}

template <typename T1, typename F>
std::optional<typename std::result_of<F(T1)>::type> map(std::optional<T1> a, F f) {
    if (a.has_value()) return f(a.value());
    return std::nullopt;
}

}