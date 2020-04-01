#pragma once

#include <optional>

#include <mff/parser_combinator/error/error_traits.h>

namespace mff::parser_combinator::error {

namespace types {

struct Incomplete {
    bool operator==(const Incomplete& rhs) const {
        return true;
    }

    bool operator!=(const Incomplete& rhs) const {
        return !operator==(rhs);
    }
};

template <typename Error>
struct Err {
    Err(Error e)
        : error(e) {
    };

    Error error;

    bool operator==(const Err<Error>& rhs) const {
        return error == rhs.error;
    }

    bool operator!=(const Err<Error>& rhs) const {
        return !operator==(rhs);
    }
};

template <typename Error>
struct Failure {
    Failure(Error e)
        : error(e) {
    };

    Error error;

    bool operator==(const Failure<Error>& rhs) const {
        return error == rhs.error;
    }

    bool operator!=(const Failure<Error>& rhs) const {
        return !operator==(rhs);
    }
};

}

template <typename Error>
class ParserError {
public:
    using IncompleteType = types::Incomplete;
    using ErrorType = types::Err<Error>;
    using FailureType = types::Failure<Error>;
    using DataType = std::variant<
        IncompleteType,
        ErrorType,
        FailureType
    >;

    ParserError(DataType data)
        : data_(data) {
    };

    bool is_incomplete() const {
        return std::holds_alternative<IncompleteType>(data_);
    }

    bool is_error() const {
        return std::holds_alternative<ErrorType>(data_);
    }

    bool is_failure() const {
        return std::holds_alternative<FailureType>(data_);
    }

    std::optional<IncompleteType> incomplete() const {
        if (is_incomplete()) return std::get<IncompleteType>(data_);

        return std::nullopt;
    }

    std::optional<Error> error() const {
        if (is_error()) return std::get<ErrorType>(data_).error;

        return std::nullopt;
    }

    std::optional<Error> failure() const {
        if (is_failure()) return std::get<FailureType>(data_).error;

        return std::nullopt;
    }

    bool operator==(const ParserError<Error>& rhs) const {
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

    bool operator!=(const ParserError<Error>& rhs) const {
        return !operator==(rhs);
    }

private:
    DataType data_;
};

}