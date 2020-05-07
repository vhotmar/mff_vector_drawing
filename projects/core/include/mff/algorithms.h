#pragma once

#include <algorithm>
#include <iterator>
#include <optional>

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


template <typename TOperand, typename TContainer>
bool any_of(const TContainer& list, const TOperand& item) {
    using namespace std;

    return any_of(begin(list), end(list), item);
}

template <typename TItem, typename TContainer>
auto find(const TContainer& list, const TItem& item) {
    using namespace std;

    auto b = begin(list);
    auto e = end(list);
    auto r = find(b, e, item);

    using iterator = decltype(r);

    if (r == e) return std::optional<iterator>();

    return std::optional<iterator>(r);
}


template <typename TOperand, typename TContainer>
auto find_if(const TContainer& list, const TOperand& op) {
    using namespace std;

    auto b = begin(list);
    auto e = end(list);
    auto r = find_if(b, e, op);

    using iterator = decltype(r);

    if (r == e) return std::optional<iterator>();

    return std::optional<iterator>(r);
}

template <typename TOperand, typename TContainer>
auto min_element(const TContainer& list, const TOperand& op) {
    using namespace std;

    auto b = begin(list);
    auto e = end(list);
    auto r = min_element(b, e, op);

    using iterator = decltype(r);

    if (r == e) return std::optional<iterator>();

    return std::optional<iterator>(r);
}


template <typename TKey, typename TContainer>
bool has(const TContainer& map_container, const TKey& key) {
    using namespace std;

    return map_container.find(key) != end(map_container);
}


}