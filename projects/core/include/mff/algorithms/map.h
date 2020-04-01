#pragma once

#include <algorithm>
#include <functional>
#include <iterator>
#include <boost/iterator/transform_iterator.hpp>

namespace mff {

namespace detail {
template <typename Function, typename Collection>
struct map_to {
    Function f;
    const Collection& c;

    template <typename T>
    operator T()&& {
        using std::begin; using std::end;
        return {boost::make_transform_iterator(begin(c), f), boost::make_transform_iterator(end(c), f)};
    }
};
}

template <typename Function, typename Collection>
detail::map_to<Function, Collection> map(Function f, const Collection& c)
{
    return { f, c };
}

}
