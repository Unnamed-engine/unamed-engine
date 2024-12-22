#include "HushEngine.hpp"

#include <FileSystem.hpp>
#include <VirtualFilesystem.hpp>
#include <filesystem/CFileSystem.hpp>

int main()
{
    Hush::VirtualFilesystem vfs;

    vfs.MountFileSystem<Hush::CFileSystem>("res://", ".");

    auto fileData = vfs.ReadFile("res://vcpkg.json");

    if (!fileData)
    {
        std::cerr << "Failed to read file data" << std::endl;
        return 1;
    }

    // Hush::HushEngine engine;
    //
    // engine.Run();
    //
    // engine.Quit();
    return 0;
}
