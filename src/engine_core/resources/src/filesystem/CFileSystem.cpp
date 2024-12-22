/*! \file FsResourceSource.cpp
    \author Alan Ramirez
    \date 2024-11-17
    \brief Resource Source interface that uses the filesystem (C API)
*/
#include "CFileSystem.hpp"

#include <Logger.hpp>

Hush::CFileSystem::CFileSystem(std::string_view root) : mRoot(root)
{
}

Hush::Result<std::span<std::byte>, Hush::IFileSystem::EError> Hush::CFileSystem::LoadData(std::string_view path)
{
    auto resolvedPath = mRoot / path;

    auto currentPath = std::filesystem::current_path();
    Log(ELogLevel::Info, "Loading " + resolvedPath.string());

    if (!exists(resolvedPath))
    {
        return EError::PathDoesntExist;
    }

    auto stringPath = resolvedPath.generic_string();

    FILE *file{};
    if (const auto err = fopen_s(&file, stringPath.c_str(), "rb"); err != 0)
    {
        LogFormat(Hush::ELogLevel::Warn, "File could not be loaded, error: {}", err);
        return EError::PathDoesntExist;
    }

    fseek(file, 0, SEEK_END);
    const auto size = ftell(file);
    fseek(file, 0, SEEK_SET);

    auto data = std::make_unique<std::byte[]>(size);
    if (fread(data.get(), sizeof(std::byte), size, file) != size)
    {
        fclose(file);
        return EError::CannotRead;
    }

    fclose(file);

    auto *rawPtr = data.release();

    // A bit hacky if you ask me, but it works.
    this->mLoadedFiles.emplace(rawPtr, std::move(data));

    return std::span(rawPtr, size);
}

void Hush::CFileSystem::LoadDataAsync(std::string_view path, AsyncCallback callback)
{
    callback(LoadData(path));
}

void Hush::CFileSystem::UnloadData(std::span<std::byte> data)
{
    auto dataPtr = data.data();
    const auto file = this->mLoadedFiles.find(dataPtr);

    if (file == this->mLoadedFiles.end())
    {
        return;
    }

    // Just delete the file, the dtor will take care of it. :)
    this->mLoadedFiles.erase(file);
}