# VFS implementation


add_library(HushFilesystem OBJECT
        src/filesystem/CFileSystem/CFileSystem.cpp
        src/filesystem/CFileSystem/CFile.cpp
        src/VirtualFilesystem.cpp
        src/IFile.cpp
)

target_include_directories(HushFilesystem PUBLIC src)

target_link_libraries(HushFilesystem PUBLIC HushUtils HushLog)

set_all_warnings(HushFilesystem)

add_test_target(
        TARGET_NAME HushFilesystemTest
        ENGINE_TARGET HushFilesystem
        SRCS tests/filesystem/CFileSystem/CFile.test.cpp tests/filesystem/CFileSystem/CFileSystem.test.cpp
        HEADER_DIRS tests
)
