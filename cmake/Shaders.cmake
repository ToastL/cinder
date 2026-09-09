find_program(GLSLANG_VALIDATOR NAMES glslangValidator glslang)

if(NOT GLSLANG_VALIDATOR)
    message(FATAL_ERROR
        "glslangValidator not found. It compiles assets/shaders/*.{vert,frag} to SPIR-V at build time.\n"
        "  brew install glslang")
endif()

function(cinder_compile_shaders target)
    file(GLOB SHADER_SOURCES
        ${CINDER_ASSETS_DIR}/shaders/*.vert
        ${CINDER_ASSETS_DIR}/shaders/*.frag)

    set(SPIRV_OUTPUTS)
    foreach(SHADER ${SHADER_SOURCES})
        set(OUTPUT ${SHADER}.spv)
        add_custom_command(
            OUTPUT ${OUTPUT}
            COMMAND ${GLSLANG_VALIDATOR} -V --target-env vulkan1.2 ${SHADER} -o ${OUTPUT}
            DEPENDS ${SHADER}
            COMMENT "glslang ${SHADER}"
            VERBATIM)
        list(APPEND SPIRV_OUTPUTS ${OUTPUT})
    endforeach()

    add_custom_target(${target} DEPENDS ${SPIRV_OUTPUTS})
endfunction()
