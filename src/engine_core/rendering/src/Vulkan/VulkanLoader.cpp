#include "VulkanLoader.hpp"
#include <fastgltf/parser.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>
#include "VkTypes.hpp"

std::optional<std::vector<std::shared_ptr<Hush::MeshAsset>>> Hush::VulkanLoader::LoadGltfMeshes(VulkanRenderer* engine, std::filesystem::path filePath)
{
	fastgltf::GltfDataBuffer data;
	data.loadFromFile(filePath);

	constexpr fastgltf::Options loadingOptions = fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;

	fastgltf::Parser parser{};

	fastgltf::Expected<fastgltf::Asset> loadedAsset = parser.loadBinaryGLTF(&data, filePath, loadingOptions);

	HUSH_ASSERT(loadedAsset, "GLTF asset at {} not properly loaded, error: {}!", filePath.c_str(), fastgltf::to_underlying(loadedAsset.error()));
	
	std::vector<uint32_t> indices;
	std::vector<Vertex> vertices;
	for (const fastgltf::Mesh& mesh : loadedAsset->meshes)
	{
		CreateMeshAssetFromGltfMesh(mesh, loadedAsset.get(), indices, vertices);
	}

	std::vector<std::shared_ptr<MeshAsset>> meshes;
}

Hush::MeshAsset Hush::VulkanLoader::CreateMeshAssetFromGltfMesh(const fastgltf::Mesh& mesh, const fastgltf::Asset& asset, std::vector<uint32_t>& indicesRef, std::vector<Vertex>& verticesRef)
{
	//Clear out the vector buffers
	indicesRef.clear();
	verticesRef.clear();

	for (const fastgltf::Primitive& primitive : mesh.primitives)
	{
		size_t initialVertex = verticesRef.size();
		GeoSurface surfaceToAdd{};
		surfaceToAdd.startIndex = indicesRef.size();
		const fastgltf::Accessor& primitiveIdxAccessor = asset.accessors[primitive.indicesAccessor.value()];
		surfaceToAdd.count = static_cast<uint32_t>(primitiveIdxAccessor.count);

		//TODO: Make a function that can load any arbitrary primitive from glTF

		// load indexes
		{
			indicesRef.reserve(indicesRef.size() + primitiveIdxAccessor.count);
			fastgltf::iterateAccessor<uint32_t>(asset, primitiveIdxAccessor, [&](uint32_t idx) {
				indicesRef.push_back(idx /* + initialVertex? Maybe */);
			});
		}

		{
			const fastgltf::Accessor& posAccessor = asset.accessors[primitive.findAttribute("POSITION")->second];
			verticesRef.resize(verticesRef.size() + posAccessor.count);

			fastgltf::iterateAccessorWithIndex<glm::vec3>(asset, posAccessor,
				[&](glm::vec3 v, size_t index) {
					Vertex verexToAdd {};
					verexToAdd.position = v;
					verexToAdd.normal = { 1, 0, 0 };
					verexToAdd.color = glm::vec4{ 1.f };
					verexToAdd.uv_x = 0;
					verexToAdd.uv_y = 0;
					verticesRef[initialVertex + index] = verexToAdd;
				}
			);
		}

	}
}
