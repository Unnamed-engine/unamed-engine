# utils.cmake
# Alan Ramirez
# 2024-09-22
# CMake utils

# Set all warnings for the target
macro(set_all_warnings target)
    if (UNIX)
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    elseif (WIN32)
        target_compile_options(${target} PRIVATE /W4 /WX)
    endif ()
endmacro()

# Helper function to add a hush library. Hush libraries by default are OBJECT libraries.
# TARGET_NAME: Name of the library target
# LIB_TYPE: Type of the library (OBJECT, STATIC, SHARED)
# SRCS: Source files for the library
# PUBLIC_HEADER_DIRS: Public header directories for the library
# PRIVATE_HEADER_DIRS: Private header directories for the library
function (hush_add_library)
    cmake_parse_arguments(LIB "" "TARGET_NAME;LIB_TYPE" "SRCS;PUBLIC_HEADER_DIRS;PRIVATE_HEADER_DIRS" ${ARGN})
    add_library(${LIB_TARGET_NAME} ${LIB_LIB_TYPE} ${LIB_SRCS})
    target_include_directories(${LIB_TARGET_NAME} PUBLIC ${LIB_PUBLIC_HEADER_DIRS})
    target_include_directories(${LIB_TARGET_NAME} PRIVATE ${LIB_PRIVATE_HEADER_DIRS})
    set_all_warnings(${LIB_TARGET_NAME})

    if (${HUSH_ENABLE_LTO})
        set_property(TARGET ${LIB_TARGET_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
    target_link_options(${LIB_TARGET_NAME} PRIVATE ${HUSH_CPU_FLAGS})
endfunction()

# Helper function to add a hush executable
# TARGET_NAME: Name of the executable target
# SRCS: Source files for the executable
# HEADER_DIRS: Header directories for the executable
function (hush_add_executable)
    cmake_parse_arguments(EXE "" "TARGET_NAME" "SRCS;HEADER_DIRS" ${ARGN})
    add_executable(${EXE_TARGET_NAME} ${EXE_SRCS})
    target_include_directories(${EXE_TARGET_NAME} PRIVATE ${EXE_HEADER_DIRS})
    set_all_warnings(${EXE_TARGET_NAME})

    if (${HUSH_ENABLE_LTO})
        set_property(TARGET ${EXE_TARGET_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endfunction()

# Adds a test target to the project
# TARGET_NAME: Name of the test target
# ENGINE_TARGET: Engine target to link against
# SRCS: Source files for the test
# HEADER_DIR: Header directories for the test
function(add_test_target)
    if ( HUSH_ENABLE_TESTS )
        cmake_parse_arguments(TEST "" "TARGET_NAME;ENGINE_TARGET" "SRCS;HEADER_DIRS" ${ARGN})
        add_executable(${TEST_TARGET_NAME} ${TEST_SRCS})
        target_include_directories(${TEST_TARGET_NAME} PRIVATE ${TEST_HEADER_DIRS})
        target_link_libraries(${TEST_TARGET_NAME} PRIVATE ${TEST_ENGINE_TARGET} HushLog Catch2::Catch2WithMain)
        set_all_warnings(${TEST_TARGET_NAME})

        catch_discover_tests(${TEST_TARGET_NAME})

        if (${HUSH_ENABLE_LTO})
            set_property(TARGET ${TEST_TARGET_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
        endif()
    else()
        return()
    endif()
endfunction()

# add_hush_module is a helper function to add a module to the project
# A module is an object library meant to be linked with hush static library.
# MODULE_NAME: Name of the module
# SRCS: Source files for the module
# HEADER_DIRS: Header directories for the module
# DEFAULT_ENABLED: Default enabled state for the module
function(add_hush_module)
    cmake_parse_arguments(MODULE "" "MODULE_NAME" "SRCS;HEADER_DIRS" ${ARGN})

    set(ENABLE_HUSH_MODULE_${MODULE_MODULE_NAME} ${MODULE_DEFAULT_ENABLED} CACHE BOOL "Enable ${MODULE_MODULE_NAME} module")

    if (NOT ENABLE_HUSH_MODULE_${MODULE_MODULE_NAME})
        return()
    endif()

    add_library(${MODULE_MODULE_NAME} OBJECT ${MODULE_SRCS})
    target_include_directories(${MODULE_MODULE_NAME} PRIVATE ${MODULE_HEADER_DIRS})
    set_all_warnings(${MODULE_MODULE_NAME})

    target_link_libraries(${MODULE_MODULE_NAME} PRIVATE Hush::ModuleLibs)

    set(_HUSH_MODULES_LIST ${_HUSH_MODULES_LIST} ${MODULE_MODULE_NAME} PARENT_SCOPE)
endfunction()