#include "EditorCamera.hpp"
#include <glm/gtx/transform.hpp>
#include "InputManager.hpp"


Hush::EditorCamera::EditorCamera(float degFov, float width, float height, float nearP, float farP) : Camera(degFov, width, height, nearP, farP)
{
	this->m_position = glm::vec3(0.f, 0.f, 5.f);
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
}

glm::mat4 Hush::EditorCamera::GetViewMatrix() const noexcept
{
	return glm::inverse(glm::translate(this->m_position));
}

