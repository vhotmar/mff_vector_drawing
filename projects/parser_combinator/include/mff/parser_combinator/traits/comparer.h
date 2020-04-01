#pragma once

namespace mff::parser_combinator::traits {

enum class CompareResult {
    ok,
    incomplete,
    error,
};

template <typename A, typename B>
class CompareTrait {
    static_assert(sizeof(A) == -1, "You have to have specialization for comparer");
};

template <>
class CompareTrait<std::string_view, std::string_view> {
public:
    CompareResult compare(const std::string_view& a, const std::string_view& b) {
        auto len = std::min(a.size(), b.size());
        auto comparison = std::string_view::traits_type::compare(a.data(), b.data(), len);

        if (a.size() >= b.size()) {
            if (comparison == 0) return CompareResult::ok;

            return CompareResult::error;
        }

        if (comparison == 0) return CompareResult::incomplete;

        return CompareResult::error;
    }
};

template <>
class CompareTrait<std::vector<char>, std::vector<char>> {
public:
    CompareResult compare(const std::vector<char>& a, const std::vector<char>& b) {
        CompareTrait<std::string_view, std::string_view> com;

        return com.compare(std::string_view(a.data(), a.size()), std::string_view(b.data(), b.size()));
    }
};

template <>
class CompareTrait<std::string, std::string> {
public:
    CompareResult compare(const std::string& a, const std::string& b) {
        CompareTrait<std::string_view, std::string_view> com;

        return com.compare(a, b);
    }
};

template <typename A, typename B>
CompareResult compare(const A& a, const B& b) {
    CompareTrait<A, B> comp;

    return comp.compare(a, b);
}

}