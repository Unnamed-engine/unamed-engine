#pragma once

#include "IEditorPanel.hpp"
#include <glm/vec3.hpp>

namespace Hush {
	class DebugTooltip final : public IEditorPanel {
	public:
		static inline DebugTooltip* s_debugTooltip = nullptr;
		void OnRender() noexcept override;
		
		float degFov = 40.0f;
	};

}
