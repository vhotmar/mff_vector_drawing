#pragma once

#include <mff/parser_combinator/error/error_kind.h>

namespace mff::parser_combinator::error {

template <typename Input>
class default_error {
public:
    default_error(Input i, ErrorKind k)
        : input(i), kind(k) {
    };

    Input input;
    ErrorKind kind;

    bool operator==(const default_error<Input>& rhs) const {
        return input == rhs.input && kind == rhs.kind;
    }

    bool operator!=(const default_error<Input>& rhs) const {
        return !operator==(rhs);
    }
};

}