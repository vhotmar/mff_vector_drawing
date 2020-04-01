#pragma once

#include <algorithm>
#include <iterator>

namespace mff {

template <typename TItem, typename TContainer>
bool contains(const TContainer& list, const TItem& item) {
    using namespace std;

    return find(begin(list), end(list), item) != end(list);
}

template <typename TOperand, typename TContainer>
bool contains_if(const TContainer& list, const TOperand& item) {
    using namespace std;

    return find_if(begin(list), end(list), item) != end(list);
}

}
