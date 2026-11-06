#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace cinder::platform {

std::string readTextFile(const std::filesystem::path& path);
void writeTextFile(const std::filesystem::path& path, std::string_view text,
                   bool verifyCompletion = false);

}
