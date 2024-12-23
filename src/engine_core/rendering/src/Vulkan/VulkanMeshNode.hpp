#pragma once
#include "Shared/RenderableNode.hpp"

namespace Hush {
	class MeshAsset;

	class VulkanMeshNode : public RenderableNode {

	public:
		VulkanMeshNode() = default;

		void Draw(const glm::mat4& topMatrix, void* drawContext) override;

	private:
		std::shared_ptr<MeshAsset> m_mesh;
	};
}
