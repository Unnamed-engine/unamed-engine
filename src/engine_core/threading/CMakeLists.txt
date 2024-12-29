# Threading support subproject

add_library(HushThreading OBJECT
        src/Executor.cpp
)

target_include_directories(HushThreading PUBLIC src)

target_link_libraries(HushThreading PUBLIC HushUtils HushLog)

if (linux)
    target_link_libraries(HushThreading PUBLIC pthread)
endif()

set_all_warnings(HushThreading)