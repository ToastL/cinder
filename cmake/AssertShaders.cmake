foreach(var ENGINE_DIR SHADER_DIR FORMATS)
    if(NOT DEFINED ${var})
        message(FATAL_ERROR "AssertShaders.cmake needs -D${var}=...")
    endif()
endforeach()

find_program(SPIRV_VAL NAMES spirv-val HINTS "$ENV{VULKAN_SDK}/bin")

file(GLOB SHADER_SOURCES "${ENGINE_DIR}/shaders/*.hlsl")

if(NOT SHADER_SOURCES)
    message(FATAL_ERROR "No .hlsl found in ${ENGINE_DIR}/shaders")
endif()

set(MISSING)
set(INVALID)
set(CHECKED 0)

foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(NAME ${SHADER} NAME_WE)

    foreach(STAGE vs ps)
        set(SPV "${SHADER_DIR}/${NAME}.${STAGE}.spv")
        if(NOT EXISTS "${SPV}")
            list(APPEND MISSING "${NAME}.${STAGE}.spv")
        else()
            math(EXPR CHECKED "${CHECKED} + 1")

            file(READ "${SPV}" MAGIC HEX LIMIT 4)
            if(NOT MAGIC STREQUAL "03022307")
                list(APPEND INVALID "${NAME}.${STAGE}.spv is not SPIR-V (magic ${MAGIC})")
            endif()

            if(SPIRV_VAL)
                execute_process(COMMAND ${SPIRV_VAL} "${SPV}"
                                RESULT_VARIABLE VAL_STATUS
                                ERROR_VARIABLE VAL_ERROR
                                OUTPUT_QUIET)
                if(NOT VAL_STATUS EQUAL 0)
                    string(STRIP "${VAL_ERROR}" VAL_ERROR)
                    list(APPEND INVALID "${NAME}.${STAGE}.spv failed spirv-val: ${VAL_ERROR}")
                endif()
            endif()
        endif()

        if("dxil" IN_LIST FORMATS AND NOT EXISTS "${SHADER_DIR}/${NAME}.${STAGE}.dxil")
            list(APPEND MISSING "${NAME}.${STAGE}.dxil")
        endif()

        if("msl" IN_LIST FORMATS AND NOT EXISTS "${SHADER_DIR}/${NAME}.${STAGE}.metal")
            list(APPEND MISSING "${NAME}.${STAGE}.metal")
        endif()
    endforeach()

    if("msl" IN_LIST FORMATS AND EXISTS "${SHADER_DIR}/${NAME}.vs.air")
        if(NOT EXISTS "${SHADER_DIR}/${NAME}.metallib")
            list(APPEND MISSING "${NAME}.metallib")
        endif()
    endif()
endforeach()

if(MISSING)
    string(REPLACE ";" "\n  " MISSING "${MISSING}")
    message(FATAL_ERROR "Shader artifacts missing from ${SHADER_DIR}:\n  ${MISSING}")
endif()

if(INVALID)
    string(REPLACE ";" "\n  " INVALID "${INVALID}")
    message(FATAL_ERROR "Invalid shader modules:\n  ${INVALID}")
endif()

if(NOT SPIRV_VAL)
    message(STATUS "spirv-val not found; checked magic numbers only")
endif()

message(STATUS "Checked ${CHECKED} SPIR-V modules in ${SHADER_DIR} (formats: ${FORMATS})")
