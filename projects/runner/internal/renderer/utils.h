#pragma once

#include <string>
#include <vector>

namespace mff::utils {

template <typename TContainer>
std::vector<const char*> to_pointer_char_data(const TContainer& data) {
    std::vector<const char*> result;

    for (const auto& item: data)
        result.push_back(&item.front());

    return result;
}

}