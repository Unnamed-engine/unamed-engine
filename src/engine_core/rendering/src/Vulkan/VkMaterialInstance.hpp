#pragma once
#include <vulkan/vulkan_core.h>
namespace Hush {
	enum class EMaterialPass : uint8_t {
		MainColor,
		Transparent,
		Other
	};

	struct VkMaterialPipeline {
		VkPipeline pipeline;
		VkPipelineLayout layout;
	};

	struct VkMaterialInstance {
		VkMaterialPipeline* pipeline;
		VkDescriptorSet materialSet;
		EMaterialPass passType;
	};
}
