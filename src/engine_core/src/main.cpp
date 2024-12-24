#include "HushEngine.hpp"

#include <FileSystem.hpp>
#include <VirtualFilesystem.hpp>
#include <filesystem/CFileSystem/CFileSystem.hpp>

int main()
{
    Hush::VirtualFilesystem vfs;

    vfs.MountFileSystem<Hush::CFileSystem>("res://", ".");

    auto fileResult = vfs.OpenFile("res://vcpkg.json");

    if (!fileResult)
    {
        std::cerr << "Failed to read file data" << std::endl;
        return 1;
    }

    auto file = std::move(fileResult.assume_value());

    std::vector<std::byte> data(file->GetMetadata().size);

    if (const auto read = file->Read(data); !read)
    {
        std::cerr << "Failed to read file data" << std::endl;
        return 1;
    }

    // Print the data
    for (const auto byte : data)
    {
        std::cout << static_cast<char>(byte);
    }

    // Hush::HushEngine engine;
    //
    // engine.Run();
    //
    // engine.Quit();
    return 0;
}
