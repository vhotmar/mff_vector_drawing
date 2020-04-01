#pragma once

#include <mff/parser_combinator/error/error_kind.h>

namespace mff::parser_combinator::error {

template <typename Input>
class DefaultError {
public:
    DefaultError(Input i, ErrorKind k)
        : input(i), kind(k) {
    };

    Input input;
    ErrorKind kind;

    bool operator==(const DefaultError<Input>& rhs) const {
        return input == rhs.input && kind == rhs.kind;
    }

    bool operator!=(const DefaultError<Input>& rhs) const {
        return !operator==(rhs);
    }
};

}