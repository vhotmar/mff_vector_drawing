#pragma once

#include <string>
#include <string_view>
#include <tuple>

namespace mff::parser_combinator::traits {

template <typename T>
class as_char_trait {
    static_assert(sizeof(T) == -1, "You have to have specialization for as_char");
};

template <>
class as_char_trait<char> {
public:
    char as_char(char c) {
        return c;
    }

    bool is_alpha(char c) {
        return isalpha(c);
    }

    bool is_alphanum(char c) {
        return isalnum(c);
    }

    bool is_dec_digit(char c) {
        return isdigit(c);
    }
};

namespace as_char {

template <typename T>
char as_char(const T& input) {
    as_char_trait<T> t;

    return t.as_char(input);
}

template <typename T>
bool is_alpha(const T& input) {
    as_char_trait<T> t;

    return t.is_alpha(input);
}

template <typename T>
bool is_alphanum(const T& input) {
    as_char_trait<T> t;

    return t.is_alphanum(input);
}

template <typename T>
bool is_dec_digit(const T& input) {
    as_char_trait<T> t;

    return t.is_dec_digit(input);
}

}

}
