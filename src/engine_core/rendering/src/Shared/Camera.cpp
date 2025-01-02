#include "Camera.hpp"

Hush::Camera::Camera(const glm::mat4 &projectionMat, const glm::mat4 &unreversedProjectionMat) noexcept
    : m_projectionMatrix(projectionMat), m_unreversedProjectionMatrix(unreversedProjectionMat)
{
}

Hush::Camera::Camera(float degFov, float width, float height, float nearP, float farP) noexcept {
    this->m_verticalFOV = degFov;
    this->m_aspectRatio = width / height;
    this->m_nearClip = nearP;
    this->m_farClip = farP;
    this->m_width = width;
    this->m_height = height;
    //Construct our perspective matrix
    this->RefreshMatrix();
}

const glm::mat4 &Hush::Camera::GetProjectionMatrix() const noexcept
{
    return this->m_projectionMatrix;
}

const glm::mat4 &Hush::Camera::GetUnreversedProjectionMatrix() const noexcept
{
    return this->m_unreversedProjectionMatrix;
}

void Hush::Camera::SetProjectionMatrix(const glm::mat4 projection, const glm::mat4 unReversedProjection)
{
    this->m_projectionMatrix = projection;
    this->m_unreversedProjectionMatrix = unReversedProjection;
}

void Hush::Camera::SetPerspectiveProjectionMatrix(const float radFov, const float width, const float height,
                                                  const float nearP, const float farP)
{
    //Yes, even though the last two parameters seem to be reversed, this is how other engines seem to be doing it
    //but, you know, adding this for future bugs and stuff
    this->m_projectionMatrix = glm::perspectiveFov(radFov, width, height, farP, nearP);
    this->m_unreversedProjectionMatrix = glm::perspectiveFov(radFov, width, height, nearP, farP);
}

void Hush::Camera::SetVerticalFOV(float degFov)
{
    this->m_verticalFOV = degFov;
    this->RefreshMatrix();
}

float Hush::Camera::GetVerticalFOV() const noexcept
{
    return this->m_verticalFOV;
}

void Hush::Camera::RefreshMatrix()
{
    this->m_projectionMatrix = glm::perspective(glm::radians(this->m_verticalFOV), this->m_aspectRatio, this->m_nearClip, this->m_farClip);
}
