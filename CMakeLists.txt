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

include(cmake/Backend.cmake)

include(cmake/Dependencies.cmake)
include(cmake/Shaders.cmake)

cinder_compile_shaders(shaders)

include(cmake/Targets.cmake)

include(cmake/Packaging.cmake)

enable_testing()
include(cmake/Testing.cmake)
