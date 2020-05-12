#pragma once

#include <string>
#include <vector>

namespace mff {

/**
 * Used for variant
 */
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

/**
 * Read file from specified path
 * @param path
 * @return
 */
std::vector<char> read_file(const std::string& path);

}