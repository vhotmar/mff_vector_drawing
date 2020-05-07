#pragma once

#include "./parsers/branch.h"
#include "./parsers/bytes.h"
#include "./parsers/character.h"
#include "./parsers/combinator.h"
#include "./parsers/multi.h"
#include "./parsers/sequence.h"

namespace mff::parser_combinator::parsers {

template <typename Input, typename Error = error::DefaultError<Input>>
struct Parsers {
    static constexpr inline alt_fn<Input, Error> alt;
    static constexpr inline between_fn<Input, Error> between;
    static constexpr inline delimited_fn<Input, Error> delimited;
    static constexpr inline pair_fn<Input, Error> pair;
    static constexpr inline preceded_fn<Input, Error> preceded;
    static constexpr inline separated_pair_fn<Input, Error> separated_pair;
    static constexpr inline terminated_fn<Input, Error> terminated;
    static constexpr inline tuple_fn<Input, Error> tuple;
    static constexpr inline many0_fn<Input, Error> many0;
    static constexpr inline many1_fn<Input, Error> many1;
    static constexpr inline separated_list_fn<Input, Error> separated_list;
    static constexpr inline separated_nonempty_list_fn<Input, Error> separated_nonempty_list;
    static constexpr inline parsers::combinator::ignore_fn<Input, Error> ignore;
    static constexpr inline parsers::combinator::constant_fn<Input, Error> constant;
    static constexpr inline parsers::combinator::map_fn<Input, Error> map;
    static constexpr inline parsers::combinator::opt_fn<Input, Error> opt;
    static constexpr inline parsers::combinator::value_fn<Input, Error> value;
    static constexpr inline parsers::combinator::recognize_fn<Input, Error> recognize;
    static constexpr inline parsers::combinator::verify_fn<Input, Error> verify;

    struct complete {
        static constexpr inline parsers::complete::alpha0_fn<Input, Error> alpha0;
        static constexpr inline parsers::complete::alpha1_fn<Input, Error> alpha1;
        static constexpr inline parsers::complete::char_p_fn<Input, Error> char_p;
        static constexpr inline parsers::complete::digit0_fn<Input, Error> digit0;
        static constexpr inline parsers::complete::digit1_fn<Input, Error> digit1;
        static constexpr inline parsers::complete::is_a_fn<Input, Error> is_a;
        static constexpr inline parsers::complete::is_not_fn<Input, Error> is_not;
        static constexpr inline parsers::complete::tag_fn<Input, Error> tag;
        static constexpr inline parsers::complete::take_while_fn<Input, Error> take_while;
        static constexpr inline parsers::complete::take_while1_fn<Input, Error> take_while1;
        static constexpr inline parsers::complete::take_while_m_n_fn<Input, Error> take_while_m_n;
        static constexpr inline parsers::complete::take_fn<Input, Error> take;
    };

    struct streaming {
        static constexpr inline parsers::streaming::is_a_fn<Input, Error> is_a;
        static constexpr inline parsers::streaming::tag_fn<Input, Error> tag;
    };
};

}
