#pragma once

#include <cstddef>
#include <cstdint>
#include <initializer_list>

namespace cinder::physics {

struct ContactKey {
    int a = 0;
    int b = 0;
    int feature = 0;

    bool operator==(const ContactKey&) const = default;
};

struct ContactKeyHash {
    std::size_t operator()(const ContactKey& key) const noexcept {
        std::uint64_t hash = 1469598103934665603ull;
        for (const int part : {key.a, key.b, key.feature}) {
            hash ^= static_cast<std::uint32_t>(part);
            hash *= 1099511628211ull;
        }
        return static_cast<std::size_t>(hash);
    }
};

}
