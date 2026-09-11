#include "gfx/pass/Overflow.hpp"

#include "platform/Log.hpp"


namespace cinder::gfx::pass {

bool warnOverflow(bool alreadyWarned, std::string_view what, std::string_view limit) {
    if (!alreadyWarned) {
        cinder::platform::logError("[gfx] %.*s is full (%.*s); dropping draws\n",
                                   static_cast<int>(what.size()), what.data(),
                                   static_cast<int>(limit.size()), limit.data());
    }
    return true;
}

}
