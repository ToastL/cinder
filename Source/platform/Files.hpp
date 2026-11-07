#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace cinder::platform {

std::string readTextFile(const std::filesystem::path& path);
std::vector<std::uint8_t> readBinaryFile(const std::filesystem::path& path);
void writeTextFile(const std::filesystem::path& path, std::string_view text);

}
