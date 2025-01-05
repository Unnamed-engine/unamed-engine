/*! \file ImageTexture.cpp
    \author Kyn21kx
    \date 2025-01-03
    \brief 
*/

#include "ImageTexture.hpp"

//TODO: To be replaced with a faster image loading library like SAIL
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include "Assertions.hpp"

Hush::ImageTexture::ImageTexture(const std::filesystem::path& filePath)
{
    HUSH_ASSERT(std::filesystem::exists(filePath), "Image texture path is not valid!");
    int32_t componentCount;
    void* rawData = stbi_load(filePath.string().c_str(), &this->m_width, &this->m_height, &componentCount, 0);
    this->m_data = static_cast<std::byte*>(rawData);
    (void)componentCount;
}

Hush::ImageTexture::ImageTexture(const std::byte* data, size_t size)
{
    HUSH_ASSERT(data != nullptr, "Invalid texture data (nullptr)!");
    int32_t componentCount;
    void* rawData = stbi_loadf_from_memory(reinterpret_cast<const uint8_t*>(data), static_cast<int32_t>(size), &this->m_width, &this->m_height, &componentCount, 0);
    this->m_data = static_cast<std::byte*>(rawData);
    (void)componentCount;
}

Hush::ImageTexture::~ImageTexture()
{
    stbi_image_free(this->m_data);
}

const int32_t& Hush::ImageTexture::GetWidth() const
{
    return this->m_width;
}

const int32_t& Hush::ImageTexture::GetHeight()
{
    return this->m_height;
}

const std::byte* Hush::ImageTexture::GetImageData() const
{
    return this->m_data;
}
