/*! \file ImGuiForwarder.hpp
    \author Leonidas Gonzalez
    \date 2024-03-16
    \brief Sets up the renderer specific ImGui implementation for any UI component
*/

#pragma once
#include "rendering/Renderer.hpp"

namespace Hush
{
    class IImGuiForwarder
    {
      public:
        IImGuiForwarder() = default;

        virtual ~IImGuiForwarder() = default;

        IImGuiForwarder(const IImGuiForwarder &) = default;

        IImGuiForwarder(IImGuiForwarder &&) = default;

        IImGuiForwarder &operator=(const IImGuiForwarder &) = default;

        IImGuiForwarder &operator=(IImGuiForwarder &&) = default;

        virtual void SetupImGui(IRenderer *renderer) = 0;
        
        virtual void NewFrame() = 0;
    };
} // namespace Hush