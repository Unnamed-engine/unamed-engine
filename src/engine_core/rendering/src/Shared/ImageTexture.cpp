/*! \file ImageTexture.cpp
    \author Kyn21kx
    \date 2025-01-03
    \brief 
*/

#include "ImageTexture.hpp"
#include <fstream>
#include "Assertions.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

Hush::ImageTexture::ImageTexture(const std::filesystem::path& filePath)
{
    HUSH_ASSERT(std::filesystem::exists(filePath), "Image texture path is not valid!");
    int32_t componentCount;
    void* rawData = stbi_load(filePath.string().c_str(), &this->m_width, &this->m_height, &componentCount, 0);
    this->m_data = static_cast<std::byte*>(rawData);
    (void)componentCount;
}

Hush::ImageTexture::ImageTexture(std::byte* buffer, const std::byte* data, size_t size)
{
	HUSH_ASSERT(data != nullptr, "Invalid texture data (nullptr)!");
	int32_t componentCount;
    size_t newBufferSize;
	uint8_t* rawData = stbi_load_from_memory_pre_alloc(
        reinterpret_cast<uint8_t*>(buffer),
        &newBufferSize,
        reinterpret_cast<const uint8_t*>(data),
        static_cast<int32_t>(size),
        &this->m_width,
        &this->m_height,
        &componentCount,
        4
    );
	this->m_data = reinterpret_cast<std::byte*>(rawData);
	(void)componentCount;
    (void)buffer;
}

Hush::ImageTexture::ImageTexture(const std::byte* data, size_t size)
{
    HUSH_ASSERT(data != nullptr, "Invalid texture data (nullptr)!");
    int32_t componentCount;
    uint8_t* rawData = stbi_load_from_memory(reinterpret_cast<const uint8_t*>(data), static_cast<int32_t>(size), &this->m_width, &this->m_height, &componentCount, 4);
    this->m_data = reinterpret_cast<std::byte*>(rawData);	
    (void)componentCount;
}

Hush::ImageTexture::~ImageTexture()
{
    stbi_image_free(this->m_data);
}

const int32_t& Hush::ImageTexture::GetWidth() const noexcept
{
    return this->m_width;
}

const int32_t& Hush::ImageTexture::GetHeight() const noexcept
{
    return this->m_height;
}

const std::byte* Hush::ImageTexture::GetImageData() const
{
    return this->m_data;
}
