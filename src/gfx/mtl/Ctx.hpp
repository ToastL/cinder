#pragma once

#include "gfx/rhi/Format.hpp"

namespace cinder::platform { class Window; }

namespace cinder::gfx::rhi {

class Ctx {
public:
    explicit Ctx(cinder::platform::Window& window);
    ~Ctx();

    Ctx(const Ctx&) = delete;
    Ctx& operator=(const Ctx&) = delete;

    void* device() const { return device_; }
    void* queue() const { return queue_; }
    Format depthFormat() const { return Format::D32Sfloat; }

    void waitIdle() const;

private:
    void* device_ = nullptr;
    void* queue_ = nullptr;
};

}
