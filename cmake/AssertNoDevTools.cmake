find_program(NM_EXECUTABLE nm)

if(NOT NM_EXECUTABLE)
    message(STATUS "nm not found; cannot verify ${EXE}")
    return()
endif()

execute_process(
    COMMAND ${NM_EXECUTABLE} -a "${EXE}"
    OUTPUT_VARIABLE SYMBOLS
    RESULT_VARIABLE STATUS
    ERROR_QUIET)

if(NOT STATUS EQUAL 0)
    message(FATAL_ERROR "nm failed on ${EXE}")
endif()

string(TOLOWER "${SYMBOLS}" LOWERED)

if(LOWERED MATCHES "imgui")
    message(FATAL_ERROR
        "${EXE} contains ImGui symbols. The engine/engine_dev split is broken: "
        "dev tools must not link into the shipping executable.")
endif()

if(LOWERED MATCHES "cinder3dev")
    message(FATAL_ERROR
        "${EXE} contains cinder::dev symbols. The engine/engine_dev split is broken.")
endif()
