
add_library(HushCore OBJECT
        src/ISystem.cpp
        src/Scene.cpp
        src/Entity.cpp
        src/Query.cpp
        src/traits/EntityTraits.cpp
)

add_library(Hush::Core ALIAS HushCore)

target_include_directories(HushCore PUBLIC src)
target_include_directories(HushCore PRIVATE private)

target_link_libraries(HushCore PUBLIC flecs::flecs_static Hush::Utils Hush::Log)

set_all_warnings(HushCore)

add_test_target(
        TARGET_NAME HushCoreTest
        ENGINE_TARGET HushCore
        SRCS tests/Entity.test.cpp tests/Query.test.cpp
        HEADER_DIRS tests
)