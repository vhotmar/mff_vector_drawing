#pragma once

#include <string>
#include <string_view>
#include <tuple>

namespace mff::parser_combinator::traits {

template <typename T, typename Token>
class find_token_trait {
    static_assert(sizeof(T) == -1, "You have to have specialization for find_token");
};

template <>
class find_token_trait<std::string_view, char> {
public:
    bool find_token(const std::string_view& input, const char& token) {
        return input.find(token) != std::string_view::npos;
    }
};

template <>
class find_token_trait<std::string, char> {
public:
    bool find_token(const std::string& input, const char& token) {
        return input.find(token) != std::string::npos;
    }
};

namespace input {

template <typename T, typename Token>
bool find_token(const T& input, const Token& token) {
    find_token_trait<T, Token> t;

    return t.find_token(input, token);
}

}

}
