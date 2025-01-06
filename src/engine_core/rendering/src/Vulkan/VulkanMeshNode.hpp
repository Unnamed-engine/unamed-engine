#pragma once
#include "Shared/RenderableNode.hpp"

namespace Hush {
	struct MeshAsset;

	class VulkanMeshNode : public RenderableNode {

	public:
		VulkanMeshNode(std::shared_ptr<MeshAsset> mesh);

		void Draw(const glm::mat4& topMatrix, void* drawContext) override;

		MeshAsset& GetMesh();

	private:
		std::shared_ptr<MeshAsset> m_mesh;
	};
}
