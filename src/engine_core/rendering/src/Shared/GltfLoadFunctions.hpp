#pragma once
#include <fastgltf/types.hpp>
#include <glm/mat4x4.hpp>
#include <vector>
#include "Result.hpp"
#include "MaterialPass.hpp"
#include "ImageTexture.hpp"
#include "Logger.hpp"
#include <magic_enum.hpp>

namespace Hush::GltfLoadFunctions {

	enum class EError {
		None = 0,
		InvalidMeshFile,
		FormatNotSupported
	};

	glm::mat4 GetNodeTransform(const fastgltf::Node& node);

	EMaterialPass GetMaterialPassFromFastGltfPass(fastgltf::AlphaMode pass);

	std::shared_ptr<ImageTexture> TextureFromImageDataSource(const fastgltf::Asset& asset, const fastgltf::Image& image);

	Hush::Result<const uint8_t*, EError> GetDataFromBufferSource(const fastgltf::Buffer& buffer);

	template<class BufferType>
	inline std::vector<BufferType> FindAttributeByName(const fastgltf::Primitive& primitive, const fastgltf::Asset& asset, const std::string_view& attributeName)
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