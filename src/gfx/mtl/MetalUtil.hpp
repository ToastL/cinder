#pragma once

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include <stdexcept>
#include <string>

namespace cinder::gfx::mtl {

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
