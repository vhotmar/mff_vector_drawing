#pragma once

#include <deque>
#include <istream>

#include "mff/parser_combinator/stream.h"

namespace mff::parser_combinator {

/**
 * This is wrapper around system streams
 */
class buffered_stream: public stream {
public:
    explicit buffered_stream(std::istream& input);

    tl::expected<char, stream_error> get() override;
    tl::expected<char, stream_error> get(int n) override;
    tl::expected<std::vector<char>, stream_error> get_n(int n) override;

    tl::expected<char, stream_error> peek() override;
    tl::expected<char, stream_error> peek(int n) override;
    tl::expected<std::vector<char>, stream_error> peek_n(int n) override;

    int position() override;
    bool ended() override;

    tl::expected<void, stream_error> skip_n(int n) override;

private:
    std::istream& input_;
    std::deque<char> buffer_;
    int position_ = 0;

    tl::expected<void, stream_error> buffer_stream_to(int n);
};

}