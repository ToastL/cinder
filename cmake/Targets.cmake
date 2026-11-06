file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
    src/components/*.cpp
    src/core/*.cpp
    src/game/*.cpp
    src/gfx/*.cpp
    src/physics/*.cpp
    src/platform/*.cpp
    src/reflect/*.cpp
    src/scene/*.cpp
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
target_link_libraries(engine PUBLIC glfw glm::glm stb freetype harfbuzz)
if(CINDER_BACKEND STREQUAL "vk")
    target_link_libraries(engine PUBLIC volk vma_impl)
endif()
target_compile_definitions(engine PUBLIC
    GLFW_INCLUDE_NONE
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_ENABLE_EXPERIMENTAL)
target_compile_definitions(engine PUBLIC CINDER_BACKEND_${CINDER_BACKEND_UPPER})
set(CINDER_ABI "${PROJECT_VERSION}-${CINDER_BACKEND}-${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}-$<CONFIG>")

target_compile_definitions(engine PRIVATE
    CINDER_ABI="${CINDER_ABI}"
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

function(cinder_export_engine target)
    set_target_properties(${target} PROPERTIES ENABLE_EXPORTS ON)
    if(APPLE)
        target_link_options(${target} PRIVATE "LINKER:-force_load,$<TARGET_FILE:engine>")
    else()
        target_link_options(${target} PRIVATE
            "LINKER:--whole-archive" "$<TARGET_FILE:engine>" "LINKER:--no-whole-archive")
    endif()
endfunction()

function(cinder_game_module target)
    set(abi "${CINDER_ABI}")
    if(ARGC GREATER 1)
        set(abi "${ARGV1}")
    endif()
    target_include_directories(${target} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        $<TARGET_PROPERTY:glm::glm,INTERFACE_INCLUDE_DIRECTORIES>)
    target_compile_definitions(${target} PRIVATE
        $<TARGET_PROPERTY:engine,INTERFACE_COMPILE_DEFINITIONS>
        CINDER_MODULE_ABI="${abi}")
    set_target_properties(${target} PROPERTIES PREFIX "" CXX_VISIBILITY_PRESET hidden)
    if(APPLE)
        target_link_options(${target} PRIVATE "LINKER:-undefined,dynamic_lookup")
    endif()
endfunction()

separate_arguments(CINDER_MODULE_FLAGS UNIX_COMMAND "${CMAKE_CXX_FLAGS}")
list(APPEND CINDER_MODULE_FLAGS -std=c++20 -fPIC -fvisibility=hidden)
if(APPLE AND CMAKE_OSX_SYSROOT)
    list(APPEND CINDER_MODULE_FLAGS -isysroot ${CMAKE_OSX_SYSROOT})
endif()
if(APPLE AND CMAKE_OSX_DEPLOYMENT_TARGET)
    list(APPEND CINDER_MODULE_FLAGS -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET})
endif()
list(JOIN CINDER_MODULE_FLAGS "\", \"" CINDER_MODULE_FLAGS_JSON)

if(CMAKE_CONFIGURATION_TYPES)
    set(CINDER_CONFIGS ${CMAKE_CONFIGURATION_TYPES})
else()
    set(CINDER_CONFIGS ${CMAKE_BUILD_TYPE})
endif()
foreach(config IN LISTS CINDER_CONFIGS)
    string(TOUPPER "${config}" upper)
    separate_arguments(flags UNIX_COMMAND "${CMAKE_CXX_FLAGS_${upper}}")
    if(flags)
        list(JOIN flags "\", \"" flags)
        string(APPEND CINDER_MODULE_FLAGS_JSON "$<$<CONFIG:${config}>:\", \"${flags}>")
    endif()
endforeach()

file(GENERATE OUTPUT "$<TARGET_FILE_DIR:editor>/cinder-toolchain.json" CONTENT "{
	\"compiler\": \"${CMAKE_CXX_COMPILER}\",
	\"abi\": \"${CINDER_ABI}\",
	\"flags\": [\"${CINDER_MODULE_FLAGS_JSON}\"],
	\"includes\": [\"${CMAKE_CURRENT_SOURCE_DIR}/src\", \"$<JOIN:$<TARGET_PROPERTY:glm::glm,INTERFACE_INCLUDE_DIRECTORIES>,\"$<COMMA> \">\"],
	\"defines\": [\"$<JOIN:$<TARGET_PROPERTY:engine,INTERFACE_COMPILE_DEFINITIONS>,\"$<COMMA> \">\"]
}
")

add_executable(editor src/editor/main.cpp)
target_link_libraries(editor PRIVATE engine_dev)
cinder_export_engine(editor)

add_executable(player src/player/main.cpp)
target_link_libraries(player PRIVATE engine)
cinder_export_engine(player)

add_executable(packager src/packager/main.cpp)
target_link_libraries(packager PRIVATE engine_dev)
add_dependencies(editor player packager)


add_executable(ui_gallery tests/gallery/main.cpp)
target_link_libraries(ui_gallery PRIVATE engine)

