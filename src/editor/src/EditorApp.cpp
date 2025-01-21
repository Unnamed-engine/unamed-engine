//
// Created by Alan5 on 22/09/2024.
//

#include "IApplication.hpp"
#include "UI.hpp"

#include <memory>

class EditorApp final : public Hush::IApplication
{
  public:
    EditorApp() : IApplication("Hush-Editor")
    {
    }

    EditorApp(const EditorApp&) = delete;
    EditorApp(EditorApp&&) = delete;
    EditorApp& operator=(const EditorApp&) = delete;
    EditorApp& operator=(EditorApp&&) = delete;

    ~EditorApp() override = default;

    void UserInit() override
    {
    }

    void UserUpdate(float delta) override
    {
        (void)delta;
    }

    void UserFixedUpdate(float delta) override
    {
        (void)delta;
    }

    void UserOnRender() override
    {
        this->userInterface.DrawPanels();
    }

    void UserOnPostRender() override
    {

    }

    void UserOnPreRender() override
    {
    }
private:
	Hush::UI userInterface;

};

extern "C" bool BundledAppExists_Internal_() // NOLINT(*-identifier-naming)
{
    return true;
}

extern "C" Hush::IApplication* BundledApp_Internal_() // NOLINT(*-identifier-naming)
{
    return new EditorApp();
}
