#pragma once
#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <range/v3/all.hpp>
#include <mff/leaf.h>
#include <mff/algorithms.h>
#include <mff/graphics/math.h>
#include <mff/graphics/memory.h>
#include <mff/graphics/vulkan/dispatcher.h>
#include <mff/graphics/vulkan/instance.h>
#include <mff/graphics/vulkan/swapchain.h>
#include <mff/graphics/vulkan/vulkan.h>
#include <mff/graphics/window.h>
#include <mff/graphics/vulkan/command_buffer/builders/unsafe.h>
#include <mff/graphics/vulkan/framebuffer/framebuffer.h>
#include <mff/graphics/vulkan/pipeline/raster.h>
#include <mff/graphics/vulkan/pipeline/blend.h>

#include "./third_party/earcut.hpp"
#include "./utils/logger.h"




////////////////////////
/// Vulkan utilities ///
////////////////////////

