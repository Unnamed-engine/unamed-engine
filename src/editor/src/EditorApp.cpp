//
// Created by Alan5 on 22/09/2024.
//

#include "IApplication.hpp"
#include "UI.hpp"

#include <memory>
#include "VirtualFilesystem.hpp"
#include "filesystem/CFileSystem/CFileSystem.hpp"

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

    void Init() override
    {
		Hush::VirtualFilesystem::s_instance = new Hush::VirtualFilesystem();
        std::filesystem::path absPath = std::filesystem::absolute(".");
        Hush::VirtualFilesystem::s_instance->MountFileSystem<Hush::CFileSystem>("res://", absPath.string());
    }

    void Update() override
    {

    }

    void OnRender() override
    {
        this->userInterface.DrawPanels();
    }

    void OnPostRender() override
    {

    }

    void OnPreRender() override
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
