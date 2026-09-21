#include "lua/LuaSource.hpp"

#include "platform/Files.hpp"


namespace cinder::lua {

std::string readSource(const std::filesystem::path& path) {
    return cinder::platform::readTextFile(path);
}

std::int64_t modifiedMillis(const std::filesystem::path& path, std::int64_t fallback) {
    std::error_code error;
    const auto time = std::filesystem::last_write_time(path, error);
    if (error) return fallback;

    return std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch()).count();
}

}
