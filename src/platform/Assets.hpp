#pragma once

#include <filesystem>
#include <string_view>

namespace cinder::platform {

void locateExecutable(const char* argv0);
const std::filesystem::path& executableDir();

const std::filesystem::path& engineRoot();
std::filesystem::path enginePath(std::string_view relative);

void setProjectRoot(const std::filesystem::path& root);
const std::filesystem::path& projectRoot();
std::filesystem::path contentPath(std::string_view relative);
std::filesystem::path sourcePath(std::string_view relative);

}
