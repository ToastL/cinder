#pragma once

#include "Gfx/RHI/Format.hpp"

namespace cinder::Platform { class Window; }

namespace cinder::Gfx::RHI {

class Ctx {
public:
    explicit Ctx(cinder::Platform::Window& window);
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
