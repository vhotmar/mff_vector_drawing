#pragma once

#include <iterator>
#include <string>
#include <string_view>

#include <mff/parser_combinator/traits/input_iterator.h>

namespace mff::parser_combinator::traits::input {

template <typename T>
long length(const T& from) {
    input_iterator <T> trait;

    return std::distance(trait.begin(), trait.end());
}

template <>
long length<std::string_view>(const std::string_view& from) {
    return from.size();
}

template <>
long length<std::string>(const std::string& from) {
    return from.size();
}

}