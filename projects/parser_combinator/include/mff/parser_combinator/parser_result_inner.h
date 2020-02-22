#pragma once

namespace mff::parser_combinator {

class parser_result_inner_tag {};

template <typename Input, typename Output>
class parser_result_inner : public parser_result_inner_tag {
public:
    using input_type = Input;
    using output_type = Output;

    parser_result_inner(input_type input, output_type output)
        : next_input(input), output(output) {
    }

    input_type next_input;
    output_type output;


    bool operator==(const parser_result_inner<Input, Output>& rhs) const {
        return next_input == rhs.next_input && output == rhs.output;
    }

    bool operator!=(const parser_result_inner<Input, Output>& rhs) const {
        return !operator==(rhs);
    }
};

}