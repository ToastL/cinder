#pragma once

#include "gfx/rhi/Handles.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace cinder::gfx::mtl {

struct CommandState {
    void* encoder = nullptr;
    void* indexBuffer = nullptr;
    std::array<std::byte, 4096> pushConstants{};
    std::uint32_t pushConstantBytes = 0;
};

}

namespace cinder::gfx::rhi {

inline cinder::gfx::mtl::CommandState& unwrap(Commands commands) {
    return *static_cast<cinder::gfx::mtl::CommandState*>(commands.handle);
}

inline void* unwrap(Uploads uploads) { return uploads.handle; }

inline Commands commands(cinder::gfx::mtl::CommandState* state) { return Commands{state}; }
inline Uploads uploads(void* commandBuffer) { return Uploads{commandBuffer}; }

}
