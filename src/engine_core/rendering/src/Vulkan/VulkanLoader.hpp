#pragma once

//TODO: Move these undefs to the common.hpp file after dev merge
// remove stupid MSVC min/max macro definitions
#ifdef WIN32
	#undef min
	#undef max
#endif

#include <vector>
#include <string>
#include <optional>
#include <memory>
#include <filesystem>
#include "GPUMeshBuffers.hpp"
#include <fastgltf/types.hpp>
#include <Result.hpp>
#include "VkMaterialInstance.hpp"
#include "Shared/ImageTexture.hpp"
#include "VulkanMeshNode.hpp"


namespace Hush {

	struct GeoSurface {
		uint32_t startIndex;
		uint32_t count;
		std::shared_ptr<VkMaterialInstance> material;
	};

	struct MeshAsset {
		std::string name;

		std::vector<GeoSurface> surfaces;
		GPUMeshBuffers meshBuffers;
	};

	//forward declaration
	class VulkanRenderer;
	class VulkanAllocatedBuffer;

	class VulkanLoader {

	public:
		enum class EError {
			None = 0,
			FileNotFound,
			InvalidMeshFile,
			FormatNotSupported
		};

		static Result<std::vector<std::shared_ptr<VulkanMeshNode>>, EError> LoadGltfMeshes(VulkanRenderer* engine, std::filesystem::path filePath);

		static AllocatedImage LoadTexture(VulkanRenderer* engine, const ImageTexture& texture);

	private:

		static std::vector<AllocatedImage> LoadAllTextures(const fastgltf::Asset& asset, VulkanRenderer* engine);

		static VulkanMeshNode CreateMeshFromGltfMesh(const fastgltf::Mesh& mesh, const fastgltf::Asset& accessors, std::vector<uint32_t>& indicesRef, std::vector<Vertex>& verticesRef, VulkanRenderer* engine);

		static Result<const uint8_t*, EError> GetDataFromBufferSource(const fastgltf::Buffer& buffer);
		
		static std::shared_ptr<VkMaterialInstance> GenerateMaterial(size_t materialIdx, const fastgltf::Asset& asset, VulkanRenderer* engine, VulkanAllocatedBuffer* sceneMaterialBuffer, DescriptorAllocatorGrowable& allocatorPool, const std::vector<AllocatedImage>& loadedTextures);

		static std::shared_ptr<ImageTexture> GetTexturePropertiesFromMaterial(const fastgltf::Asset& asset, const fastgltf::Material& material);
		
		static std::optional<AllocatedImage> LoadedTextureFromMaterial(const fastgltf::Asset& asset, const fastgltf::Material& material, const std::vector<AllocatedImage>& loadedTextures);
		
		static std::shared_ptr<ImageTexture> TextureFromImageDataSource(const fastgltf::Asset& asset, const fastgltf::Image& image);

		template<class BufferType>
		static std::vector<BufferType> FindAttributeByName(const fastgltf::Primitive& primitive, const fastgltf::Asset& asset, const std::string_view& attributeName);
	
		static constexpr VkFilter ExtractFilter(const fastgltf::Filter& filter);
		static constexpr VkSamplerMipmapMode ExtractMipMapMode(const fastgltf::Filter& filter);
	};

	template<class BufferType>
	inline std::vector<BufferType> VulkanLoader::FindAttributeByName(const fastgltf::Primitive& primitive, const fastgltf::Asset& asset, const std::string_view& attributeName)
	{
		std::vector<BufferType> result;
		const fastgltf::Primitive::attribute_type* attribute = primitive.findAttribute(attributeName);
		if (attribute == primitive.attributes.end()) {
			return result;
		}
		const fastgltf::Accessor& foundAccessor = asset.accessors[attribute->second];
		const fastgltf::BufferView& accessorBufferView = asset.bufferViews[foundAccessor.bufferViewIndex.value()];
		const fastgltf::Buffer& buffer = asset.buffers[accessorBufferView.bufferIndex];
		result.reserve(foundAccessor.count);
		// Calculate the pointer to the start of the position data by using buffer, buffer view, and accessor offsets.
		Result<const uint8_t*, EError> bufferData = GetDataFromBufferSource(buffer);
		
		if (bufferData.has_error()) {
			LogFormat(ELogLevel::Warn, "{} Error! Could not read the data variant for the buffer", magic_enum::enum_name(bufferData.error()));
			return std::vector<BufferType>();
		}

		const BufferType* posData = reinterpret_cast<const BufferType*>(bufferData.value() + accessorBufferView.byteOffset + foundAccessor.byteOffset);

		for (size_t i = 0; i < foundAccessor.count; ++i) {
			result.push_back(posData[i]);
		}
		return result;
	}

}