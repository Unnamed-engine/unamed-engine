#pragma once
#include "Camera.hpp"

namespace Hush {
	class EditorCamera final : public Camera {
	public: 
		EditorCamera() = default;

		EditorCamera(float degFov, float width, float height, float nearP, float farP);

		void OnUpdate();

		glm::mat4 GetViewMatrix() const noexcept;

		glm::mat4 GetOrientationMatrix() const noexcept;

	private:
		glm::vec3 m_position;
		float m_yaw, m_pitch;
	};
}
