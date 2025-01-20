#pragma once
namespace Hush {

	struct ShaderBindings {
		std::string name; //TODO: Maybe remove the string(?
		uint32_t bindingIndex;
		uint32_t size;
		uint32_t offset;
		uint32_t stageFlags;
	};

}
