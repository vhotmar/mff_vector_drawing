#pragma once

namespace mff::parser_combinator::error {

enum class ErrorKind {
    Tag,
    Char,
    IsA,
    Alpha,
    Digit,
    Alt,
    Many0,
    Many1,
    SeparatedList,
    TakeWhile1,
    TakeWhileMN,
    Eof,
    Verify,

    User,
};

}