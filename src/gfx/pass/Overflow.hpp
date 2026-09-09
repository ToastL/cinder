#pragma once

#include <string_view>

namespace cinder::gfx::pass {

bool warnOverflow(bool alreadyWarned, std::string_view what, std::string_view limit);

}
