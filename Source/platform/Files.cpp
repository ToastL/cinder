#include "platform/Files.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <system_error>

namespace cinder::platform {

std::string readTextFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot read " + path.string());

    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

void writeTextFile(const std::filesystem::path& path, std::string_view text) {
    std::filesystem::path staged = path;
    staged += ".tmp";
    {
        std::ofstream out(staged, std::ios::binary | std::ios::trunc);
        if (!out) throw std::runtime_error("Cannot write " + path.string());
        out << text;
        out.close();
        if (!out) {
            std::error_code ignored;
            std::filesystem::remove(staged, ignored);
            throw std::runtime_error("Cannot write " + path.string());
        }
    }
    std::error_code error;
    std::filesystem::rename(staged, path, error);
    if (error) {
        std::filesystem::remove(staged, error);
        throw std::runtime_error("Cannot replace " + path.string());
    }
}

}
