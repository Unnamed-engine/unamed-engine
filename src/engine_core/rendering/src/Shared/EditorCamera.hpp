#pragma once
#include "Camera.hpp"

namespace Hush {

	class EditorCamera final : public Camera {
	public: 
		EditorCamera() = default;
		EditorCamera(const float degFov, const float width, const float height, const float nearP, const float farP);
		
		void OnUpdate(float delta);

		glm::mat4 GetOrientationMatrix() const noexcept;

		glm::mat4 GetViewMatrix() noexcept;

		[[nodiscard]] glm::vec3 GetPosition() const noexcept;

		void SetPosition(const glm::vec3& position);

	private:

		float m_pitch, m_yaw;
		float m_pitchDelta, m_yawDelta;
		glm::vec3 m_position;
		bool m_isEditorFocused;
	};
}
