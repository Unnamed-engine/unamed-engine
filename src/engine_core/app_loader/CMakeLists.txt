# HushCore

add_library(HushAppLoader OBJECT
        src/IApplication.cpp
        src/ApplicationLoader.cpp
)

add_library(Hush::AppLoader ALIAS HushAppLoader)

target_include_directories(HushAppLoader PUBLIC src)
target_include_directories(HushAppLoader PRIVATE internal)

target_link_libraries(HushAppLoader PUBLIC HushUtils HushLog Hush::Core)

set_all_warnings(HushAppLoader)