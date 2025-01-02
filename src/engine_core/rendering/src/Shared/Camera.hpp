/*! \file Camera.hpp
    \author Kyn21kx
    \date 2024-05-30
    \brief Camera descriptor class for both scene and editor rendering
*/

#pragma once

#include <glm/gtx/transform.hpp>

namespace Hush
{
    class Camera
    {
      public:
        Camera() = default;
        Camera(const glm::mat4 &projectionMat, const glm::mat4 &unreversedProjectionMat) noexcept;
        Camera(float degFov, float width, float height, float nearP, float farP) noexcept;
        virtual ~Camera() = default;

        [[nodiscard]] const glm::mat4 &GetProjectionMatrix() const noexcept;

        [[nodiscard]] const glm::mat4 &GetUnreversedProjectionMatrix() const noexcept;

        void SetProjectionMatrix(glm::mat4 projection, glm::mat4 unReversedProjection);

        void SetPerspectiveProjectionMatrix(const float radFov, const float width, const float height,
                                            const float nearP, const float farP);

        void SetVerticalFOV(float degFov);

        float GetVerticalFOV() const noexcept;

      protected:
		float m_verticalFOV, m_aspectRatio, m_nearClip, m_farClip, m_width, m_height;
		// NOLINTNEXTLINE
        float m_exposure = 0.8f; //Aribtrary value (inspired from the Hazel Engine)
        glm::mat4 m_projectionMatrix = glm::mat4(1.0f);
        // Currently only needed for shadow maps and ImGuizmo
        glm::mat4 m_unreversedProjectionMatrix = glm::mat4(1.0f);

      private:
          void RefreshMatrix();
    };
}