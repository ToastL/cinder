foreach(var PLAYER ENGINE_DIR PROJECT_DIR OUT_DIR)
    if(NOT DEFINED ${var})
        message(FATAL_ERROR "PackageGame.cmake needs -D${var}=...")
    endif()
endforeach()

if(NOT EXISTS "${PROJECT_DIR}/project.lua")
    message(FATAL_ERROR "${PROJECT_DIR} is not a project: it has no project.lua")
endif()

file(REMOVE_RECURSE "${OUT_DIR}")
file(MAKE_DIRECTORY "${OUT_DIR}/engine/shaders" "${OUT_DIR}/engine/lua")

file(COPY "${PLAYER}" DESTINATION "${OUT_DIR}")

file(GLOB SPIRV "${ENGINE_DIR}/shaders/*.spv")
file(COPY ${SPIRV} DESTINATION "${OUT_DIR}/engine/shaders")

file(GLOB PRELUDE "${ENGINE_DIR}/lua/*.lua")
file(COPY ${PRELUDE} DESTINATION "${OUT_DIR}/engine/lua")

file(COPY "${PROJECT_DIR}/" DESTINATION "${OUT_DIR}/project")

message(STATUS "Packaged ${PROJECT_DIR} into ${OUT_DIR}")
