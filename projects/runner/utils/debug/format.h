#pragma once

#include <string>

namespace utils::debug::format {

template<typename TOStream>
struct DebugStruct {
public:
    DebugStruct(TOStream& stream, const std::string& name): stream_(stream) {
        stream_ << name;
    }

    template <typename TValue>
    DebugStruct& field(const std::string& key, const TValue& value) {
        if (fields_) {
            stream_ << ", ";
        } else {
            stream_ << " { ";
        }

        stream_ << key;
        stream_ << ": ";
        stream_ << value;

        fields_ = true;

        return *this;
    }

    TOStream& finish_non_exhaustive() {
        if (fields_) {
            stream_ << ", ..";
        } else {
            stream_ << " { ..";
        }

        stream_ << " }";

        return stream_;
    }

    TOStream& finish() {
        if (fields_) {
            stream_ << " }";
        }

        return stream_;
    }

    template <>
    DebugStruct& field(const std::string& key, const std::string& value) {
        if (fields_) {
            stream_ << ", ";
        } else {
            stream_ << " { ";
        }

        stream_ << key;
        stream_ << ": ";
        stream_ << "\"" << value << "\"";

        fields_ = true;

        return *this;
    }

private:
    TOStream& stream_;
    bool fields_ = false;
};

}
