#pragma once

#include <iterator>
#include <string>
#include <string_view>

#include <mff/parser_combinator/traits/input_iterator.h>

namespace mff::parser_combinator::traits::input {

template <typename T>
inline long offset(const T& from, const T& to) {
    static_assert(sizeof(T) == -1, "You have to have specialization for offset");
}

template <>
inline long offset<std::string_view>(const std::string_view& from, const std::string_view& to) {
    return to.data() - from.data();
}

}