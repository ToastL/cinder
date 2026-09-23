find_program(DXC NAMES dxc HINTS "$ENV{VULKAN_SDK}/bin")

if(NOT DXC)
    message(FATAL_ERROR
        "dxc not found. It compiles engine/shaders/*.hlsl to SPIR-V at build time.\n"
        "  Install the LunarG Vulkan SDK, or put dxc on PATH.")
endif()

function(cinder_compile_shaders target)
    file(GLOB SHADER_SOURCES ${CINDER_ENGINE_DIR}/shaders/*.hlsl)

    set(SPIRV_OUTPUTS)
    foreach(SHADER ${SHADER_SOURCES})
        get_filename_component(DIR ${SHADER} DIRECTORY)
        get_filename_component(NAME ${SHADER} NAME_WE)
        foreach(STAGE vs ps)
            string(TOUPPER ${STAGE} ENTRY)
            set(OUTPUT ${DIR}/${NAME}.${STAGE}.spv)
            add_custom_command(
                OUTPUT ${OUTPUT}
                COMMAND ${DXC} -spirv -fspv-target-env=vulkan1.2 -T ${STAGE}_6_0 -E ${ENTRY}Main
                        ${SHADER} -Fo ${OUTPUT}
                DEPENDS ${SHADER}
                COMMENT "dxc ${NAME}.${STAGE}"
                VERBATIM)
            list(APPEND SPIRV_OUTPUTS ${OUTPUT})
        endforeach()
    endforeach()

    add_custom_target(${target} DEPENDS ${SPIRV_OUTPUTS})
endfunction()
