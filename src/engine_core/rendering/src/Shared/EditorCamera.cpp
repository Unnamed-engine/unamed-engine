#include "EditorCamera.hpp"
#include "InputManager.hpp"
#include "Logger.hpp"
#include <glm/gtx/quaternion.hpp>


constexpr glm::vec3 VEC_UP = { 0.f, 1.f, 0.f };
constexpr glm::vec3 VEC_FORWARD = { 0.f, 0.f, -1.f };

Hush::EditorCamera::EditorCamera(const float degFov, const float width, const float height, const float nearP, const float farP) 
	: Camera(degFov, width, height, nearP, farP)
{
	constexpr glm::vec3 initialPosition = { 0, 0, 5 };
	this->m_yaw = 0.0f;
	this->m_pitch = 0.0f;
	this->m_position = initialPosition;
}

void Hush::EditorCamera::OnUpdate(float delta)
{
	(void)delta;
	
	/*
	glm::mat4 viewMatrix = this->GetViewMatrix();
	glm::vec3 right = glm::vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);
	glm::vec3 up = glm::vec3(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]);
	glm::vec3 forward = -glm::vec3(viewMatrix[0][2], viewMatrix[1][2], viewMatrix[2][2]);

	glm::vec3 camPos = this->GetPosition();
	float speed = 0.01f; // Adjust speed as necessary

	if (InputManager::IsKeyDown(EKeyCode::W)) {
		camPos += forward * speed;
	}
	if (InputManager::IsKeyDown(EKeyCode::S)) {
		camPos -= forward * speed;
	}
	if (InputManager::IsKeyDown(EKeyCode::A)) {
		camPos -= right * speed;
	}
	if (InputManager::IsKeyDown(EKeyCode::D)) {
		camPos += right * speed;
	}
	if (InputManager::IsKeyDown(EKeyCode::Q)) {
		camPos += up * speed;
	}
	if (InputManager::IsKeyDown(EKeyCode::E)) {
		camPos -= up * speed;
	}


	this->SetPosition(camPos);
	
	if (!InputManager::GetMouseButtonPressed(EMouseButton::Right)) {
		return;
	}
	*/

	glm::vec2 mouseAcceleration = InputManager::GetMouseAcceleration();
	if (mouseAcceleration != glm::vec2{ 0.0f }) {
		this->m_yaw += mouseAcceleration.x / 200.0f;
		this->m_pitch += mouseAcceleration.y / 200.0f;
	}
	//Check for editor use
	if (!this->m_isEditorFocused) return;
}

glm::mat4 Hush::EditorCamera::GetOrientationMatrix() const noexcept
{
	glm::quat pitchRotation = glm::angleAxis(this->m_pitch, glm::vec3{ -1.f, 0.f, 0.f });
	glm::quat yawRotation = glm::angleAxis(this->m_yaw, glm::vec3{ 0.f, -1.f, 0.f });

	return glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);
}

glm::mat4 Hush::EditorCamera::GetViewMatrix() noexcept
{
	glm::mat4 cameraTranslation = glm::translate(glm::mat4(1.f), this->m_position);
	glm::mat4 cameraRotation = this->GetOrientationMatrix();
	glm::mat4 viewMatrix = cameraTranslation * cameraRotation;
	return glm::inverse(viewMatrix);
}

glm::vec3 Hush::EditorCamera::GetPosition() const noexcept
{
	return this->m_position;
}

void Hush::EditorCamera::SetPosition(const glm::vec3& position)
{
	this->m_position = position;
}
