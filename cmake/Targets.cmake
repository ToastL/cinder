file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
    src/components/*.cpp
    src/core/*.cpp
    src/gfx/*.cpp
    src/lua/*.cpp
    src/physics/*.cpp
    src/platform/*.cpp
    src/reflect/*.cpp
    src/scene/*.cpp
    src/script/*.cpp
    src/serial/*.cpp
    src/text/*.cpp
    src/ui/*.cpp)

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

target_compile_definitions(engine_dev PRIVATE
    CINDER_CMAKE_COMMAND="${CMAKE_COMMAND}"
    CINDER_BUILD_DIR="${CMAKE_BINARY_DIR}"
    CINDER_PLAYER_PATH="$<TARGET_FILE:player>"
    CINDER_PACKAGE_SCRIPT="${CMAKE_CURRENT_SOURCE_DIR}/cmake/PackageGame.cmake"
    CINDER_ENGINE_DIR="${CINDER_ENGINE_DIR}"
    CINDER_SHADER_DIR="${CMAKE_BINARY_DIR}/shaders")

add_executable(ui_gallery tests/gallery/main.cpp)
target_link_libraries(ui_gallery PRIVATE engine)

