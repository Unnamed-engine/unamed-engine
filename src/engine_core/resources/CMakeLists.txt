# Resource classes and resource manager-things.


add_library(HushResource OBJECT
        src/filesystem/CFileSystem/CFileSystem.cpp
        src/filesystem/CFileSystem/CFile.cpp
        src/VirtualFilesystem.cpp
        src/IFile.cpp
)

target_include_directories(HushResource PUBLIC src)

target_link_libraries(HushResource PUBLIC HushUtils HushLog)

set_all_warnings(HushResource)