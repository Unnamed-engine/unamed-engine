
add_library(HushCore OBJECT src/ISystem.cpp src/Scene.cpp)

add_library(Hush::Core ALIAS HushCore)

target_include_directories(HushCore PUBLIC src)

target_link_libraries(HushCore PUBLIC flecs::flecs_static Hush::Utils Hush::Log)

set_all_warnings(HushInput)