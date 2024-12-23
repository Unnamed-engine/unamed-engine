#include "GltfMetallicRoughness.hpp"
#include "VulkanRenderer.hpp"
#include "VulkanPipelineBuilder.hpp"
#include "VkUtilsFactory.hpp"

void Hush::GLTFMetallicRoughness::BuildPipelines(IRenderer* engine, const std::string_view& fragmentShaderPath, const std::string_view& vertexShaderPath)
{
	auto* vkEngine = static_cast<VulkanRenderer*>(engine);
	VkShaderModule meshFragmentShader;
	VkDevice device = vkEngine->GetVulkanDevice();

	if (!VulkanHelper::LoadShaderModule(fragmentShaderPath, device, &meshFragmentShader)) {
		LogError("Error when building the triangle fragment shader module");
	}

	VkShaderModule meshVertexShader;
	if (!VulkanHelper::LoadShaderModule(vertexShaderPath, device, &meshVertexShader)) {
		LogError("Error when building the triangle vertex shader module");
	}

	VkPushConstantRange matrixRange{};
	matrixRange.offset = 0;
	matrixRange.size = sizeof(GPUDrawPushConstants);
	matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	//Create the material layout
	DescriptorLayoutBuilder layoutBuilder{};
	layoutBuilder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	layoutBuilder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	layoutBuilder.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	this->materialLayout = layoutBuilder.Build(device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	
	// Create the mesh layouts
	VkDescriptorSetLayout layouts[] = { vkEngine->GetGpuSceneDataDescriptorLayout(), this->materialLayout };

	VkPipelineLayoutCreateInfo meshLayoutInfo = VkUtilsFactory::PipelineLayoutCreateInfo();
	meshLayoutInfo.setLayoutCount = 2;
	meshLayoutInfo.pSetLayouts = layouts;
	meshLayoutInfo.pPushConstantRanges = &matrixRange;
	meshLayoutInfo.pushConstantRangeCount = 1;

	VkPipelineLayout newLayout;
	VkResult rc = vkCreatePipelineLayout(device, &meshLayoutInfo, nullptr, &newLayout);
	HUSH_VK_ASSERT(rc, "Failed to create pipeline mesh pipeline layout!");

	this->opaquePipeline.layout = newLayout;
	this->transparentPipeline.layout = newLayout;

	// build the stage-create-info for both vertex and fragment stages. This lets
	// the pipeline know the shader modules per stage
	VulkanPipelineBuilder pipelineBuilder(newLayout);
	pipelineBuilder.SetShaders(meshVertexShader, meshFragmentShader);
	pipelineBuilder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipelineBuilder.SetPolygonMode(VK_POLYGON_MODE_FILL);
	pipelineBuilder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	pipelineBuilder.SetMultiSamplingNone();
	pipelineBuilder.DisableBlending();
	pipelineBuilder.EnableDepthTest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

	//render format
	pipelineBuilder.SetColorAttachmentFormat(vkEngine->GetDrawImage().imageFormat);
	pipelineBuilder.SetDepthFormat(vkEngine->GetDepthImage().imageFormat);

	// finally build the pipeline
	this->opaquePipeline.pipeline = pipelineBuilder.Build(device);

	// create the transparent variant
	pipelineBuilder.EnableBlendingAdditive();

	pipelineBuilder.EnableDepthTest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);

	this->transparentPipeline.pipeline = pipelineBuilder.Build(device);

	//clean structures
	vkDestroyShaderModule(device, meshFragmentShader, nullptr);
	vkDestroyShaderModule(device, meshVertexShader, nullptr);

}

VkMaterialInstance Hush::GLTFMetallicRoughness::WriteMaterial(VkDevice device, EMaterialPass pass, const MaterialResources& resources, DescriptorAllocatorGrowable& descriptorAllocator)
{
	VkMaterialInstance matData;
	matData.passType = pass;
	if (pass == EMaterialPass::Transparent) {
		matData.pipeline = &transparentPipeline;
	}
	else {
		matData.pipeline = &opaquePipeline;
	}

	matData.materialSet = descriptorAllocator.Allocate(device, materialLayout);


	writer.Clear();
	writer.WriteBuffer(0, resources.dataBuffer, sizeof(MaterialConstants), resources.dataBufferOffset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	writer.WriteImage(1, resources.colorImage.imageView, resources.colorSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	writer.WriteImage(2, resources.metalRoughImage.imageView, resources.metalRoughSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	writer.UpdateSet(device, matData.materialSet);

	return matData;
}
