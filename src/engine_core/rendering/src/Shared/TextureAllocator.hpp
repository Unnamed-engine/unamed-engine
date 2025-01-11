/*! \file TextureAllocator.hpp
    \author Kyn21kx
    \date 2025-01-10
    \brief 
*/

#pragma once
#include "Logger.hpp"

//TODO: To be replaced with a faster image loading library like SAIL

void* CustomAlloc(size_t size) {
	Hush::LogInfo("Called custom malloc!");
	return malloc(size);
}

#define STBI_MALLOC(size) CustomAlloc(size);
#define STBI_REALLOC(p,newsz) realloc(p, newsz);
#define STBI_FREE(p) free(p)