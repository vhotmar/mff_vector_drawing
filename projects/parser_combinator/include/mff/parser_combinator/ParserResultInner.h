#pragma once

namespace mff::parser_combinator {

class parser_result_inner_tag {};

template <typename Input, typename Output>
class ParserResultInner : public parser_result_inner_tag {
public:
    using input_type = Input;
    using output_type = Output;

    ParserResultInner(input_type input, output_type&& output)
        : next_input(input), output(std::forward<output_type>(output)) {
    }

    input_type next_input;
    output_type output;


    bool operator==(const ParserResultInner<Input, Output>& rhs) const {
        return next_input == rhs.next_input && output == rhs.output;
    }

    bool operator!=(const ParserResultInner<Input, Output>& rhs) const {
        return !operator==(rhs);
    }
};

}