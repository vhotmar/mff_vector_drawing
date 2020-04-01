#pragma once

#include <tl/expected.hpp>

#define TRY(...)                                     \
    ({                                               \
        auto res = __VA_ARGS__;                      \
        if (!res) {                                  \
            return tl::make_unexpected(res.error()); \
        }                                            \
        std::move(res.value());                      \
    })

#define TRY_V(...)                                   \
    ({                                               \
        auto res = __VA_ARGS__;                      \
        if (!res) {                                  \
            return tl::make_unexpected(res.error()); \
        }                                            \
    })

#define VK_TRY(...)                                                \
    ({                                                             \
        auto res = __VA_ARGS__;                                    \
        if (res.result != vk::Result::eSuccess) {                  \
            return tl::make_unexpected(vk::to_string(res.result)); \
        }                                                          \
        std::move(res.value);                                      \
    })

#define VK_TRY_V(...)                                       \
    ({                                                      \
        auto res = __VA_ARGS__;                             \
        if (res != vk::Result::eSuccess) {                  \
            return tl::make_unexpected(vk::to_string(res)); \
        }                                                   \
    })