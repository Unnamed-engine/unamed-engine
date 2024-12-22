/*! \file CFileSystem.hpp
    \author Alan Ramirez
    \date 2024-11-17
    \brief Resource Source interface that uses the filesystem (C API)
*/

#pragma once
#include "../FileSystem.hpp"
#include <cstdio>
#include <unordered_map>
#include <string>
#include <filesystem>

namespace Hush
{
    /// Loads data using C APIs.
    /// This, however, is not efficient as it loads the whole file into memory.
    /// For more complex scenarios, look at memory mapped sources.
    class CFileSystem final : public IFileSystem
    {
      public:
        CFileSystem(std::string_view root);

        CFileSystem(const CFileSystem &) = delete;

        CFileSystem(CFileSystem &&rhs) noexcept : mLoadedFiles(std::move(rhs.mLoadedFiles))
        {
        }

        CFileSystem &operator=(const CFileSystem &) = delete;

        CFileSystem &operator=(CFileSystem &&rhs) noexcept
        {
            mLoadedFiles = std::move(rhs.mLoadedFiles);
            return *this;
        }

        ~CFileSystem() override = default;

        /// Loads data from a specific source
        /// @param path Path of the resource to load.
        /// @return A result with the raw data.
        Result<std::span<std::byte>, EError> LoadData(std::string_view path) override;

        /// Loads data and calls a callback.
        /// This doesn't implement async, it only calls loadData.
        /// @param path Path of the resource to load.
        /// @param callback Function to be called when the resource is loaded
        void LoadDataAsync(std::string_view path, AsyncCallback callback) override;

        void UnloadData(std::span<std::byte> data) override;

      private:
        std::filesystem::path mRoot;
        std::unordered_map<std::byte *, std::unique_ptr<std::byte[]>> mLoadedFiles;
    };
} // namespace Hush