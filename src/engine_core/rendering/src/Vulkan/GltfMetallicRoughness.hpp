/*! \file GltfMetallicRoughness.hpp
    \author Kyn21kx
    \date 2024-05-31
    \brief Describes a GLTF asset to be loaded
*/

#pragma once
#include "VkTypes.hpp"
#include "VkDescriptors.hpp"
#include "VkMaterialInstance.hpp"

namespace Hush {
	class IRenderer;
	struct GLTFMetallicRoughness
	{
		VkMaterialPipeline opaquePipeline;
		VkMaterialPipeline transparentPipeline;

		VkDescriptorSetLayout materialLayout;

		struct MaterialConstants
		{
			glm::vec4 colorFactors;
			glm::vec4 metal_rough_factors;
			// padding, we need it anyway for uniform buffers
			glm::vec4 extra[14];
		};

		struct MaterialResources
		{
			AllocatedImage colorImage;
			VkSampler colorSampler;
			AllocatedImage metalRoughImage;
			VkSampler metalRoughSampler;
			VkBuffer dataBuffer;
			uint32_t dataBufferOffset;
		};

		DescriptorWriter writer;

		void BuildPipelines(IRenderer* engine, const std::string_view& fragmentShaderPath, const std::string_view& vertexShaderPath);
		void ClearResources(VkDevice device);

		VkMaterialInstance WriteMaterial(VkDevice device, EMaterialPass pass, const MaterialResources& resources,
			DescriptorAllocatorGrowable& descriptorAllocator);
	};
}