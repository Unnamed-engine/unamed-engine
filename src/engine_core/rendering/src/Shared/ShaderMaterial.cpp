#include "ShaderMaterial.hpp"
#include "Vulkan/VkTypes.hpp"
#include <spirv_reflect.h>
#include <magic_enum.hpp>

#ifdef HUSH_VULKAN_IMPL
#include "Vulkan/VulkanRenderer.hpp"
#include "Vulkan/VulkanPipelineBuilder.hpp"
#include "Vulkan/VkMaterialInstance.hpp"
#include "Vulkan/VkUtilsFactory.hpp"

namespace Hush {
	struct OpaqueMaterialPipeline {
		VkMaterialPipeline pipeline;
		VkDescriptorSetLayout descriptorLayout;
		DescriptorWriter writer;
	};
}
#endif

Hush::ShaderMaterial::~ShaderMaterial()
{
	//Free resources more specifically later
	delete this->m_materialPipeline;
}

Hush::ShaderMaterial::EError Hush::ShaderMaterial::LoadShaders(IRenderer* renderer, const std::filesystem::path& fragmentShaderPath, const std::filesystem::path& vertexShaderPath)
{
	this->m_renderer = renderer;
#ifdef HUSH_VULKAN_IMPL
	VulkanRenderer* rendererImpl = static_cast<VulkanRenderer*>(renderer);
	VkDevice device = rendererImpl->GetVulkanDevice();

	this->m_materialPipeline = new OpaqueMaterialPipeline();

	VkShaderModule meshFragmentShader;
	std::vector<uint32_t> spirvByteCodeBuffer;

	if (!VulkanHelper::LoadShaderModule(fragmentShaderPath.string(), device, &meshFragmentShader, &spirvByteCodeBuffer)) {
		return EError::FragmentShaderNotFound;
	}

	//Reflect on fragment shader
	std::span<uint32_t> byteCodeSpan(spirvByteCodeBuffer.begin(), spirvByteCodeBuffer.end());
	Result<std::vector<ShaderBindings>, EError> bindingsResult = this->ReflectShader(byteCodeSpan);

	VkShaderModule meshVertexShader;
	if (!VulkanHelper::LoadShaderModule(vertexShaderPath.string(), device, &meshVertexShader, &spirvByteCodeBuffer)) {
		return EError::VertexShaderNotFound;
	}

	//Reflect on vertex shader (using the same buffer to avoid more allocations)

	VkPushConstantRange matrixRange{};
	matrixRange.offset = 0;
	matrixRange.size = sizeof(GPUDrawPushConstants);
	matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	//Create the material layout
	DescriptorLayoutBuilder layoutBuilder{};
	layoutBuilder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	layoutBuilder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	layoutBuilder.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	
	this->m_materialPipeline->descriptorLayout = layoutBuilder.Build(device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

	// Create the mesh layouts
	VkDescriptorSetLayout layouts[] = { rendererImpl->GetGpuSceneDataDescriptorLayout(), this->m_materialPipeline->descriptorLayout };

	VkPipelineLayoutCreateInfo meshLayoutInfo = VkUtilsFactory::PipelineLayoutCreateInfo();
	meshLayoutInfo.setLayoutCount = 2;
	meshLayoutInfo.pSetLayouts = layouts;
	meshLayoutInfo.pPushConstantRanges = &matrixRange;
	meshLayoutInfo.pushConstantRangeCount = 1;

	VkPipelineLayout newLayout;
	VkResult rc = vkCreatePipelineLayout(device, &meshLayoutInfo, nullptr, &newLayout);
	HUSH_VK_ASSERT(rc, "Failed to create pipeline mesh pipeline layout!");
	this->m_materialPipeline->pipeline.layout = newLayout;

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
	pipelineBuilder.SetColorAttachmentFormat(rendererImpl->GetDrawImage().imageFormat);
	pipelineBuilder.SetDepthFormat(rendererImpl->GetDepthImage().imageFormat);

	// finally build the pipeline
	this->m_materialPipeline->pipeline.pipeline = pipelineBuilder.Build(device);

	//clean structures
	vkDestroyShaderModule(device, meshFragmentShader, nullptr);
	vkDestroyShaderModule(device, meshVertexShader, nullptr);

#endif
	return EError::None;
}

void Hush::ShaderMaterial::GenerateMaterialInstance(OpaqueDescriptorAllocator* descriptorAllocator, void* outMaterialInstance)
{
	VulkanRenderer* rendererImpl = static_cast<VulkanRenderer*>(this->m_renderer);
	VkDevice device = rendererImpl->GetVulkanDevice();

	Hush::VkMaterialInstance matData;
	matData.passType = EMaterialPass::MainColor;
	
	//Make sure that we can cast this stuff
	auto* realDescriptorAllocator = reinterpret_cast<DescriptorAllocatorGrowable*>(descriptorAllocator);


	//Not initialized material layout here from VkLoader
	matData.materialSet = realDescriptorAllocator->Allocate(device, this->m_materialPipeline->descriptorLayout);

	// This should be where I write to the shader the stuff I want, but I wouldn't know that until I have the shader
	//this->m_materialPipeline->writer.Clear();
	//this->m_materialPipeline->writer.WriteBuffer(0, resources.dataBuffer, sizeof(MaterialConstants), resources.dataBufferOffset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	//this->m_materialPipeline->writer.WriteImage(1, resources.colorImage.imageView, resources.colorSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	//this->m_materialPipeline->writer.WriteImage(2, resources.metalRoughImage.imageView, resources.metalRoughSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	//writer.UpdateSet(device, matData.materialSet);
	memcpy(outMaterialInstance, &matData, sizeof(Hush::VkMaterialInstance));
}

Hush::Result<std::vector<Hush::ShaderBindings>, Hush::ShaderMaterial::EError> Hush::ShaderMaterial::ReflectShader(std::span<uint32_t> shaderBinary)
{
#ifdef HUSH_VULKAN_IMPL
	size_t byteCodeLength = shaderBinary.size() * sizeof(uint32_t);
	SpvReflectShaderModule reflectionModule;
	SpvReflectResult rc = spvReflectCreateShaderModule(byteCodeLength, shaderBinary.data(), &reflectionModule);
	
	if (rc != SpvReflectResult::SPV_REFLECT_RESULT_SUCCESS) {
		LogFormat(ELogLevel::Error, "Failed to perform reflection on shader, error: {}", magic_enum::enum_name(rc));
		return EError::ReflectionError;
	}
	uint32_t pushConstantsCount{};
	spvReflectEnumeratePushConstantBlocks(&reflectionModule, &pushConstantsCount, nullptr);
	std::vector<SpvReflectBlockVariable*> pushConstants(pushConstantsCount);
	spvReflectEnumeratePushConstantBlocks(&reflectionModule, &pushConstantsCount, pushConstants.data());
	std::vector<ShaderBindings> bindings(pushConstantsCount);

	for (const SpvReflectBlockVariable* pushConstant : pushConstants) {
		ShaderBindings binding;
		binding.name = pushConstant->name;
		binding.bindingIndex = 0;  // Push constants are not bound to a specific index
		binding.size = pushConstant->size;
		binding.offset = pushConstant->offset;
		binding.stageFlags = reflectionModule.shader_stage;

		bindings.push_back(binding);
	}

	spvReflectDestroyShaderModule(&reflectionModule);

	return bindings;
#endif // HUSH_VULKAN_IMPL

}
