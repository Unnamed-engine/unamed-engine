# utils.cmake
# Alan Ramirez
# 2024-09-22
# CMake utils

macro(set_all_warnings target)
    if (UNIX)
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    elseif (WIN32)
        target_compile_options(${target} PRIVATE /W4 /WX)
    endif ()
endmacro()

# Adds a test target to the project
# TARGET_NAME: Name of the test target
# ENGINE_TARGET: Engine target to link against
# SRCS: Source files for the test
# HEADER_DIR: Header directories for the test
function(add_test_target)
    cmake_parse_arguments(TEST "" "TARGET_NAME;ENGINE_TARGET" "SRCS;HEADER_DIRS" ${ARGN})
    add_executable(${TEST_TARGET_NAME} ${TEST_SRCS})
    target_include_directories(${TEST_TARGET_NAME} PRIVATE ${TEST_HEADER_DIRS})
    target_link_libraries(${TEST_TARGET_NAME} PRIVATE ${TEST_ENGINE_TARGET} HushLog Catch2::Catch2WithMain)
    set_all_warnings(${TEST_TARGET_NAME})

    #catch_discover_tests(${TEST_TARGET_NAME})
endfunction()
