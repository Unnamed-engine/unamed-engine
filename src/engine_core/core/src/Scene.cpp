/*! \file Scene.cpp
    \author Alan Ramirez
    \date 2025-01-20
    \brief Scene implementation
*/

#include "Scene.hpp"

// TODO: implement parallel for for systems.

Hush::Scene::~Scene() = default;

void Hush::Scene::Init()
{
    // Init all systems
    for (const std::vector<ISystem *> &systemBucket : m_systems)
    {
        for (ISystem *system : systemBucket)
        {
            system->Init();
        }
    }
}

void Hush::Scene::Update(float delta)
{
    for (const std::vector<ISystem *> &systemBucket : m_systems)
    {
        for (ISystem *system : systemBucket)
        {
            system->OnUpdate(delta);
        }
    }
}

void Hush::Scene::FixedUpdate(float delta)
{
    for (const std::vector<ISystem *> &systemBucket : m_systems)
    {
        for (ISystem *system : systemBucket)
        {
            system->OnFixedUpdate(delta);
        }
    }
}

void Hush::Scene::PreRender()
{
    for (const std::vector<ISystem *> &systemBucket : m_systems)
    {
        for (ISystem *system : systemBucket)
        {
            system->OnPreRender();
        }
    }
}
void Hush::Scene::Render()
{
    for (const std::vector<ISystem *> &systemBucket : m_systems)
    {
        for (ISystem *system : systemBucket)
        {
            system->OnRender();
        }
    }
}

void Hush::Scene::PostRender()
{
    for (const std::vector<ISystem *> &systemBucket : m_systems)
    {
        for (ISystem *system : systemBucket)
        {
            system->OnPostRender();
        }
    }
}

void Hush::Scene::Shutdown()
{
    for (const std::vector<ISystem *> &systemBucket : m_systems)
    {
        for (ISystem *system : systemBucket)
        {
            system->OnShutdown();
        }
    }
}

void Hush::Scene::RemoveSystem(std::string_view name)
{
    // Find the system
    auto it = std::ranges::find_if(
        m_userSystems, [name](const std::unique_ptr<ISystem> &system) { return system->GetName() == name; });

    // If the system was found, remove it
    if (it != m_userSystems.end())
    {
        m_userSystems.erase(it);
    }

    // Sort the systems again
    SortSystems();
}

void Hush::Scene::AddEngineSystem(ISystem *system)
{
    m_engineSystems.push_back(system);
}

void Hush::Scene::SortSystems()
{
    // First, clear the buckets
    for (std::vector<ISystem *> &bucket : m_systems)
    {
        bucket.clear();
    }

    // Then, add the engine systems
    for (ISystem *system : m_engineSystems)
    {
        m_systems[system->Order()].push_back(system);
    }

    // Finally, add the user systems
    for (std::unique_ptr<ISystem> &system : m_userSystems)
    {
        m_systems[system->Order()].push_back(system.get());
    }
}