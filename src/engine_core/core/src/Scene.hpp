/*! \file Scene.hpp
    \author Alan Ramirez
    \date 2025-01-20
    \brief Scene implementation
*/

#pragma once

#include "ISystem.hpp"

#include "flecs.h"
#include <array>
#include <memory>
#include <vector>

namespace Hush
{
    class Engine;

    // TODO: this class is expected to change a lot, it's just a placeholder for now.
    // The API is not ready and I would like to think about implementing it considering scripting in the future and bindings.
    class Scene
    {
        constexpr static std::uint16_t ORDER_BUCKET_SIZE = ISystem::MAX_ORDER + 1;

    public:
        using EntityId = ecs_entity_t;

        /// Constructor.
        /// @param engine Game engine
        Scene(Engine &engine);

        ~Scene();

        /// Init is called when the scene is initialized.
        void Init();

        /// Update is called every frame.
        void Update(float delta);

        /// FixedUpdate is called every fixed frame.
        void FixedUpdate(float delta);

        /// PreRender is called before rendering.
        void PreRender();

        /// Render is called when the scene should render.
        void Render();

        /// PostRender is called after rendering.
        void PostRender();

        /// Shutdown is called when the scene is shutting down.
        void Shutdown();

        ///
        /// @tparam S Add a system to the scene
        template <typename S>
            requires std::derived_from<S, ISystem>
        void AddSystem()
        {
            m_userSystems.push_back(std::make_unique<S>());
        }

        /// Remove a system from the scene by name.
        /// @param name Name of the system to remove.
        void RemoveSystem(std::string_view name);

        EntityId CreateEntity()
        {
            return m_world.entity();
        }

        template <typename C>
        void AddComponent(EntityId entity, C &&component)
        {
            m_world.component<C>();

            m_world.set(entity, std::forward<C>(component));
        }

        template <typename C>
        void RemoveComponent(EntityId entity)
        {
            m_world.remove<C>(entity);
        }

        void DeleteEntity(EntityId entity)
        {
            m_world.delete_with(entity);
        }

    private:
        /// Add an engine system to the scene
        /// @param system System to add
        void AddEngineSystem(ISystem *system);

        /// Sort the systems based on their order and store them in the buckets
        void SortSystems();

        /// Ordered array of systems
        /// This is not the most efficient way to store the systems btw.
        std::array<std::vector<ISystem *>, ORDER_BUCKET_SIZE> m_systems;

        /// Special vector to store the systems that come from the engine.
        std::vector<ISystem *> m_engineSystems;

        /// User systems
        std::vector<std::unique_ptr<ISystem>> m_userSystems;

        flecs::world m_world;
    };
} // namespace Hush