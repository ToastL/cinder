#pragma once

#include <cstdint>
#include <string_view>

namespace cinder::gfx::pass {

bool warnOverflow(bool alreadyWarned, std::string_view what, std::uint32_t limit, std::string_view unit);

}
