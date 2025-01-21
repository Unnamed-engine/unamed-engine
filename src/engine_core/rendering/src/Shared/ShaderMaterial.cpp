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
	
	//Bind

	VkShaderModule meshVertexShader;
	if (!VulkanHelper::LoadShaderModule(vertexShaderPath.string(), device, &meshVertexShader, &spirvByteCodeBuffer)) {
		return EError::VertexShaderNotFound;
	}

	//Reflect on vertex shader (using the same buffer to avoid more allocations)
	byteCodeSpan = std::span<uint32_t>(spirvByteCodeBuffer.begin(), spirvByteCodeBuffer.end());
	bindingsResult = this->ReflectShader(byteCodeSpan);
	
	//Bind

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
	size_t byteCodeLength = shaderBinary.size() * sizeof(uint32_t);
	SpvReflectShaderModule reflectionModule;
	SpvReflectResult rc = spvReflectCreateShaderModule(byteCodeLength, shaderBinary.data(), &reflectionModule);

	if (rc != SpvReflectResult::SPV_REFLECT_RESULT_SUCCESS) {
		LogFormat(ELogLevel::Error, "Failed to perform reflection on shader, error: {}", magic_enum::enum_name(rc));
		return EError::ReflectionError;
	}

	uint32_t pushConstantsCount{};
	uint32_t inputVarsCount{};
	uint32_t descriptorCount{};

	spvReflectEnumeratePushConstantBlocks(&reflectionModule, &pushConstantsCount, nullptr);
	spvReflectEnumerateInputVariables(&reflectionModule, &inputVarsCount, nullptr);
	spvReflectEnumerateDescriptorBindings(&reflectionModule, &descriptorCount, nullptr);

	std::vector<SpvReflectBlockVariable*> pushConstants(pushConstantsCount);
	spvReflectEnumeratePushConstantBlocks(&reflectionModule, &pushConstantsCount, pushConstants.data());

	std::vector<ShaderBindings> bindings;
	bindings.reserve(pushConstantsCount + inputVarsCount + descriptorCount);
	for (const SpvReflectBlockVariable* pushConstant : pushConstants) {
		ShaderBindings binding;
		binding.name = pushConstant->name;
		binding.bindingIndex = 0;  // Push constants are not bound to a specific index
		binding.size = pushConstant->size;
		binding.offset = pushConstant->offset;
		binding.stageFlags = reflectionModule.shader_stage;
		binding.type = ShaderBindings::EBindingType::PushConstant;
		bindings.emplace_back(binding);
	}

	std::vector<SpvReflectInterfaceVariable*> inputVars(inputVarsCount);
	spvReflectEnumerateInputVariables(&reflectionModule, &inputVarsCount, inputVars.data());


	for (const SpvReflectInterfaceVariable* inputVar : inputVars) {
		ShaderBindings binding;
		binding.type = ShaderBindings::EBindingType::InputVariable;
		binding.name = inputVar->name;
		binding.bindingIndex = inputVar->location;
		binding.size = 0;
		binding.offset = inputVar->word_offset.location;
		binding.stageFlags = reflectionModule.shader_stage;
		bindings.emplace_back(binding);
	}
	std::vector<SpvReflectDescriptorBinding*> descriptorBindings(descriptorCount);
	spvReflectEnumerateDescriptorBindings(&reflectionModule, &descriptorCount, descriptorBindings.data());


	for (const SpvReflectDescriptorBinding* descriptor : descriptorBindings) {

		ShaderBindings binding;
		binding.name = descriptor->name;
		binding.bindingIndex = descriptor->binding;
		binding.size = descriptor->block.size;
		binding.offset = descriptor->block.offset;
		//binding.stageFlags = descriptor->;
		switch (descriptor->descriptor_type) {
		case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			binding.type = ShaderBindings::EBindingType::UniformBuffer;
			break;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
			binding.type = ShaderBindings::EBindingType::StorageBuffer;
			break;
		case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
			binding.type = ShaderBindings::EBindingType::Sampler;
			break;
		case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
			binding.type = ShaderBindings::EBindingType::Texture;
			break;
		default:
			LogFormat(ELogLevel::Warn, "Binding type {} not supported!", magic_enum::enum_name(descriptor->descriptor_type));
			binding.type = ShaderBindings::EBindingType::Unknown;
			break;
		}

		bindings.emplace_back(binding);
		if (descriptor->descriptor_type != SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
			continue;
		}

		//Now, for each member, add it if applicable
		for (uint32_t i = 0; i < descriptor->block.member_count; ++i) {
			const SpvReflectBlockVariable& member = descriptor->block.members[i];

			// Create a new ShaderBindings entry for each member
			Hush::ShaderBindings memberBinding;
			memberBinding.name = member.name;
			memberBinding.bindingIndex = descriptor->binding;  // Same binding index as the block
			memberBinding.setIndex = descriptor->set;
			memberBinding.size = member.size;
			memberBinding.offset = member.offset;  // Offset within the uniform block
			memberBinding.type = ShaderBindings::EBindingType::UniformBufferMember;  // Add a type for UBO members
			memberBinding.stageFlags = descriptor->spirv_id;

			bindings.emplace_back(memberBinding);
		}


	}
	spvReflectDestroyShaderModule(&reflectionModule);
	return bindings;
}
uint32_t Hush::ShaderMaterial::GetAPIBinding(Hush::ShaderBindings::EBindingType agnosticBinding)
{
#ifdef HUSH_VULKAN_IMPL
	switch (agnosticBinding)
	{
		case ShaderBindings::EBindingType::Sampler:
            return VK_DESCRIPTOR_TYPE_SAMPLER;
        case ShaderBindings::EBindingType::CombinedImageSampler:
            return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        case ShaderBindings::EBindingType::Texture:
            return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case ShaderBindings::EBindingType::StorageImage:
            return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case ShaderBindings::EBindingType::UniformTexelBuffer:
            return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        case ShaderBindings::EBindingType::StorageTexelBuffer:
            return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        case ShaderBindings::EBindingType::UniformBuffer:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case ShaderBindings::EBindingType::StorageBuffer:
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case ShaderBindings::EBindingType::UniformBufferDynamic:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        case ShaderBindings::EBindingType::StorageBufferDynamic:
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        case ShaderBindings::EBindingType::InputAttachment:
            return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        case ShaderBindings::EBindingType::InlineUniformBlock:
            return VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK;
        case ShaderBindings::EBindingType::AccelerationStructure:
            return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        default:
            return VK_DESCRIPTOR_TYPE_MAX_ENUM;
	}
#endif
}
