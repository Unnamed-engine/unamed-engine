#include "HushEngine.hpp"
// #include <editor/UI.hpp>
#include "ApplicationLoader.hpp"
#include <WindowManager.hpp>
#include <imgui/imgui.h>
#include <spdlog/details/os-inl.h>

Hush::HushEngine::~HushEngine()
{
    this->Quit();
}

void Hush::HushEngine::Run()
{
    this->m_app = LoadApplication(this);

    this->m_isApplicationRunning = true;
    WindowRenderer mainRenderer(m_app->GetAppName().data());
    IRenderer *rendererImpl = mainRenderer.GetInternalRenderer();

    // Initialize any static resources we need
    this->Init();

    std::chrono::steady_clock::duration elapsed;

    while (this->m_isApplicationRunning)
    {
        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
        mainRenderer.HandleEvents(&this->m_isApplicationRunning);
        // TODO: Change this to the window renderer
        if (!mainRenderer.IsActive())
        {
            // Arbitrary sleep to avoid taking all CPU usage
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        const float deltaTime = std::chrono::duration<float>(elapsed).count();

        this->m_app->Update(deltaTime);

        this->m_app->OnPreRender();

        rendererImpl->NewUIFrame();

        this->m_app->OnRender();

        rendererImpl->Draw(deltaTime);

        this->m_app->OnPostRender();

        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        elapsed = end - start;
    }
}

void Hush::HushEngine::Quit()
{
    this->m_isApplicationRunning = false;
}

void Hush::HushEngine::Init()
{
    this->m_app->Init();
}
