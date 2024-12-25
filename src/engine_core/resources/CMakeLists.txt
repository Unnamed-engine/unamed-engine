# Resource classes and resource manager-things.


add_library(HushResource OBJECT
        src/ResourceManager.cpp
)

target_include_directories(HushResource PUBLIC src)

target_link_libraries(HushResource PUBLIC HushUtils HushLog HushFilesystem)

set_all_warnings(HushResource)