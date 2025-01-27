#include "VulkanFullScreenPass.hpp"
#include "Shared/ShaderMaterial.hpp"
#include "VulkanRenderer.hpp"
#include "VulkanPipelineBuilder.hpp"

namespace Hush {
	struct OpaqueMaterialData {
		VkMaterialPipeline pipeline;
		VkDescriptorSetLayout descriptorLayout;
		DescriptorWriter writer;
		VkBufferCreateInfo uniformBufferCreateInfo;
	};
}

Hush::VulkanFullScreenPass::VulkanFullScreenPass(VulkanRenderer* renderer, std::shared_ptr<ShaderMaterial> material)
{
	this->m_renderer = renderer;
	this->m_materialInstance = material;
}

void Hush::VulkanFullScreenPass::RecordCommands(VkCommandBuffer cmd, VkDescriptorSet globalDescriptorSet)
{
	OpaqueMaterialData* matData = this->m_materialInstance->GetMaterialData();
	VkPipeline pipeline = matData->pipeline.pipeline;
	VkDescriptorSet descSet = this->m_materialInstance->GetInternalMaterial().materialSet;
	// Bind pipeline
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, matData->pipeline.layout, 0, 1, &globalDescriptorSet, 0, nullptr);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, matData->pipeline.layout, 1, 1, &descSet, 0, nullptr);
	// Draw full-screen triangle
	vkCmdDraw(cmd, 4, 1, 0, 0); // 3 vertices = full-screen triangle
}

