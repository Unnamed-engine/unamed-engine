/*! \file VirtualFilesystem.hpp
    \author Alan Ramirez
    \date 2024-12-07
    \brief A Virtual filesystem implementation
*/

#pragma once
#include "FileSystem.hpp"
#include "Result.hpp"
#include <cstddef>
#include <span>
#include <string_view>
#include <vector>
#include <optional>

namespace Hush
{
    /// The VFS allows to manage paths as virtual locations (that might not refer to the real filesystem).
    /// For instance, to access an asset, we could use rs://myasset.txt, instead of referring to the physical location
    /// of the file. This, while introducing overhead, allows to manage more complex scenarios such as
    /// having the game files in a ZIP or a tar file. Or some other format.
    /// To use it, you must instantiate it
    class VirtualFilesystem final
    {
        struct MountPoint
        {
            std::string path;
            std::unique_ptr<IFileSystem> filesystem;
        };

      public:
        struct ResolvedPath
        {
            IFileSystem* filesystem = nullptr;
            std::string_view path;
        };
        enum class EError
        {
            None,
            FileDoesntExist,
            OperationNotSupported,
        };

        enum class EListOptions
        {
            None,
            Recursive,
        };

        //HACK: Instance for some quick testing throughout the app
        static inline VirtualFilesystem* s_instance;

        VirtualFilesystem();

        ~VirtualFilesystem();

        VirtualFilesystem(const VirtualFilesystem &) = delete;
        VirtualFilesystem &operator=(const VirtualFilesystem &) = delete;

        VirtualFilesystem(VirtualFilesystem &&rhs) noexcept;
        VirtualFilesystem &operator=(VirtualFilesystem &&rhs) noexcept;

        void Unmount(std::string_view virtualPath);

        std::vector<std::string_view> ListPath(std::string_view virtualPath, EListOptions options = EListOptions::None);

        Result<std::unique_ptr<IFile>, IFile::EError> OpenFile(std::string_view virtualPath, EFileOpenMode mode = EFileOpenMode::Read);

        Result<std::filesystem::path, IFile::EError> ToAbsolutePath(const std::string_view& virtualPath);

        template <class T, class... Args> void MountFileSystem(std::string_view path, Args &&...args)
        {
            MountFileSystemInternal(path, std::make_unique<T>(std::forward<Args>(args)...));
        }

      private:
        std::optional<ResolvedPath> ResolveFileSystem(std::string_view path);

        void MountFileSystemInternal(std::string_view path, std::unique_ptr<IFileSystem> resourceLoader);

        std::vector<MountPoint> m_mountedFileSystems;
    };
} // namespace Hush