#include "platform/Files.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace cinder::platform {

std::string readTextFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot read " + path.string());

    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

void writeTextFile(const std::filesystem::path& path, std::string_view text, bool verifyCompletion) {
    std::ofstream out(path, std::ios::binary);
    if (!out) throw std::runtime_error("Cannot write " + path.string());
    out << text;
    if (verifyCompletion) {
        out.close();
        if (!out) throw std::runtime_error("Cannot write " + path.string());
    }
}

}
