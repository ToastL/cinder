#pragma once

#include "Gfx/RHI/Handles.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace cinder::Gfx::MTL {

struct CommandState {
    void* encoder = nullptr;
    void* indexBuffer = nullptr;
    std::array<std::byte, 4096> pushConstants{};
    std::uint32_t pushConstantBytes = 0;
};

}

namespace cinder::Gfx::RHI {

inline cinder::Gfx::MTL::CommandState& unwrap(Commands commands) {
    return *static_cast<cinder::Gfx::MTL::CommandState*>(commands.handle);
}

inline void* unwrap(Uploads uploads) { return uploads.handle; }

inline Commands commands(cinder::Gfx::MTL::CommandState* state) { return Commands{state}; }
inline Uploads uploads(void* commandBuffer) { return Uploads{commandBuffer}; }

}
