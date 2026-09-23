find_program(DXC NAMES dxc HINTS "$ENV{VULKAN_SDK}/bin")

if(NOT DXC)
    message(FATAL_ERROR
        "dxc not found. It compiles engine/shaders/*.hlsl at build time.\n"
        "  Install the LunarG Vulkan SDK, or put dxc on PATH.")
endif()

if(CMAKE_HOST_APPLE)
    set(CINDER_SHADER_FORMATS_DEFAULT spirv msl)
elseif(CMAKE_HOST_WIN32)
    set(CINDER_SHADER_FORMATS_DEFAULT spirv dxil)
else()
    set(CINDER_SHADER_FORMATS_DEFAULT spirv)
endif()

set(CINDER_SHADER_FORMATS "${CINDER_SHADER_FORMATS_DEFAULT}" CACHE STRING
    "Shader formats to build: any of spirv, dxil, msl")

set(SHADER_FORMATS ${CINDER_SHADER_FORMATS})
foreach(FORMAT ${SHADER_FORMATS})
    if(NOT FORMAT MATCHES "^(spirv|dxil|msl)$")
        message(FATAL_ERROR "CINDER_SHADER_FORMATS: unknown format \"${FORMAT}\"")
    endif()
endforeach()

if(NOT "spirv" IN_LIST SHADER_FORMATS)
    list(INSERT SHADER_FORMATS 0 spirv)
endif()

set(CINDER_MSL_VERSION 20300)
set(CINDER_METAL_STD macos-metal2.3)

if("dxil" IN_LIST SHADER_FORMATS AND NOT CMAKE_HOST_WIN32)
    message(WARNING
        "DXIL is being built on a non-Windows host. dxc cannot sign DXIL without dxil.dll, "
        "so the output is unsigned and D3D12 will reject it outside developer mode. "
        "Treat it as a syntax check only.")
endif()

set(METAL_TOOLCHAIN FALSE)
if("msl" IN_LIST SHADER_FORMATS)
    find_program(SPIRV_CROSS NAMES spirv-cross HINTS "$ENV{VULKAN_SDK}/bin")
    if(NOT SPIRV_CROSS)
        message(FATAL_ERROR
            "spirv-cross not found, but \"msl\" is in CINDER_SHADER_FORMATS.\n"
            "  Install it (brew install spirv-cross), or drop msl from CINDER_SHADER_FORMATS.")
    endif()

    find_program(XCRUN NAMES xcrun)
    if(XCRUN)
        execute_process(COMMAND ${XCRUN} metal --version
                        RESULT_VARIABLE METAL_PROBE
                        OUTPUT_QUIET ERROR_QUIET)
        if(METAL_PROBE EQUAL 0)
            set(METAL_TOOLCHAIN TRUE)
        endif()
    endif()

    if(NOT METAL_TOOLCHAIN)
        message(STATUS
            "Metal Toolchain unavailable: emitting .metal source without compiling to .metallib. "
            "Install it with: xcodebuild -downloadComponent MetalToolchain")
    endif()
endif()

function(cinder_compile_shaders target)
    file(GLOB SHADER_SOURCES CONFIGURE_DEPENDS ${CINDER_ENGINE_DIR}/shaders/*.hlsl)

    set(OUT_DIR ${CMAKE_BINARY_DIR}/shaders)
    file(MAKE_DIRECTORY ${OUT_DIR})

    set(OUTPUTS)
    foreach(SHADER ${SHADER_SOURCES})
        get_filename_component(NAME ${SHADER} NAME_WE)
        set(AIR_FILES)

        foreach(STAGE vs ps)
            string(TOUPPER ${STAGE} ENTRY)

            set(SPV ${OUT_DIR}/${NAME}.${STAGE}.spv)
            add_custom_command(
                OUTPUT ${SPV}
                COMMAND ${DXC} -spirv -fspv-target-env=vulkan1.2 -T ${STAGE}_6_0 -E ${ENTRY}Main
                        ${SHADER} -Fo ${SPV}
                DEPENDS ${SHADER}
                COMMENT "dxc spirv ${NAME}.${STAGE}"
                VERBATIM)
            list(APPEND OUTPUTS ${SPV})

            if("dxil" IN_LIST SHADER_FORMATS)
                set(DXIL ${OUT_DIR}/${NAME}.${STAGE}.dxil)
                add_custom_command(
                    OUTPUT ${DXIL}
                    COMMAND ${DXC} -T ${STAGE}_6_0 -E ${ENTRY}Main ${SHADER} -Fo ${DXIL}
                    DEPENDS ${SHADER}
                    COMMENT "dxc dxil ${NAME}.${STAGE}"
                    VERBATIM)
                list(APPEND OUTPUTS ${DXIL})
            endif()

            if("msl" IN_LIST SHADER_FORMATS)
                if(STAGE STREQUAL vs)
                    set(MSL_STAGE vert)
                    set(MSL_COORDINATE_OPTIONS --flip-vert-y)
                else()
                    set(MSL_STAGE frag)
                    set(MSL_COORDINATE_OPTIONS)
                endif()

                set(METAL ${OUT_DIR}/${NAME}.${STAGE}.metal)
                add_custom_command(
                    OUTPUT ${METAL}
                    COMMAND ${SPIRV_CROSS} --msl --msl-version ${CINDER_MSL_VERSION}
                            --msl-decoration-binding
                            ${MSL_COORDINATE_OPTIONS}
                            --rename-entry-point ${ENTRY}Main ${ENTRY}Main ${MSL_STAGE}
                            ${SPV} --output ${METAL}
                    DEPENDS ${SPV}
                    COMMENT "spirv-cross msl ${NAME}.${STAGE}"
                    VERBATIM)
                list(APPEND OUTPUTS ${METAL})

                set(REFLECT ${OUT_DIR}/${NAME}.${STAGE}.json)
                add_custom_command(
                    OUTPUT ${REFLECT}
                    COMMAND ${SPIRV_CROSS} --reflect --output ${REFLECT} ${SPV}
                    DEPENDS ${SPV}
                    COMMENT "spirv-cross reflect ${NAME}.${STAGE}"
                    VERBATIM)
                list(APPEND OUTPUTS ${REFLECT})

                if(METAL_TOOLCHAIN)
                    set(AIR ${OUT_DIR}/${NAME}.${STAGE}.air)
                    add_custom_command(
                        OUTPUT ${AIR}
                        COMMAND ${XCRUN} metal -std=${CINDER_METAL_STD}
                                -fmodules-cache-path=${OUT_DIR}/metal-module-cache
                                -c ${METAL} -o ${AIR}
                        DEPENDS ${METAL}
                        COMMENT "metal ${NAME}.${STAGE}"
                        VERBATIM)
                    list(APPEND AIR_FILES ${AIR})
                endif()
            endif()
        endforeach()

        if(AIR_FILES)
            set(METALLIB ${OUT_DIR}/${NAME}.metallib)
            add_custom_command(
                OUTPUT ${METALLIB}
                COMMAND ${XCRUN} metallib ${AIR_FILES} -o ${METALLIB}
                DEPENDS ${AIR_FILES}
                COMMENT "metallib ${NAME}"
                VERBATIM)
            list(APPEND OUTPUTS ${METALLIB})
        endif()
    endforeach()

    add_custom_target(${target} DEPENDS ${OUTPUTS})
endfunction()
