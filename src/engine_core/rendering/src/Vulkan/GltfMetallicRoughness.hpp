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
			glm::vec4 metalRoughFactors;
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

		inline VkMaterialInstance WriteMaterial(VkDevice device, EMaterialPass pass, const MaterialResources& resources, DescriptorAllocatorGrowable& descriptorAllocator) {
			Hush::VkMaterialInstance matData;
			matData.passType = pass;
			if (pass == EMaterialPass::Transparent) {
				matData.pipeline = &transparentPipeline;
			}
			else {
				matData.pipeline = &opaquePipeline;
			}

			matData.materialSet = descriptorAllocator.Allocate(device, this->materialLayout);


			writer.Clear();
			writer.WriteBuffer(0, resources.dataBuffer, sizeof(MaterialConstants), resources.dataBufferOffset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
			writer.WriteImage(1, resources.colorImage.imageView, resources.colorSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
			writer.WriteImage(2, resources.metalRoughImage.imageView, resources.metalRoughSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

			writer.UpdateSet(device, matData.materialSet);

			return matData;
		}
	};
}