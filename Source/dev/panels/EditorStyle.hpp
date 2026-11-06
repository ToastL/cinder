#pragma once

#include "ui/core/Style.hpp"

namespace cinder::ui { class Application; }

namespace cinder::dev::panels {

cinder::ui::Theme editorTheme();

bool primaryHeld(const cinder::ui::Application& app);

}
