#include "VulkanLoader.hpp"
#include <fastgltf/parser.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>
#include "VkTypes.hpp"
#include "VulkanRenderer.hpp"
#include "Shared/ImageTexture.hpp"
#include "VkMaterialInstance.hpp"
#include "VulkanMeshNode.hpp"
#include "VulkanAllocatedBuffer.hpp"
#include "vk_mem_alloc.hpp"

std::optional<std::vector<std::shared_ptr<Hush::MeshAsset>>> Hush::VulkanLoader::LoadGltfMeshes(VulkanRenderer* engine, std::filesystem::path filePath)
{
	fastgltf::GltfDataBuffer data;
	if (!std::filesystem::exists(filePath)) {
		LogFormat(ELogLevel::Error, "Failed to load mesh at {} file does not exist", filePath.string());
		return std::nullopt;
	}
	data.loadFromFile(filePath);

	constexpr fastgltf::Options loadingOptions = fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;

	fastgltf::Parser parser{};

	fastgltf::Expected<fastgltf::Asset> loadedAsset = parser.loadBinaryGLTF(&data, filePath.parent_path(), loadingOptions);

	HUSH_ASSERT(loadedAsset, "GLTF asset at {} not properly loaded, error: {}!", filePath.string(), fastgltf::getErrorMessage(loadedAsset.error()));
	
	std::vector<uint32_t> indices;
	std::vector<Vertex> vertices;
	std::vector<std::shared_ptr<MeshAsset>> meshes;
	for (const fastgltf::Mesh& mesh : loadedAsset->meshes)
	{
		VulkanMeshNode node = CreateMeshFromGltfMesh(mesh, loadedAsset.get(), indices, vertices, engine);
		meshes.push_back(node.m_mesh);
	}
	return meshes;
}

AllocatedImage Hush::VulkanLoader::LoadTexture(VulkanRenderer* engine, const ImageTexture& texture)
{	
	VkExtent3D extent{};
	extent.width = texture.GetWidth();
	extent.height = texture.GetHeight();
	extent.depth = 1;

	constexpr VkFormat defaultImageFormat = VK_FORMAT_R8G8B8A8_UNORM;
	
	return engine->CreateImage(texture.GetImageData(), extent, defaultImageFormat, VK_IMAGE_USAGE_SAMPLED_BIT);
}

Hush::VulkanMeshNode Hush::VulkanLoader::CreateMeshFromGltfMesh(const fastgltf::Mesh& mesh, const fastgltf::Asset& asset, std::vector<uint32_t>& indicesRef, std::vector<Vertex>& verticesRef, VulkanRenderer* engine)
{
	MeshAsset resultMesh;

	resultMesh.name = mesh.name;
	//Clear out the vector buffers
	indicesRef.clear();
	verticesRef.clear();

	for (const fastgltf::Primitive& primitive : mesh.primitives)
	{
		size_t initialVertex = verticesRef.size();
		GeoSurface surfaceToAdd{};
		surfaceToAdd.startIndex = static_cast<uint32_t>(indicesRef.size());
		const fastgltf::Accessor& primitiveIdxAccessor = asset.accessors[primitive.indicesAccessor.value()];
		surfaceToAdd.count = static_cast<uint32_t>(primitiveIdxAccessor.count);

		//TODO: Make a function that can load any arbitrary primitive from glTF

		// load indexes
		indicesRef.reserve(indicesRef.size() + primitiveIdxAccessor.count);
		fastgltf::iterateAccessor<uint32_t>(asset, primitiveIdxAccessor, [&](uint32_t idx) {
			indicesRef.push_back(idx + static_cast<uint32_t>(initialVertex));
			});

		std::vector<glm::vec3> vertexBuffer = FindAttributeByName<glm::vec3>(primitive, asset, "POSITION");
		for (const glm::vec3& v : vertexBuffer) {
			Vertex vertexToAdd{};
			vertexToAdd.position = v;
			verticesRef.push_back(vertexToAdd);
		}

		std::vector<glm::vec3> normalBuffer = FindAttributeByName<glm::vec3>(primitive, asset, "NORMAL");
		for (uint32_t i = 0; i < normalBuffer.size(); i++) {
			verticesRef.at(i + initialVertex).normal = normalBuffer.at(i);
		}

		// load UVs
		std::vector<glm::vec2> texBuffer = FindAttributeByName<glm::vec2>(primitive, asset, "TEXCOORD_0");

		for (uint32_t i = 0; i < texBuffer.size(); i++) {
			verticesRef.at(i + initialVertex).uv_x = texBuffer.at(i).x;
			verticesRef.at(i + initialVertex).uv_y = texBuffer.at(i).y;
		}

		// load vertex colors
		std::vector<glm::vec4> colors = FindAttributeByName<glm::vec4>(primitive, asset, "COLOR_0");

		for (uint32_t i = 0; i < colors.size(); i++) {
			verticesRef.at(i + initialVertex).color = colors.at(i);
		}

		resultMesh.surfaces.emplace_back(surfaceToAdd);
	}

	std::vector<VkSampler> samplers;
	samplers.reserve(asset.samplers.size());

	for (const fastgltf::Sampler& sampler : asset.samplers) {

		VkSamplerCreateInfo sampl = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr };
		sampl.maxLod = VK_LOD_CLAMP_NONE;
		sampl.minLod = 0;

		sampl.magFilter = ExtractFilter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
		sampl.minFilter = ExtractFilter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

		sampl.mipmapMode = ExtractMipMapMode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

		VkSampler newSampler;
		vkCreateSampler(engine->GetVulkanDevice(), &sampl, nullptr, &newSampler);

		samplers.push_back(newSampler);
	}

	VulkanAllocatedBuffer materialDataBuffer(
		sizeof(GLTFMetallicRoughness::MaterialConstants) * asset.materials.size(), 
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU,
		engine->GetVmaAllocator()
	);

	//TODO: constexpr(?
	std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = { { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 } };

	DescriptorAllocatorGrowable allocatorPool(engine->GetVulkanDevice(), asset.materials.size(), sizes);
	//Load mats first, then apply those to the primitives
	for (size_t materialIdx = 0; materialIdx < asset.materials.size(); materialIdx++) {
		auto materialInstance = GenerateMaterial(materialIdx, asset, engine, &materialDataBuffer, allocatorPool);
	}

	resultMesh.meshBuffers = engine->UploadMesh(indicesRef, verticesRef);
	VulkanMeshNode meshNode(std::make_shared<MeshAsset>(resultMesh));
	
	meshNode.SetDescriptorPool(allocatorPool);
	meshNode.SetMaterialDataBuffer(materialDataBuffer);
	return meshNode;
}

Hush::Result<const uint8_t*, Hush::VulkanLoader::EError> Hush::VulkanLoader::GetDataFromBufferSource(const fastgltf::Buffer& buffer)
{
	const fastgltf::sources::Vector* vectorData = std::get_if<fastgltf::sources::Vector>(&buffer.data);
	if (vectorData != nullptr) {
		return vectorData->bytes.data();
	}
	//Ok, try the ByteView
	const fastgltf::sources::ByteView* byteData = std::get_if<fastgltf::sources::ByteView>(&buffer.data);
	if (byteData != nullptr) {
		return reinterpret_cast<const uint8_t*>(byteData->bytes.data());
	}
	//Else, idk, we don't recognize this yet
	return EError::InvalidMeshFile;
}

std::shared_ptr<Hush::VkMaterialInstance> Hush::VulkanLoader::GenerateMaterial(size_t materialIdx, const fastgltf::Asset& asset, VulkanRenderer* engine, VulkanAllocatedBuffer* sceneMaterialBuffer, DescriptorAllocatorGrowable& allocatorPool)
{
	const fastgltf::Material& material = asset.materials.at(materialIdx);
	std::optional<ImageTexture> textureToUse = GetTexturePropertiesFromMaterial(asset, material);
	if (!textureToUse.has_value()) {
		//Default material without texture will be created, but this is temporary, let's change it
		return nullptr; 
	}

	GLTFMetallicRoughness::MaterialConstants constants;
	HUSH_STATIC_ASSERT(
		sizeof(constants.colorFactors) == sizeof(material.pbrData.baseColorFactor),
		"Material constants' colors are not the same size as fastgltf pbr data colors, make sure fastgltf is compiled with using num = float"
	);
	constants.colorFactors = *reinterpret_cast<const glm::vec4*>(&material.pbrData.baseColorFactor);

	constants.metalRoughFactors.x = material.pbrData.metallicFactor;
	constants.metalRoughFactors.y = material.pbrData.roughnessFactor;

	//Scene Material buffer writing
	VmaAllocationInfo& allocInfo = sceneMaterialBuffer->GetAllocationInfo();
	GLTFMetallicRoughness::MaterialConstants* mappedData = static_cast<GLTFMetallicRoughness::MaterialConstants*>(allocInfo.pMappedData);
	mappedData[materialIdx] = constants;

	EMaterialPass passType = EMaterialPass::MainColor;
	if (material.alphaMode == fastgltf::AlphaMode::Blend) {
		passType = EMaterialPass::Transparent;
	}

	GLTFMetallicRoughness::MaterialResources materialResources;
	// default the material textures

	materialResources.colorImage = LoadTexture(engine, textureToUse.value());
	materialResources.colorSampler = engine->GetDefaultSamplerLinear();
	materialResources.metalRoughImage = engine->GetDefaultWhiteImage();
	materialResources.metalRoughSampler = engine->GetDefaultSamplerLinear();

	// set the uniform buffer for the material data
	materialResources.dataBuffer = sceneMaterialBuffer->GetBuffer();
	materialResources.dataBufferOffset = materialIdx * sizeof(GLTFMetallicRoughness::MaterialConstants);
	// grab textures from gltf file

	VkMaterialInstance finalMat = engine->GetMetalRoughMaterial().WriteMaterial(engine->GetVulkanDevice(), EMaterialPass::MainColor, materialResources, allocatorPool);
	return std::make_shared<VkMaterialInstance>(finalMat);
}

std::optional<Hush::ImageTexture> Hush::VulkanLoader::GetTexturePropertiesFromMaterial(const fastgltf::Asset& asset, const fastgltf::Material& material)
{
	if (!material.pbrData.baseColorTexture.has_value()) {
		return std::nullopt;
	}
	size_t textureDataIdx = material.pbrData.baseColorTexture->textureIndex;
	const fastgltf::Texture& fastgltfTexture = asset.textures.at(textureDataIdx);
	if (!fastgltfTexture.imageIndex.has_value()) {
		return std::nullopt;
	}
	const fastgltf::Image& image = asset.images.at(fastgltfTexture.imageIndex.value());
	return TextureFromImageDataSource(image);
}

std::optional<Hush::ImageTexture> Hush::VulkanLoader::TextureFromImageDataSource(const fastgltf::Image& image)
{
	const fastgltf::sources::Vector* vectorData = std::get_if<fastgltf::sources::Vector>(&image.data);
	if (vectorData != nullptr) {
		return ImageTexture(reinterpret_cast<const std::byte*>(vectorData->bytes.data()), vectorData->bytes.size());
	}
	const fastgltf::sources::URI* uriData = std::get_if<fastgltf::sources::URI>(&image.data);
	
	//TODO: support for file byte offset
	if (uriData == nullptr || uriData->fileByteOffset > 0) { 
		return std::nullopt;
	}
	return ImageTexture(uriData->uri.fspath());
}


constexpr VkFilter Hush::VulkanLoader::ExtractFilter(const fastgltf::Filter& filter)
{
	switch (filter) {
		case fastgltf::Filter::Nearest:
		case fastgltf::Filter::NearestMipMapNearest:
		case fastgltf::Filter::NearestMipMapLinear:
			return VK_FILTER_NEAREST;

		case fastgltf::Filter::Linear:
		case fastgltf::Filter::LinearMipMapNearest:
		case fastgltf::Filter::LinearMipMapLinear:
		default:
			return VK_FILTER_LINEAR;
	}
}

constexpr VkSamplerMipmapMode Hush::VulkanLoader::ExtractMipMapMode(const fastgltf::Filter& filter)
{
	switch (filter) {
		case fastgltf::Filter::NearestMipMapNearest:
		case fastgltf::Filter::LinearMipMapNearest:
			return VK_SAMPLER_MIPMAP_MODE_NEAREST;

		case fastgltf::Filter::NearestMipMapLinear:
		case fastgltf::Filter::LinearMipMapLinear:
		default:
			return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	}
}
