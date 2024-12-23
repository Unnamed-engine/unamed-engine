#pragma once

#include <glm/mat4x4.hpp>
#include <vector>
#include <memory>
#include "IRenderable.hpp"
namespace Hush {
	/// @brief Common renderable node for scenes with multiple children to render
	/// the Draw function *MUST* be called (recursed down) for every implementation
	/// PENDING: Unite this into one renderableNode implementation IF AND ONLY IF this becomes a bottleneck
	class RenderableNode : public IRenderable {

	public:
		void RefreshTransform(const glm::mat4& parentMatrix) {
			this->m_worldTransform = parentMatrix * this->m_localTransform;
			for (std::shared_ptr<RenderableNode>& child : this->m_children) {
				child->RefreshTransform(this->m_worldTransform);
			}
		}
	
		void Draw(const glm::mat4& topMatrix, void* drawContext) override {
			for (std::shared_ptr<RenderableNode>& child : this->m_children) {
				child->Draw(topMatrix, drawContext);
			}
		}

	protected:
		std::weak_ptr<RenderableNode> m_parent;
		std::vector<std::shared_ptr<RenderableNode>> m_children;

		glm::mat4 m_localTransform;
		glm::mat4 m_worldTransform;
	};
}
