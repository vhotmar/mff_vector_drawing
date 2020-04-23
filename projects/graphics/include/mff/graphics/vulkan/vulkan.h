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
