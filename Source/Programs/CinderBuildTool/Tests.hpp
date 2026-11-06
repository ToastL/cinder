#pragma once

#include "CinderBuildTool/Build.hpp"

#include <string>
#include <vector>

namespace cbt {

int runTests(Builder& builder, const std::vector<std::string>& names);

}
