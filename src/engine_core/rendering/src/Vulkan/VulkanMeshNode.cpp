#include "VulkanMeshNode.hpp"
#include "VkRenderObject.hpp"
#include "VulkanLoader.hpp"
#include "Assertions.hpp"

void Hush::VulkanMeshNode::Draw(const glm::mat4& topMatrix, void* drawContext)
{
	//Interpret drawContext as: std::vector<VkRenderObject>* OpaqueSurfaces;
	HUSH_ASSERT(drawContext != nullptr, "Draw context should not be null for any render node");
	auto* opaqueSurfaces = static_cast<std::vector<VkRenderObject>*>(drawContext);

	glm::mat4 nodeMatrix = topMatrix * this->m_worldTransform;

	for (GeoSurface& s : this->m_mesh->surfaces) {
		VkRenderObject def{};
		def.indexCount = s.count;
		def.firstIndex = s.startIndex;
		def.indexBuffer = this->m_mesh->meshBuffers.indexBuffer.GetBuffer();
		def.material = s.material.get();

		def.transform = nodeMatrix;
		def.vertexBufferAddress = this->m_mesh->meshBuffers.vertexBufferAddress;

		opaqueSurfaces->push_back(def);
	}


	RenderableNode::Draw(topMatrix, drawContext);
}
