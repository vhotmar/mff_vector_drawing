#pragma once

#include <cstddef>
#include <tl/expected.hpp>

template <typename I, typename O, typename E>
using IResult = tl::expected<std::tuple<I, O>, E>;

/*template <class From, class To>
concept convertible_to = std::is_convertible_v<From, To> && requires(From (& f)()) {
    static_cast<To>(f());
};

template <typename T>
concept InputLength = requires(T a) {
    { a.length() } -> convertible_to<std::size_t>;
};*/