#include <mff/utils.h>

#include <fstream>

namespace mff {

std::vector<char> read_file(const std::string& path) {
    std::ifstream file(path.c_str(), std::ios::in | std::ios::binary | std::ios::ate);

    std::ifstream::pos_type file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> bytes(file_size);
    file.read(bytes.data(), file_size);
    file.close();

    return bytes;
}

}