#pragma once

namespace mff::parser_combinator::traits {

enum class compare_result {
    ok,
    incomplete,
    error,
};

template <typename A, typename B>
class compare_trait {
    static_assert(sizeof(A) == -1, "You have to have specialization for comparer");
};

template <>
class compare_trait<std::string_view, std::string_view> {
public:
    compare_result compare(const std::string_view& a, const std::string_view& b) {
        auto len = std::min(a.size(), b.size());
        auto comparison = std::string_view::traits_type::compare(a.data(), b.data(), len);

        if (a.size() >= b.size()) {
            if (comparison == 0) return compare_result::ok;

            return compare_result::error;
        }

        if (comparison == 0) return compare_result::incomplete;

        return compare_result::error;
    }
};

template <>
class compare_trait<std::vector<char>, std::vector<char>> {
public:
    compare_result compare(const std::vector<char>& a, const std::vector<char>& b) {
        compare_trait<std::string_view, std::string_view> com;

        return com.compare(std::string_view(a.data(), a.size()), std::string_view(b.data(), b.size()));
    }
};

template <>
class compare_trait<std::string, std::string> {
public:
    compare_result compare(const std::string& a, const std::string& b) {
        compare_trait<std::string_view, std::string_view> com;

        return com.compare(a, b);
    }
};

template <typename A, typename B>
compare_result compare(const A& a, const B& b) {
    compare_trait<A, B> comp;

    return comp.compare(a, b);
}

}