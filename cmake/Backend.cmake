if(APPLE)
    set(CINDER_BACKEND_DEFAULT mtl)
else()
    set(CINDER_BACKEND_DEFAULT vk)
endif()

set(CINDER_BACKEND ${CINDER_BACKEND_DEFAULT} CACHE STRING "Graphics backend: vk or mtl")
set_property(CACHE CINDER_BACKEND PROPERTY STRINGS vk mtl)

if(NOT CINDER_BACKEND MATCHES "^(vk|mtl)$")
    message(FATAL_ERROR "CINDER_BACKEND must be vk or mtl (got ${CINDER_BACKEND})")
endif()

configure_file(cmake/Backend.hpp.in
    ${CMAKE_BINARY_DIR}/generated/gfx/rhi/Backend.hpp @ONLY)

string(TOUPPER ${CINDER_BACKEND} CINDER_BACKEND_UPPER)

if(CINDER_BACKEND STREQUAL "mtl")
    if(NOT APPLE)
        message(FATAL_ERROR "CINDER_BACKEND=mtl is only available on Apple platforms")
    endif()
    enable_language(OBJC)
    enable_language(OBJCXX)
    set(CMAKE_OBJC_STANDARD 11)
    set(CMAKE_OBJCXX_STANDARD 20)
    set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
endif()

