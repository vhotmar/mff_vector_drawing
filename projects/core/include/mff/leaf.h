#pragma once

#include <boost/leaf/all.hpp>

#define LEAF_inner_CONCATENATE_DETAIL(x, y) x##y
#define LEAF_inner_CONCATENATE(x, y) LEAF_inner_CONCATENATE_DETAIL(x, y)
#define LEAF_inner_MAKE_UNIQUE(x) LEAF_inner_CONCATENATE(x, __COUNTER__)

#define LEAF_AUTO_TO_DETAIL(v, r, u)\
    static_assert(::boost::leaf::is_result_type<typename std::decay<decltype(r)>::type>::value, "LEAF_AUTO requires a result type");\
    auto && u = r;\
    if(!u)\
        return u.error();\
    v = std::move(u.value())

#define LEAF_AUTO_TO(v, r) LEAF_AUTO_TO_DETAIL(v, r, LEAF_inner_MAKE_UNIQUE(_r_))

#define LEAF_DEFAULT(v, d, r)\
    static_assert(::boost::leaf::is_result_type<typename std::decay<decltype(r)>::type>::value, "LEAF_AUTO requires a result type");\
    auto && v = !r ? d : std::move(r.value())

#define LEAF_CHECK_OPTIONAL(v, r)\
    if (!r)\
        return LEAF_NEW_ERROR();\
    auto && v = std::move(r.value());
