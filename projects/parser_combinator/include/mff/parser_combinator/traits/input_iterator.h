#pragma once

#include <iterator>
#include <optional>
#include <string>

namespace mff::parser_combinator::traits {

template <typename T>
class InputIteratorTrait {
public:
    static_assert(sizeof(T) == -1, "You have to have specialization for input_iter");
};

template <>
class InputIteratorTrait<std::string> {
public:
    using value_type = std::string::value_type;
    using const_iterator = std::string::const_iterator;

    const_iterator begin(const std::string& from) const {
        return from.begin();
    };

    const_iterator end(const std::string& from) const {
        return from.end();
    };
};

template <>
class InputIteratorTrait<std::string_view> {
public:
    using value_type = std::string_view::value_type;
    using const_iterator = std::string_view::const_iterator;

    const_iterator begin(const std::string_view& from) const {
        return from.begin();
    };

    const_iterator end(const std::string_view& from) const {
        return from.end();
    };
};

namespace iterator {

template <typename T>
using value_type_t = typename InputIteratorTrait<T>::value_type;

template <typename T>
auto begin(const T& from) {
    InputIteratorTrait<T> trait;

    return trait.begin(from);
}

template <typename T>
auto end(const T& from) {
    InputIteratorTrait<T> trait;

    return trait.end(from);
}

template <typename T, typename Predicate>
std::optional<size_t> position(const T& from, Predicate predicate) {
    auto begin_iterator = iterator::begin<T>(from);
    auto end_iterator = iterator::end<T>(from);
    auto iter = std::find_if(begin_iterator, end_iterator, predicate);

    if (iter == end_iterator) {
        return std::nullopt;
    }

    return std::distance(begin_iterator, iter);
}

template <typename T>
size_t length(const T& from) {
    return std::distance(iterator::begin(from), iterator::end(from));
}

}

}