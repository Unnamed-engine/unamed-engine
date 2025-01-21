/*! \file IApplication.hpp
    \author Alan Ramirez
    \date 2024-09-21
    \brief Application meant to be run by Hush
*/

#pragma once

#include "Scene.hpp"
#include <string>
#include <string_view>

namespace Hush
{
    class IApplication
    {
    public:
        IApplication(std::string_view appName)
            : m_appName(appName)
        {
        }

        IApplication(const IApplication &) = delete;
        IApplication(IApplication &&) = delete;
        IApplication &operator=(const IApplication &) = default;
        IApplication &operator=(IApplication &&) = default;

        virtual ~IApplication() = default;

        void Init();

        void Update(float delta);

        void FixedUpdate(float delta);

        void OnPreRender();

        void OnRender();

        void OnPostRender();

        [[nodiscard]]
        std::string GetAppName() const
        {
            return m_appName;
        }

    protected:
        virtual void UserInit() = 0;

        virtual void UserUpdate(float delta) = 0;

        virtual void UserFixedUpdate(float delta) = 0;

        virtual void UserOnPreRender() = 0;

        virtual void UserOnRender() = 0;

        virtual void UserOnPostRender() = 0;

    private:
        std::string m_appName;
        std::unique_ptr<Scene> m_scene;
    };
} // namespace Hush
