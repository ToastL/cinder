cmake_minimum_required(VERSION 3.24)
project(cinder VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

set(CINDER_ENGINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/engine")

if(APPLE)
    set(CINDER_BACKEND_DEFAULT mtl)
else()
    set(CINDER_BACKEND_DEFAULT vk)
endif()

set(CINDER_BACKEND ${CINDER_BACKEND_DEFAULT} CACHE STRING "Graphics backend: vk or mtl")
set_property(CACHE CINDER_BACKEND PROPERTY STRINGS vk mtl)

if(NOT IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/${CINDER_BACKEND}")
    message(FATAL_ERROR "CINDER_BACKEND=${CINDER_BACKEND}: src/gfx/${CINDER_BACKEND} does not exist")
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

include(cmake/Dependencies.cmake)
include(cmake/Shaders.cmake)

cinder_compile_shaders(shaders)

file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
    src/components/*.cpp
    src/core/*.cpp
    src/gfx/*.cpp
    src/gfx/asset/*.cpp
    src/gfx/pass/*.cpp
    src/gfx/rhi/*.cpp
    src/gfx/${CINDER_BACKEND}/*.cpp
    src/gfx/${CINDER_BACKEND}/*.mm
    src/lua/*.cpp
    src/physics/*.cpp
    src/platform/*.cpp
    src/reflect/*.cpp
    src/scene/*.cpp
    src/script/*.cpp
    src/serial/*.cpp
    src/text/*.cpp
    src/ui/*.cpp)

# The recursive gfx glob above picks up backend implementations as well as the
# backend-neutral renderer. Keep exactly the backend selected by CINDER_BACKEND.
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "/src/gfx/(vk|mtl)/")
file(GLOB_RECURSE CINDER_BACKEND_SOURCES CONFIGURE_DEPENDS
    src/gfx/${CINDER_BACKEND}/*.cpp
    src/gfx/${CINDER_BACKEND}/*.mm)
list(APPEND ENGINE_SOURCES ${CINDER_BACKEND_SOURCES})

add_library(engine STATIC ${ENGINE_SOURCES})
add_dependencies(engine shaders)

target_include_directories(engine PUBLIC src ${CMAKE_BINARY_DIR}/generated)
target_link_libraries(engine PUBLIC glfw glm::glm lua stb freetype harfbuzz)
if(CINDER_BACKEND STREQUAL "vk")
    target_link_libraries(engine PUBLIC volk vma_impl)
endif()
target_compile_definitions(engine PUBLIC
    GLFW_INCLUDE_NONE
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_ENABLE_EXPERIMENTAL)
target_compile_definitions(engine PUBLIC CINDER_BACKEND_${CINDER_BACKEND_UPPER})
target_compile_definitions(engine PRIVATE
    CINDER_ENGINE_DEFAULT="${CINDER_ENGINE_DIR}"
    CINDER_SHADERS_DEFAULT="${CMAKE_BINARY_DIR}/shaders"
    CINDER_VERSION="${PROJECT_VERSION}")
target_compile_options(engine PRIVATE -Wall -Wextra -Wno-unused-parameter)

if(CINDER_BACKEND STREQUAL "mtl")
    target_compile_options(engine PRIVATE $<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>)
    target_link_libraries(engine PUBLIC
        "-framework Metal" "-framework QuartzCore" "-framework Foundation"
        "-framework CoreGraphics" "-framework AppKit")
    target_compile_definitions(engine PUBLIC GLFW_EXPOSE_NATIVE_COCOA)
endif()

file(GLOB_RECURSE DEV_SOURCES CONFIGURE_DEPENDS src/dev/*.cpp)

add_library(engine_dev STATIC ${DEV_SOURCES})
target_link_libraries(engine_dev PUBLIC engine)
target_compile_options(engine_dev PRIVATE -Wall -Wextra -Wno-unused-parameter)

add_executable(editor src/editor/main.cpp)
target_link_libraries(editor PRIVATE engine_dev)

add_executable(player EXCLUDE_FROM_ALL src/player/main.cpp)
target_link_libraries(player PRIVATE engine)

add_executable(ui_gallery tests/gallery/main.cpp)
target_link_libraries(ui_gallery PRIVATE engine)

set(CINDER_PACKAGE_PROJECT "${CMAKE_CURRENT_SOURCE_DIR}/samples/sandbox2d" CACHE PATH
    "Project folder the package_game target stages next to the player")
file(GLOB CINDER_PACKAGE_MARKER "${CINDER_PACKAGE_PROJECT}/*.cinder")
if(CINDER_PACKAGE_MARKER)
    list(GET CINDER_PACKAGE_MARKER 0 CINDER_PACKAGE_MARKER)
    get_filename_component(CINDER_PACKAGE_NAME "${CINDER_PACKAGE_MARKER}" NAME_WLE)
else()
    get_filename_component(CINDER_PACKAGE_NAME "${CINDER_PACKAGE_PROJECT}" NAME)
endif()

if(CMAKE_HOST_APPLE)
    set(CINDER_HOST_PLATFORM macOS)
elseif(CMAKE_HOST_WIN32)
    set(CINDER_HOST_PLATFORM Windows)
else()
    set(CINDER_HOST_PLATFORM Linux)
endif()

add_custom_target(package_game
    COMMAND ${CMAKE_COMMAND}
        -DPLAYER=$<TARGET_FILE:player>
        -DHOST_PLATFORM=${CINDER_HOST_PLATFORM}
        -DENGINE_DIR=${CINDER_ENGINE_DIR}
        -DSHADER_DIR=${CMAKE_BINARY_DIR}/shaders
        -DPROJECT_DIR=${CINDER_PACKAGE_PROJECT}
        -DOUT_DIR=${CMAKE_BINARY_DIR}/dist/${CINDER_PACKAGE_NAME}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/PackageGame.cmake
    VERBATIM)
add_dependencies(package_game player shaders)

enable_testing()

add_test(NAME architecture_layers COMMAND ${CMAKE_COMMAND}
    -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/src
    -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/AssertLayers.cmake)
add_test(NAME architecture_checker COMMAND ${CMAKE_COMMAND}
    -DBINARY_DIR=${CMAKE_BINARY_DIR}
    -DCHECKER=${CMAKE_CURRENT_SOURCE_DIR}/cmake/AssertLayers.cmake
    -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/TestLayers.cmake)

add_test(NAME shaders_are_valid COMMAND ${CMAKE_COMMAND}
    -DENGINE_DIR=${CINDER_ENGINE_DIR}
    -DSHADER_DIR=${CMAKE_BINARY_DIR}/shaders
    "-DFORMATS=${SHADER_FORMATS}"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/AssertShaders.cmake)

file(GLOB TEST_SOURCES CONFIGURE_DEPENDS tests/*.cpp)
add_executable(tests ${TEST_SOURCES})
target_link_libraries(tests PRIVATE engine_dev doctest::doctest)
target_compile_definitions(tests PRIVATE CINDER_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME unit COMMAND tests)

add_executable(graphics_smoke EXCLUDE_FROM_ALL tests/graphics/smoke.cpp)
target_link_libraries(graphics_smoke PRIVATE engine_dev)
target_compile_definitions(graphics_smoke PRIVATE
    CINDER_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    CINDER_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}")

add_test(NAME build_player COMMAND ${CMAKE_COMMAND}
    --build ${CMAKE_BINARY_DIR} --target player --config $<CONFIG>)
set_tests_properties(build_player PROPERTIES FIXTURES_SETUP player)

add_test(NAME shipping_binary_is_clean COMMAND ${CMAKE_COMMAND}
    -DEXE=$<TARGET_FILE:player>
    -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/AssertNoDevTools.cmake)
set_tests_properties(shipping_binary_is_clean PROPERTIES FIXTURES_REQUIRED player)
