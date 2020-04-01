#pragma once

#include <mff/parser_combinator/error/error_kind.h>

namespace mff::parser_combinator::error {

template <typename Error, typename Input>
class ParserErrorTraits {
public:
};

template <typename Input>
class ParserErrorTraits<Input, DefaultError<Input>>

{
public:

DefaultError <Input> from_char(const Input& input, char c) {
    return from_error_kind(input, ErrorKind::Char);
}

DefaultError <Input> or_error(const DefaultError <Input>& from, const DefaultError <Input>& other) {
    return other;
}

DefaultError <Input> from_error_kind(const Input& input, ErrorKind kind) {
    return DefaultError(input, kind);
}

DefaultError <Input> append(const Input& input, ErrorKind kind, const DefaultError <Input>& other) {
    return other;
}

DefaultError <Input> add_context(const Input& input, const std::string& context, const DefaultError <Input>& other) {
    return other;
}
};


template <typename Input, typename Error>
Error or_error(const Error& from, const Error& other) {
    ParserErrorTraits <Input, Error> pem;

    return pem.or_error(from, other);
}

template <typename Input, typename Error>
Error make_error(const Input& input, ErrorKind kind) {
    ParserErrorTraits <Input, Error> pem;

    return pem.from_error_kind(input, kind);
}

template <typename Input, typename Error>
Error from_char(const Input& input, char c) {
    ParserErrorTraits <Input, Error> pem;

    return pem.from_char(input, c);
}

template <typename Input, typename Error>
Error append_error(const Input& input, ErrorKind kind, const Error& other) {
    ParserErrorTraits <Input, Error> pem;

    return pem.append(input, kind, other);
}

}