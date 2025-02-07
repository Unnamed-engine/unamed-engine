#pragma once
#include <vulkan/vulkan_core.h>
namespace Hush {
	enum class EMaterialPass : uint8_t {
		MainColor,
		Transparent,
		Mask,
		Other
	};

	struct VkMaterialPipeline {
		VkPipeline pipeline;
		VkPipelineLayout layout; //Check deletion queue?
	};

	struct VkMaterialInstance {
		VkMaterialPipeline* pipeline;
		VkDescriptorSet materialSet;
		EMaterialPass passType;
	};
}
