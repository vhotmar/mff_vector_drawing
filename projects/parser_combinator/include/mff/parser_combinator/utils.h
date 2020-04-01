#pragma once

#include <tuple>


namespace mff::parser_combinator::utils {


template <typename T>
struct expected_value;

template <typename T, typename E>
struct expected_value<tl::expected<T, E>> {
    using type = T;
};

template <typename T>
using expected_value_t = typename expected_value<T>::type;

template <typename T>
struct expected_error;

template <typename T, typename E>
struct expected_error<tl::expected<T, E>> {
    using type = E;
};

template <typename T>
using expected_error_t = typename expected_error<T>::type;


template <class T>
struct is_tuple_impl : std::false_type {};
template <class ...Args>
struct is_tuple_impl<std::tuple<Args...>> : std::true_type {};
template <class T>
using is_tuple = is_tuple_impl<std::decay_t<T>>;

template <typename T, typename Input>
struct parser_result {
private:
    static_assert(std::is_invocable_v<T, Input>, "T must be invokable with type Input");

public:
    using type = std::decay_t<std::invoke_result_t<T, Input>>;

private:
    static_assert(tl::detail::is_expected<type>::value, "T(Input) must be expected");

    using expected_value = std::decay_t<expected_value_t<type>>;

    using next_input = std::decay_t<decltype(std::declval<expected_value>().next_input)>;

    static_assert(std::is_same<next_input, Input>::value, "T(Input) first tuple value must be of type Input");
};

template <typename T, typename Input>
using parser_result_t = typename parser_result<T, Input>::type;

template <typename T, typename Input>
using parser_result_value_t = std::decay_t<expected_value_t<parser_result_t<T, Input>>>;

template <typename T, typename Input>
using parser_output_t = std::decay_t<decltype(std::declval<parser_result_value_t<T, Input>>().output)>;

template <typename T, typename Input>
using parser_error_t = expected_error_t<parser_result_t<T, Input>>;

}