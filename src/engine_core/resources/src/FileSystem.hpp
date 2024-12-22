/*! \file FileSystem.hpp
    \author Alan Ramirez
    \date 2024-11-17
    \brief Resource Source interface
*/

#pragma once
#include "Result.hpp"
#include <cstddef>
#include <functional>
#include <string_view>
#include <span>

namespace Hush
{
    /// A resource loader is an interface meant to be implemented by classes that
    /// load things from a specific source (i.e. a ZIP file). This means that they must implement a way
    /// to load resources from an underlying sources, both in an async and a sync way.
    /// Some resource sources do not support a
    /// Resource sources must live until all of their children are deleted.
    class IFileSystem
    {
      public:
        /// Errors when loading data.
        enum class EError
        {
            None,
            PathDoesntExist,
            NotSupported,
            CannotRead,
        };

        /// Callback to be called when data is loaded. Raw data is owned by the Resource Source, not by the caller.
        using AsyncCallback = std::function<void(Result<std::span<std::byte>, EError>)>;

        IFileSystem() = default;

        IFileSystem(const IFileSystem &) = delete;
        IFileSystem(IFileSystem &&) = default;
        IFileSystem &operator=(const IFileSystem &) = delete;
        IFileSystem &operator=(IFileSystem &&) = default;

        virtual ~IFileSystem() = default;

        /// Loads data from a specific path.
        /// Pointer is non owning, this means that it should be freed by the instance who created it.
        /// @param path Path of the resource to load
        /// @return A result with a pointer to the bytes of the resource.
        virtual Result<std::span<std::byte>, EError> LoadData(std::string_view path) = 0;

        /// Loads data from a specific path in an async way. The way it is loaded is defined by the resource loader.
        /// Some might use other threads, other might use something like Windows Overlapped I/O or epoll on Linux.
        /// Pointer is non owning, this means that it should be freed by the instance who created it.
        /// @param path Path of the resource to load.
        /// @param callback Callback to be called when the resource is loaded. The parameter is as result/
        virtual void LoadDataAsync(std::string_view path, AsyncCallback callback) = 0;

        /// Unloads a specific pointer.
        /// @param data Data to unload
        virtual void UnloadData(std::span<std::byte> data) = 0;
    };
} // namespace Hush
