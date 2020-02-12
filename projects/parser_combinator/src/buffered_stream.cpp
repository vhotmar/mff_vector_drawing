#include <vector>

#include <mff/parser_combinator/buffered_stream.h>

namespace mff::parser_combinator {

buffered_stream::buffered_stream(std::istream& input): input_(input) {}

tl::expected<void, stream_error> buffered_stream::buffer_stream_to(int n) {
    int to_go = n - buffer_.size();

    while (to_go > 0) {
        char c = input_.get();

        if (input_.fail()) {
            return tl::unexpected(stream_error("Could not read from stream (probably end of stream)",
                                               stream_error::Kind::StreamReadFail));
        }

        buffer_.push_back(c);

        to_go--;
    }

    return {};
}

tl::expected<char, stream_error> buffered_stream::get() {
    auto result = buffer_stream_to(1);

    if (!result) return tl::unexpected(result.error());

    char c = buffer_.front();

    position_++;
    buffer_.pop_front();

    return c;
}

tl::expected<char, stream_error> buffered_stream::get(int n) {
    auto result = skip_n(n - 1);

    if (!result) return tl::unexpected(result.error());

    return get();
}

tl::expected<std::vector<char>, stream_error> buffered_stream::get_n(int n) {
    auto buffering_result = buffer_stream_to(n);

    if (!buffering_result) return tl::unexpected(buffering_result.error());

    std::vector<char> result(n);

    auto end_iterator = buffer_.begin() + n;
    std::move(buffer_.begin(), end_iterator, std::back_inserter(result));

    buffer_.erase(buffer_.begin(), end_iterator);
    position_ += n;

    return result;
}

tl::expected<char, stream_error> buffered_stream::peek() {
    return peek(0);
}


tl::expected<char, stream_error> buffered_stream::peek(int n) {
    auto buffering_result = buffer_stream_to(n + 1);

    if (!buffering_result) return tl::unexpected(buffering_result.error());

    return buffer_.at(n);
}

tl::expected<std::vector<char>, stream_error> buffered_stream::peek_n(int n) {
    auto buffering_result = buffer_stream_to(n);

    if (!buffering_result) return tl::unexpected(buffering_result.error());

    std::vector<char> result(buffer_.begin(), buffer_.begin() + n);

    return result;
}

int buffered_stream::position() {
    return position_;
}

bool buffered_stream::ended() {
    return !peek().has_value();
}

tl::expected<void, stream_error> buffered_stream::skip_n(int n) {
    auto buffering_result = buffer_stream_to(n);

    if (!buffering_result) return buffering_result;

    buffer_.erase(buffer_.begin(), buffer_.begin() + n);
    position_ += n;

    return {};
}

}
