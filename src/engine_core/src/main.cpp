#include "HushEngine.hpp"

#include <FileSystem.hpp>
#include <VirtualFilesystem.hpp>
#include <filesystem/CFileSystem/CFileSystem.hpp>

int main()
{
    Hush::HushEngine engine;
    
    engine.Run();
    
    engine.Quit();
    return 0;
}
