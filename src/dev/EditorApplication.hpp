#pragma once

namespace cinder::core {
struct LaunchOptions;
struct ProjectConfig;
}

namespace cinder::dev {

void runEditor(const core::ProjectConfig& config, const core::LaunchOptions& options);

}
