#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace cinder::lua {

std::string readSource(const std::filesystem::path& path);
std::int64_t modifiedMillis(const std::filesystem::path& path, std::int64_t fallback);

}
