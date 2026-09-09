#include "lua/LuaSource.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace cinder::lua {

std::string readSource(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot read " + path.string());

    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

std::int64_t modifiedMillis(const std::filesystem::path& path, std::int64_t fallback) {
    std::error_code error;
    const auto time = std::filesystem::last_write_time(path, error);
    if (error) return fallback;

    return std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch()).count();
}

}
