foreach(var PLAYER HOST_PLATFORM ENGINE_DIR SHADER_DIR PROJECT_DIR OUT_DIR)
    if(NOT DEFINED ${var})
        message(FATAL_ERROR "PackageGame.cmake needs -D${var}=...")
    endif()
endforeach()

file(GLOB MARKERS "${PROJECT_DIR}/*.cinder")
list(LENGTH MARKERS MARKER_COUNT)
if(NOT MARKER_COUNT EQUAL 1)
    message(FATAL_ERROR
        "${PROJECT_DIR} is not a project: it needs exactly one .cinder file, found ${MARKER_COUNT}")
endif()

file(READ "${MARKERS}" DESCRIPTOR)
string(JSON FILE_VERSION ERROR_VARIABLE JSON_ERROR GET "${DESCRIPTOR}" fileVersion)
if(JSON_ERROR)
    message(FATAL_ERROR "${MARKERS} is not a project descriptor: ${JSON_ERROR}")
endif()

set(PLATFORMS)
string(JSON PLATFORM_COUNT ERROR_VARIABLE NO_PLATFORMS LENGTH "${DESCRIPTOR}" targetPlatforms)
if(NOT NO_PLATFORMS AND PLATFORM_COUNT GREATER 0)
    set(TARGETED FALSE)
    math(EXPR LAST "${PLATFORM_COUNT} - 1")
    foreach(i RANGE ${LAST})
        string(JSON PLATFORM GET "${DESCRIPTOR}" targetPlatforms ${i})
        list(APPEND PLATFORMS ${PLATFORM})
        if(PLATFORM STREQUAL HOST_PLATFORM)
            set(TARGETED TRUE)
        endif()
    endforeach()
    if(NOT TARGETED)
        message(FATAL_ERROR "${MARKERS} does not list ${HOST_PLATFORM} in targetPlatforms")
    endif()
else()
    set(PLATFORMS macOS Linux Windows)
endif()

set(SHADER_PATTERNS)
foreach(PLATFORM ${PLATFORMS})
    if(PLATFORM STREQUAL "macOS")
        list(APPEND SHADER_PATTERNS "*.spv" "*.metallib")
    elseif(PLATFORM STREQUAL "iOS")
        list(APPEND SHADER_PATTERNS "*.metallib")
    elseif(PLATFORM STREQUAL "Linux" OR PLATFORM STREQUAL "Android")
        list(APPEND SHADER_PATTERNS "*.spv")
    elseif(PLATFORM STREQUAL "Windows")
        list(APPEND SHADER_PATTERNS "*.dxil")
    endif()
endforeach()
list(REMOVE_DUPLICATES SHADER_PATTERNS)

function(run_build_steps kind)
    string(JSON STEP_COUNT ERROR_VARIABLE NO_STEPS LENGTH "${DESCRIPTOR}" ${kind} ${HOST_PLATFORM})
    if(NO_STEPS OR STEP_COUNT EQUAL 0)
        return()
    endif()

    math(EXPR LAST "${STEP_COUNT} - 1")
    foreach(i RANGE ${LAST})
        string(JSON STEP GET "${DESCRIPTOR}" ${kind} ${HOST_PLATFORM} ${i})
        string(REPLACE "$(ProjectDir)" "${PROJECT_DIR}" STEP "${STEP}")
        string(REPLACE "$(EngineDir)" "${ENGINE_DIR}" STEP "${STEP}")
        string(REPLACE "$(StageDir)" "${OUT_DIR}" STEP "${STEP}")
        message(STATUS "${kind}: ${STEP}")

        if(HOST_PLATFORM STREQUAL "Windows")
            execute_process(COMMAND cmd /c "${STEP}" WORKING_DIRECTORY "${PROJECT_DIR}"
                            RESULT_VARIABLE RESULT)
        else()
            execute_process(COMMAND sh -c "${STEP}" WORKING_DIRECTORY "${PROJECT_DIR}"
                            RESULT_VARIABLE RESULT)
        endif()
        if(NOT RESULT EQUAL 0)
            message(FATAL_ERROR "${kind} failed with ${RESULT}: ${STEP}")
        endif()
    endforeach()
endfunction()

run_build_steps(preBuildSteps)

file(REMOVE_RECURSE "${OUT_DIR}")
file(MAKE_DIRECTORY "${OUT_DIR}/engine/shaders" "${OUT_DIR}/engine/lua" "${OUT_DIR}/engine/fonts"
    "${OUT_DIR}/project")

file(COPY "${PLAYER}" DESTINATION "${OUT_DIR}")

foreach(PATTERN ${SHADER_PATTERNS})
    file(GLOB STAGED "${SHADER_DIR}/${PATTERN}")
    if(NOT STAGED)
        message(WARNING
            "No ${PATTERN} in ${SHADER_DIR}: targetPlatforms asks for it, but the build did not "
            "produce it. Add the format to CINDER_SHADER_FORMATS.")
    endif()
    file(COPY ${STAGED} DESTINATION "${OUT_DIR}/engine/shaders")
endforeach()

file(GLOB PRELUDE "${ENGINE_DIR}/lua/*.lua")
file(COPY ${PRELUDE} DESTINATION "${OUT_DIR}/engine/lua")

file(GLOB FONTS "${ENGINE_DIR}/fonts/*.ttf" "${ENGINE_DIR}/fonts/*.txt")
file(COPY ${FONTS} DESTINATION "${OUT_DIR}/engine/fonts")

file(COPY ${MARKERS} DESTINATION "${OUT_DIR}/project")
foreach(folder Config Content Source)
    if(IS_DIRECTORY "${PROJECT_DIR}/${folder}")
        file(COPY "${PROJECT_DIR}/${folder}" DESTINATION "${OUT_DIR}/project")
    endif()
endforeach()

run_build_steps(postBuildSteps)

message(STATUS "Packaged ${PROJECT_DIR} into ${OUT_DIR}")
