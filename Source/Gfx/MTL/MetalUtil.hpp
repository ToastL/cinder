#pragma once

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include <stdexcept>
#include <string>

extern "C" void* objc_autoreleasePoolPush(void);
extern "C" void objc_autoreleasePoolPop(void* token);

namespace cinder::Gfx::MTL {

constexpr NSUInteger PUSH_CONSTANT_SLOT = 0;
constexpr NSUInteger VERTEX_BUFFER_SLOT = 1;
constexpr NSUInteger TEXTURE_SLOT = 0;

class PoolScope {
public:
    PoolScope() : token_(objc_autoreleasePoolPush()) {}
    ~PoolScope() {
        if (token_ != nullptr) objc_autoreleasePoolPop(token_);
    }

    PoolScope(const PoolScope&) = delete;
    PoolScope& operator=(const PoolScope&) = delete;

    void* detach() {
        void* token = token_;
        token_ = nullptr;
        return token;
    }

private:
    void* token_ = nullptr;
};

inline void* retain(id object) { return (__bridge_retained void*)object; }

inline void release(void*& object) {
    if (object == nullptr) return;
    (void)CFBridgingRelease(object);
    object = nullptr;
}

template <typename T>
inline T bridge(void* object) {
    return (__bridge T)object;
}

inline std::runtime_error error(const char* what, NSError* detail = nil) {
    std::string message(what);
    if (detail != nil) {
        message += ": ";
        message += detail.localizedDescription.UTF8String;
    }
    return std::runtime_error(message);
}

}
