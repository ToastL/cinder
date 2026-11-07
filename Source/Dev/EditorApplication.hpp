#pragma once

namespace cinder::Core {
struct LaunchOptions;
struct ProjectConfig;
}

namespace cinder::Dev {

void runEditor(const Core::ProjectConfig& config, const Core::LaunchOptions& options);

}
