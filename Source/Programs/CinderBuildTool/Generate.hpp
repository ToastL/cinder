#pragma once

#include "CinderBuildTool/Build.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace cbt {

bool generateCompileCommands(Builder& builder, const std::vector<std::string>& targets);
bool generateXcode(Builder& builder, const std::vector<std::string>& targets, const std::filesystem::path& cbt);

}
