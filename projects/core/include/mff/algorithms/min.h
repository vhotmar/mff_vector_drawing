#pragma once

#include <algorithm>
#include <iterator>
#include <optional>

namespace mff {

template <typename TOp, typename TContainer>
std::optional<decltype(TContainer::value_type)> min_element(const TContainer& list, const TOp& op) {
    using namespace std;

    auto it = min_element(begin(list), end(list), op);

    if (it == end(list)) return std::nullopt;

    return *it;
}

}
