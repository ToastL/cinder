#include "gfx/pass/Overflow.hpp"

#include <cstdio>

namespace cinder::gfx::pass {

bool warnOverflow(bool alreadyWarned, std::string_view what, std::string_view limit) {
    if (!alreadyWarned) {
        std::fprintf(stderr, "[gfx] %.*s is full (%.*s); dropping draws\n",
                     static_cast<int>(what.size()), what.data(),
                     static_cast<int>(limit.size()), limit.data());
    }
    return true;
}

}
