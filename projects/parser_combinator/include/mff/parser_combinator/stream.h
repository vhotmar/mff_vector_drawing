#pragma once

#include <string>
#include <tl/expected.hpp>
#include <utility>

namespace mff::parser_combinator {

struct stream_error {
    enum class Kind {
        StreamReadFail,
        Generic
    };

    explicit stream_error(std::string text, Kind kind = Kind::Generic): kind(kind), text(std::move(text)) {};

    Kind kind;
    std::string text;
};

/**
 * Interface which we will expect for our parser combinators
 */
class stream {
public:
    virtual tl::expected<char, stream_error> get() = 0;
    virtual tl::expected<char, stream_error> get(int n) = 0;
    virtual tl::expected<std::vector<char>, stream_error> get_n(int n) = 0;

    virtual tl::expected<char, stream_error> peek() = 0;
    virtual tl::expected<char, stream_error> peek(int n) = 0;
    virtual tl::expected<std::vector<char>, stream_error> peek_n(int n) = 0;

    virtual int position() = 0;
    virtual bool ended() = 0;

    virtual tl::expected<void, stream_error> skip_n(int n) = 0;
};

}
