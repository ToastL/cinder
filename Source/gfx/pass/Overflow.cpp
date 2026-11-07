#include "gfx/pass/Overflow.hpp"

#include "platform/Log.hpp"

namespace cinder::gfx::pass {

bool warnOverflow(bool alreadyWarned, std::string_view what, std::uint32_t limit, std::string_view unit) {
    if (!alreadyWarned) {
        cinder::platform::logError("[gfx] %.*s is full (%u%s%.*s); dropping draws\n", static_cast<int>(what.size()),
                                   what.data(), limit, unit.empty() ? "" : " ", static_cast<int>(unit.size()),
                                   unit.data());
    }
    return true;
}

}
