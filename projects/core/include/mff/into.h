#pragma once

namespace mff {

template <typename To, typename From>
To into(const From& i) {
    if constexpr (std::is_convertible_v < From, To >) {
        return i;
    } else {
        static_assert(sizeof(From) == -1, "You have to have specialization for from");
        static_assert(sizeof(To) == -1, "You have to have specialization for from");
    }
}

}