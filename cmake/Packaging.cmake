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

