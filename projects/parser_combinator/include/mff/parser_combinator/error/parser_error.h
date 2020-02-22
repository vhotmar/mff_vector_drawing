#pragma once

#include <optional>

#include <mff/parser_combinator/error/error_traits.h>

namespace mff::parser_combinator::error {

namespace types {

struct incomplete {
    bool operator==(const incomplete& rhs) const {
        return true;
    }

    bool operator!=(const incomplete& rhs) const {
        return !operator==(rhs);
    }
};

template <typename Error>
struct err {
    err(Error e)
        : error(e) {
    };

    Error error;

    bool operator==(const err<Error>& rhs) const {
        return error == rhs.error;
    }

    bool operator!=(const err<Error>& rhs) const {
        return !operator==(rhs);
    }
};

template <typename Error>
struct failure {
    failure(Error e)
        : error(e) {
    };

    Error error;

    bool operator==(const failure<Error>& rhs) const {
        return error == rhs.error;
    }

    bool operator!=(const failure<Error>& rhs) const {
        return !operator==(rhs);
    }
};

}

template <typename Error>
class parser_error {
public:
    using incomplete_type = types::incomplete;
    using error_type = types::err<Error>;
    using failure_type = types::failure<Error>;
    using data_type = std::variant<
        incomplete_type,
        error_type,
        failure_type
    >;

    parser_error(data_type data)
        : data_(data) {
    };

    bool is_incomplete() const {
        return std::holds_alternative<incomplete_type>(data_);
    }

    bool is_error() const {
        return std::holds_alternative<error_type>(data_);
    }

    bool is_failure() const {
        return std::holds_alternative<failure_type>(data_);
    }

    std::optional<incomplete_type> incomplete() const {
        if (is_incomplete()) return std::get<incomplete_type>(data_);

        return std::nullopt;
    }

    std::optional<Error> error() const {
        if (is_error()) return std::get<error_type>(data_).error;

        return std::nullopt;
    }

    std::optional<Error> failure() const {
        if (is_failure()) return std::get<failure_type>(data_).error;

        return std::nullopt;
    }

    bool operator==(const parser_error<Error>& rhs) const {
        if (is_failure() && rhs.is_failure()) {
            return failure() == rhs.failure();
        }

        if (is_error() && rhs.is_error()) {
            return error() == rhs.error();
        }

        if (is_incomplete() && rhs.is_incomplete()) {
            return incomplete() == rhs.incomplete();
        }

        return false;
    }

    bool operator!=(const parser_error<Error>& rhs) const {
        return !operator==(rhs);
    }

private:
    data_type data_;
};

}