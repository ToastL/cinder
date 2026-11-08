#pragma once

#include <functional>

namespace cinder::Dev {

struct PrefabActions {
    std::function<void(int)> save;
    std::function<void(int)> apply;
    std::function<void(int)> revert;
    std::function<void(int)> unpack;
    std::function<bool()> enabled;

    bool available() const { return enabled && enabled(); }
};

}
