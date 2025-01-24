#include "ShaderMaterial.hpp"
#include <spirv_reflect.h>
#include <magic_enum.hpp>

#ifdef HUSH_VULKAN_IMPL
#define VK_NO_PROTOTYPES
#include <volk.h>
#include "Vulkan/VkTypes.hpp"
#include "Vulkan/VulkanRenderer.hpp"
#include "Vulkan/VulkanPipelineBuilder.hpp"
#include "Vulkan/VkMaterialInstance.hpp"
#include "Vulkan/VkUtilsFactory.hpp"
namespace Hush {
	struct OpaqueMaterialData {
		VkMaterialPipeline pipeline;
		VkDescriptorSetLayout descriptorLayout;
		DescriptorWriter writer;
		VkBufferCreateInfo uniformBufferCreateInfo;
	};
}
#endif
#include "BitwiseUtils.hpp"

Hush::ShaderMaterial::~ShaderMaterial()
{
	//Free resources more specifically later
	delete this->m_materialData;
}

Hush::ShaderMaterial::EError Hush::ShaderMaterial::LoadShaders(IRenderer* renderer, const std::filesystem::path& fragmentShaderPath, const std::filesystem::path& vertexShaderPath)
{
	this->m_renderer = renderer;
#ifdef HUSH_VULKAN_IMPL
	VulkanRenderer* rendererImpl = static_cast<VulkanRenderer*>(renderer);
	VkDevice device = rendererImpl->GetVulkanDevice();

	this->m_materialData = new OpaqueMaterialData();
	this->InitializeMaterialDataMembers();

	VkShaderModule meshFragmentShader;
	std::vector<uint32_t> spirvByteCodeBuffer;

	if (!VulkanHelper::LoadShaderModule(fragmentShaderPath.string(), device, &meshFragmentShader, &spirvByteCodeBuffer)) {
		return EError::FragmentShaderNotFound;
	}

	//Reflect on fragment shader
	std::span<uint32_t> byteCodeSpan(spirvByteCodeBuffer.begin(), spirvByteCodeBuffer.end());
	Result<std::vector<ShaderBindings>, EError> fragBindingsResult = this->ReflectShader(byteCodeSpan);
	
	VkShaderModule meshVertexShader;
	if (!VulkanHelper::LoadShaderModule(vertexShaderPath.string(), device, &meshVertexShader, &spirvByteCodeBuffer)) {
		return EError::VertexShaderNotFound;
	}

	//Reflect on vertex shader (using the same buffer to avoid more allocations)
	byteCodeSpan = std::span<uint32_t>(spirvByteCodeBuffer.begin(), spirvByteCodeBuffer.end());
	Result<std::vector<ShaderBindings>, EError> vertBindingsResult = this->ReflectShader(byteCodeSpan);

	this->BindShader(vertBindingsResult.value(), fragBindingsResult.value());
	// build the stage-create-info for both vertex and fragment stages. This lets
	// the pipeline know the shader modules per stage
	VulkanPipelineBuilder pipelineBuilder(this->m_materialData->pipeline.layout);
	pipelineBuilder.SetShaders(meshVertexShader, meshFragmentShader);
	pipelineBuilder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipelineBuilder.SetPolygonMode(VK_POLYGON_MODE_FILL);
	//TODO: Make cull mode dynamic depending on the reflected shader code / inspector
	pipelineBuilder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	pipelineBuilder.SetMultiSamplingNone();
	pipelineBuilder.DisableBlending();
	pipelineBuilder.EnableDepthTest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

	//render format
	pipelineBuilder.SetColorAttachmentFormat(rendererImpl->GetDrawImage().imageFormat);
	pipelineBuilder.SetDepthFormat(rendererImpl->GetDepthImage().imageFormat);

	// finally build the pipeline
	this->m_materialData->pipeline.pipeline = pipelineBuilder.Build(device);

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
	matData.materialSet = realDescriptorAllocator->Allocate(device, this->m_materialData->descriptorLayout);

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


	size_t inputVarsByteLength = this->m_shaderInputData.size();
	for (const SpvReflectInterfaceVariable* inputVar : inputVars) {
		ShaderBindings binding;
		binding.type = ShaderBindings::EBindingType::InputVariable;
		binding.name = inputVar->name;
		binding.bindingIndex = inputVar->location;
		size_t calculatedSize = this->CalculateTypeSize(inputVar->type_description);
		binding.size = static_cast<uint32_t>(calculatedSize);
		binding.offset = inputVar->word_offset.location;
		binding.stageFlags = reflectionModule.shader_stage;
		inputVarsByteLength += calculatedSize;
		bindings.emplace_back(binding);
	}
	this->m_shaderInputData.resize(inputVarsByteLength);
	std::vector<SpvReflectDescriptorBinding*> descriptorBindings(descriptorCount);
	spvReflectEnumerateDescriptorBindings(&reflectionModule, &descriptorCount, descriptorBindings.data());

	size_t uniformBuffersSize = this->m_uniformBufferData.size();
	for (const SpvReflectDescriptorBinding* descriptor : descriptorBindings) {

		ShaderBindings binding;
		binding.name = descriptor->name;
		binding.bindingIndex = descriptor->binding;
		binding.size = descriptor->block.size;
		binding.offset = descriptor->block.offset;
		binding.stageFlags = 0;
		switch (descriptor->descriptor_type) {
		case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			binding.type = ShaderBindings::EBindingType::UniformBuffer;
			uniformBuffersSize += binding.size;
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
		this->m_uniformBufferData.resize(uniformBuffersSize);
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

Hush::ShaderMaterial::EError Hush::ShaderMaterial::BindShader(const std::vector<ShaderBindings>& vertBindings, const std::vector<ShaderBindings>& fragBindings)
{
#ifdef HUSH_VULKAN_IMPL
	VulkanRenderer* rendererImpl = static_cast<VulkanRenderer*>(this->m_renderer);
	VkDevice device = rendererImpl->GetVulkanDevice();

	std::vector<VkPushConstantRange> pushConstants;
	DescriptorLayoutBuilder layoutBuilder{};

	size_t uniformBufferSize = 0;

	for (const ShaderBindings& b : vertBindings) {
		//TODO: Refactor this as a function
		switch (b.type)
		{
		case ShaderBindings::EBindingType::PushConstant:
			pushConstants.push_back({
				.stageFlags = b.stageFlags,
				.offset = b.offset,
				.size = b.size
			});
			break;

		case ShaderBindings::EBindingType::UniformBuffer:
			//For uniform buffers, we can add stuff to allocate a VK memory buffer
			uniformBufferSize += b.size;
			//NOTE: Yes, the lack of a break here is intentional
		case ShaderBindings::EBindingType::CombinedImageSampler:
			//Add bindings for applicable types
			layoutBuilder.AddBinding(b.bindingIndex, static_cast<VkDescriptorType>(this->GetAPIBinding(b.type)), b.stageFlags);
			break;
		}
	}
	for (const ShaderBindings& b : fragBindings) {
		switch (b.type)
		{
		case ShaderBindings::EBindingType::PushConstant:
			pushConstants.push_back({
				.stageFlags = b.stageFlags,
				.offset = b.offset,
				.size = b.size
				});
			break;

		case ShaderBindings::EBindingType::UniformBuffer:
			//For uniform buffers, we can add stuff to allocate a VK memory buffer
			uniformBufferSize += b.size;
			//NOTE: Yes, the lack of a break here is intentional
		case ShaderBindings::EBindingType::CombinedImageSampler:
			//Add bindings for applicable types
			layoutBuilder.AddBinding(b.bindingIndex, static_cast<VkDescriptorType>(this->GetAPIBinding(b.type)), b.stageFlags);
			break;
		}
	}

	this->m_materialData->descriptorLayout = layoutBuilder.Build(device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

	const VkDescriptorSetLayout layouts[] = {
		rendererImpl->GetGpuSceneDataDescriptorLayout(),
		m_materialData->descriptorLayout
	};

	//Bind push constants
	VkPipelineLayoutCreateInfo layoutInfo = VkUtilsFactory::PipelineLayoutCreateInfo();
	layoutInfo.setLayoutCount = 2;
	layoutInfo.pSetLayouts = layouts;
	layoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstants.size());
	layoutInfo.pPushConstantRanges = pushConstants.data();

	VkPipelineLayout newLayout;
	VkResult rc = vkCreatePipelineLayout(device, &layoutInfo, nullptr, &newLayout);
	HUSH_VK_ASSERT(rc, "Failed to create pipeline layout for custom shader material!");

	this->m_materialData->pipeline.layout = newLayout;


	return EError::None;
#endif // HUSH_VULKAN_IMPL
}

void Hush::ShaderMaterial::InitializeMaterialDataMembers()
{
#ifdef HUSH_VULKAN_IMPL
	this->m_materialData->uniformBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	this->m_materialData->uniformBufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	this->m_materialData->uniformBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
#endif // HUSH_VULKAN_IMPL
}

size_t Hush::ShaderMaterial::CalculateTypeSize(const SpvReflectTypeDescription* type)
{
	int32_t typeFlag = type->type_flags; //Narrow this from uint to int
	constexpr SpvReflectTypeFlags fourByteFlags = SPV_REFLECT_TYPE_FLAG_BOOL | SPV_REFLECT_TYPE_FLAG_INT | SPV_REFLECT_TYPE_FLAG_FLOAT;
	size_t cumSize = 0;
	constexpr size_t byteLengthBits = 8;
	if (Bitwise::HasCompositeFlag(typeFlag, fourByteFlags)) {
		cumSize += type->traits.numeric.scalar.width / byteLengthBits; //Use standard integers to base our sizes off of
	}

	if (Bitwise::HasFlag(typeFlag, SPV_REFLECT_TYPE_FLAG_MATRIX)) {
		uint32_t rows = type->traits.numeric.matrix.row_count;
		uint32_t cols = type->traits.numeric.matrix.column_count;
		cumSize *= rows * cols;
	}
	//Wrapping this in an else because matrices have more "priority" than vectors
	//As matrix types will also contain the vector flag and will lead to duplicate calculations
	else if (Bitwise::HasFlag(typeFlag, SPV_REFLECT_TYPE_FLAG_VECTOR)) {
		uint32_t component_count = type->traits.numeric.vector.component_count;
		cumSize *= component_count;
	}

	if (Bitwise::HasFlag(typeFlag, SPV_REFLECT_TYPE_FLAG_STRUCT)) {
		size_t total_size = 0;
		for (uint32_t i = 0; i < type->member_count; ++i) {
			total_size += CalculateTypeSize(&type->members[i]);
		}
		return total_size;
	}

	return cumSize; // Handle unsupported types
}
