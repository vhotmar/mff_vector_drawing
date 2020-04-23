#pragma once

// we want to use dynamic dispatcher
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
// we do not want to cope with exceptions
#define VULKAN_HPP_NO_EXCEPTIONS
// and we want to ignore the asserts (probably horrible thing, but otherwise
// we can't really control flow of the program)
#define VULKAN_HPP_ASSERT ignore

// otherwise vulkan.hpp fails
#include <cassert>
#include <vulkan/vulkan.hpp>

#include "./leaf.h"

namespace boost::leaf {

template <>
struct is_e_type<vk::Result> : public std::true_type {};

}

namespace mff {

template <typename T>
boost::leaf::result<T> to_result(vk::ResultValue<T> vk_result) {
    if (vk_result.result != vk::Result::eSuccess) {
        return boost::leaf::new_error(vk_result.result);
    }

    return std::move(vk_result.value);
}

};
