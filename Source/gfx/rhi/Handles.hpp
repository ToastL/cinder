#pragma once

#include <cstdint>

namespace cinder::gfx::rhi {

struct Commands {
    void* handle = nullptr;

    explicit operator bool() const { return handle != nullptr; }
    bool operator==(const Commands&) const = default;
};

struct Uploads {
    void* handle = nullptr;

    explicit operator bool() const { return handle != nullptr; }
    bool operator==(const Uploads&) const = default;
};

struct TextureBinding {
    std::uint64_t id = 0;

    explicit operator bool() const { return id != 0; }
    bool operator==(const TextureBinding&) const = default;
};

}
