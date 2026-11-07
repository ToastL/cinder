#pragma once

#include <filesystem>
#include <string>

namespace cbt {

bool isRuleFile(const std::filesystem::path& path);
std::string findTool(const std::string& name, const char* overrideVariable = nullptr);

}
