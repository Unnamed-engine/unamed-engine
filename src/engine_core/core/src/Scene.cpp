/*! \file Scene.cpp
    \author Alan Ramirez
    \date 2025-01-20
    \brief Scene implementation
*/

#include "Scene.hpp"

#define FLECS_NO_CPP
#include <flecs.h>

constexpr std::size_t DEFAULT_SYSTEMS_CAPACITY = 128;

Hush::Scene::Scene(Engine *engine)
    : m_engine(engine),
      m_world(ecs_init())
{
    // Reserve the buckets
    m_userSystems.reserve(DEFAULT_SYSTEMS_CAPACITY);
}

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

Hush::Entity Hush::Scene::CreateEntity()
{
    auto *world = static_cast<ecs_world_t *>(m_world);

    const Entity::EntityId entityId = ecs_new(world);

    return Entity{this, entityId};
}

Hush::Entity Hush::Scene::CreateEntityWihName(std::string_view name)
{
    auto *world = static_cast<ecs_world_t *>(m_world);

    const ecs_entity_desc_t desc = {
        .name = name.data(),
    };

    const Entity::EntityId entityId = ecs_entity_init(world, &desc);

    return Entity{this, entityId};
}

void Hush::Scene::DestroyEntity(Entity &&entity)
{
    auto *world = static_cast<ecs_world_t *>(m_world);

    ecs_delete(world, entity.GetId());
}

std::optional<std::uint64_t> Hush::Scene::GetRegisteredComponentId(std::string_view name)
{
    std::shared_lock lock(m_registeredEntitiesMutex);
    const auto entityIt = m_registeredEntities.find(name.data());

    if (entityIt != m_registeredEntities.end())
    {
        return entityIt->second;
    }

    // Component not found.
    return std::nullopt;
}

void Hush::Scene::RegisterComponentId(std::string_view name, Entity::EntityId id)
{
    std::unique_lock lock(m_registeredEntitiesMutex);
    m_registeredEntities.insert_or_assign(name.data(), id);
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