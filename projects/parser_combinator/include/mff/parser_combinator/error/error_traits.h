#pragma once

#include <mff/parser_combinator/error/error_kind.h>

namespace mff::parser_combinator::error {

template <typename Error, typename Input>
class parser_error_traits {
public:
};

template <typename Input>
class parser_error_traits<Input, default_error < Input>>

{
public:

default_error <Input> from_char(const Input& input, char c) {
    return from_error_kind(input, ErrorKind::Char);
}

default_error <Input> or_error(const default_error <Input>& from, const default_error <Input>& other) {
    return other;
}

default_error <Input> from_error_kind(const Input& input, ErrorKind kind) {
    return default_error(input, kind);
}

default_error <Input> append(const Input& input, ErrorKind kind, const default_error <Input>& other) {
    return other;
}

default_error <Input> add_context(const Input& input, const std::string& context, const default_error <Input>& other) {
    return other;
}
};


template <typename Input, typename Error>
Error or_error(const Error& from, const Error& other) {
    parser_error_traits <Input, Error> pem;

    return pem.or_error(from, other);
}

template <typename Input, typename Error>
Error make_error(const Input& input, ErrorKind kind) {
    parser_error_traits <Input, Error> pem;

    return pem.from_error_kind(input, kind);
}

template <typename Input, typename Error>
Error from_char(const Input& input, char c) {
    parser_error_traits <Input, Error> pem;

    return pem.from_char(input, c);
}

template <typename Input, typename Error>
Error append_error(const Input& input, ErrorKind kind, const Error& other) {
    parser_error_traits <Input, Error> pem;

    return pem.append(input, kind, other);
}

}