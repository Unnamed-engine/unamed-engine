#include "EditorCamera.hpp"
#include <glm/gtx/transform.hpp>
#include "InputManager.hpp"
#include <glm/gtx/quaternion.hpp>


Hush::EditorCamera::EditorCamera(float degFov, float width, float height, float nearP, float farP) : Camera(degFov, width, height, nearP, farP)
{
	this->m_position = glm::vec3(0.f, 0.f, 5.f);
	this->m_yaw = 0.0f;
	this->m_pitch = 0.0f;
}

void Hush::EditorCamera::OnUpdate()
{
	if (InputManager::IsKeyDown(EKeyCode::W)) {
		this->m_position.z -= 0.01f;
	}
	if (InputManager::IsKeyDown(EKeyCode::S)) {
		this->m_position.z += 0.01f;
	}

	if (InputManager::IsKeyDown(EKeyCode::A)) {
		this->m_position.x -= 0.01f;
	}
	if (InputManager::IsKeyDown(EKeyCode::D)) {
		this->m_position.x += 0.01f;
	}

	if (InputManager::IsKeyDown(EKeyCode::Q)) {
		this->m_position.y -= 0.01f;
	}
	if (InputManager::IsKeyDown(EKeyCode::E)) {
		this->m_position.y += 0.01f;
	}

	glm::vec2 mouseAcceleration = InputManager::GetMouseAcceleration();
	if (mouseAcceleration != glm::vec2{ 0.0f }) {
		this->m_yaw += mouseAcceleration.x / 200.0f;
		this->m_pitch += mouseAcceleration.y / 200.0f;
	}

}

glm::mat4 Hush::EditorCamera::GetOrientationMatrix() const noexcept
{
	glm::quat pitchRotation = glm::angleAxis(this->m_pitch, glm::vec3{ -1.f, 0.f, 0.f });
	glm::quat yawRotation = glm::angleAxis(this->m_yaw, glm::vec3{ 0.f, -1.f, 0.f });

	return glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);
}

glm::mat4 Hush::EditorCamera::GetViewMatrix() const noexcept
{
	glm::mat4 cameraTranslation = glm::translate(glm::mat4(1.f), this->m_position);
	glm::mat4 cameraRotation = this->GetOrientationMatrix();
	glm::mat4 viewMatrix = cameraTranslation * cameraRotation;
	return glm::inverse(viewMatrix);
}

