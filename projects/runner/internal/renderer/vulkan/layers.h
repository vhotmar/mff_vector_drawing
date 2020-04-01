#pragma once

#include <string>
#include <set>

namespace mff::internal::renderer::vulkan {

std::set<std::string> get_required_mff_layers();

}