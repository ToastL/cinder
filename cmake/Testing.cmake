add_test(NAME architecture_layers COMMAND ${CMAKE_COMMAND}
    -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/src
    -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/AssertLayers.cmake)
add_test(NAME architecture_checker COMMAND ${CMAKE_COMMAND}
    -DBINARY_DIR=${CMAKE_BINARY_DIR}
    -DCHECKER=${CMAKE_CURRENT_SOURCE_DIR}/cmake/AssertLayers.cmake
    -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/TestLayers.cmake)

add_test(NAME shaders_are_valid COMMAND ${CMAKE_COMMAND}
    -DENGINE_DIR=${CINDER_ENGINE_DIR}
    -DSHADER_DIR=${CMAKE_BINARY_DIR}/shaders
    "-DFORMATS=${SHADER_FORMATS}"
    -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/AssertShaders.cmake)

file(GLOB TEST_SOURCES CONFIGURE_DEPENDS tests/*.cpp)
add_executable(tests ${TEST_SOURCES})
target_link_libraries(tests PRIVATE engine_dev doctest::doctest)
target_compile_definitions(tests PRIVATE CINDER_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME unit COMMAND tests)

add_executable(graphics_smoke EXCLUDE_FROM_ALL tests/graphics/smoke.cpp)
target_link_libraries(graphics_smoke PRIVATE engine_dev)
target_compile_definitions(graphics_smoke PRIVATE
    CINDER_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    CINDER_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}")

add_test(NAME build_player COMMAND ${CMAKE_COMMAND}
    --build ${CMAKE_BINARY_DIR} --target player --config $<CONFIG>)
set_tests_properties(build_player PROPERTIES FIXTURES_SETUP player)

add_test(NAME shipping_binary_is_clean COMMAND ${CMAKE_COMMAND}
    -DEXE=$<TARGET_FILE:player>
    -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/AssertNoDevTools.cmake)
set_tests_properties(shipping_binary_is_clean PROPERTIES FIXTURES_REQUIRED player)
