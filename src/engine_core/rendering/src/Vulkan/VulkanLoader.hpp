#pragma once
#include <vector>
#include <string>
#include <optional>
#include <memory>
#include <filesystem>
#include "GPUMeshBuffers.hpp"

namespace fastgltf {
	class Mesh;
	class Asset;
}

namespace Hush {

	struct GeoSurface {
		uint32_t startIndex;
		uint32_t count;
	};

	struct MeshAsset {
		std::string name;

		std::vector<GeoSurface> surfaces;
		GPUMeshBuffers meshBuffers;
	};

	//forward declaration
	class VulkanRenderer;

	class VulkanLoader {

	public:
		static std::optional<std::vector<std::shared_ptr<MeshAsset>>> LoadGltfMeshes(VulkanRenderer* engine, std::filesystem::path filePath);


	private:
		static MeshAsset CreateMeshAssetFromGltfMesh(const fastgltf::Mesh& mesh, const fastgltf::Asset& accessors, std::vector<uint32_t>& indicesRef, std::vector<Vertex>& verticesRef);
	};

}