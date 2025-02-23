/*! \file Entity.hpp
    \author Alan Ramirez
    \date 2025-01-22
    \brief Scene entity
*/

#pragma once

#include "traits/EntityTraits.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace Hush
{
    class Scene;

    template <typename... Components>
    class Query;

    ///
    /// Describes an entity in the scene.
    /// An entity is something that exists in the scene. It can have components attached to it.
    /// Entities are not meant to be created directly, but through the scene.
    ///
    /// Creating components requires the component to be registered in the scene. This is done automatically when
    /// any of the component functions are called. If the component is not registered, it will be registered in the
    /// scene.
    ///
    /// For scripting, this class exposes `*ComponentRaw` functions that are used to interact with components using
    /// raw component ids. These functions are not meant to be used directly, and, while you can, they're inconvenient.
    /// The `*Component` functions are the ones that should be used.
    class Entity
    {
    public:
        explicit Entity(Scene *ownerScene, std::uint64_t entityId)
            : m_entityId(entityId),
              m_ownerScene(ownerScene)
        {
        }
        using EntityId = std::uint64_t;

        /// Entity destructor. It does not destroy the entity. For that, use `Scene::DestroyEntity`.
        ~Entity() noexcept = default;

        Entity(const Entity &) = delete;
        Entity &operator=(const Entity &) = delete;

        Entity(Entity &&other) noexcept
            : m_entityId(std::exchange(other.m_entityId, 0)),
              m_ownerScene(std::exchange(other.m_ownerScene, nullptr))
        {
        }

        Entity &operator=(Entity &&other) noexcept
        {
            if (this != &other)
            {
                m_entityId = std::exchange(other.m_entityId, 0);
                m_ownerScene = std::exchange(other.m_ownerScene, nullptr);
            }

            return *this;
        }

        /// Checks if the entity has a component of the given type.
        /// @tparam T Type of the component.
        /// @return True if the entity has the component, false otherwise.
        template <typename T>
        bool HasComponent()
        {
            EntityId componentId = RegisterIfNeededSlow<std::remove_cvref_t<T>>();

            return HasComponentRaw(componentId);
        }

        /// Add a component to the entity.
        /// If the component is already added, it will return a reference to the existing component.
        /// @tparam T Type of the component.
        /// @return Reference to the added component.
        template <typename T>
            requires std::is_default_constructible_v<T>
        std::remove_cvref_t<T> &AddComponent()
        {
            const EntityId componentId = RegisterIfNeededSlow<T>();

            return *static_cast<std::remove_cvref_t<T> *>(AddComponentRaw(componentId));
        }

        /// Emplace a component to the entity. If the component is already added, it will return a reference to the
        /// existing component.
        ///
        /// @tparam T Type of the component.
        /// @tparam Args Arguments to pass to the constructor of the component.
        /// @param args Arguments to pass to the constructor of the component.
        /// @return Reference to the added component.
        template <typename T, typename... Args>
            requires std::is_constructible_v<T, Args...>
        std::remove_cvref_t<T> &EmplaceComponent(Args &&...args)
        {
            const EntityId componentId = RegisterIfNeededSlow<std::remove_cvref_t<T>>();

            bool isNew = false;

            auto *component = EmplaceComponentRaw(componentId, isNew);

            if (!isNew)
            {
                return *static_cast<std::remove_cvref_t<T> *>(component);
            }

            return *new (component) std::remove_cvref_t<T>(std::forward<Args>(args)...);
        }

        /// Get a component from the entity.
        /// @tparam T Type of the component.
        /// @return Pointer to the component, or nullptr if the component is not found.
        template <typename T>
        std::remove_cvref_t<T> *GetComponent()
        {
            const EntityId componentId = RegisterIfNeededSlow<std::remove_cvref_t<T>>();

            return static_cast<std::remove_cvref_t<T> *>(GetComponentRaw(componentId));
        }

        /// Get a component from the entity.
        /// @tparam T Type of the component.
        /// @return Pointer to the component, or nullptr if the component is not found.
        template <typename T>
        const std::remove_cvref_t<T> *GetComponent() const
        {
            const EntityId componentId = RegisterIfNeededSlow<std::remove_cvref_t<T>>();

            return static_cast<const std::remove_cvref_t<T> *>(GetComponentRaw(componentId));
        }

        /// Remove a component from the entity.
        /// @tparam T Type of the component.
        /// @return True if the component was removed, false otherwise.
        template <typename T>
        bool RemoveComponent()
        {
            const EntityId entityId = RegisterIfNeededSlow<std::remove_cvref_t<T>>();

            return RemoveComponentRaw(entityId);
        }

        /// Register a component to the entity.
        /// @tparam T Type to register.
        template <typename T>
        void RegisterComponent() const
        {
            (void)RegisterIfNeededSlow<std::remove_cvref_t<T>>();
        }

        // Raw component functions. Mostly for internal use but also usable by bindings. They don't check if the
        // component is registered.

        /// Register a component.
        /// @param desc Component description.
        /// @return Id of the component.
        EntityId RegisterComponentRaw(const ComponentTraits::ComponentInfo &desc) const;

        /// Add a component to the entity.
        /// @param componentId Id of the component.
        /// @return Pointer to the component.
        void *AddComponentRaw(EntityId componentId);

        /// Get a component from the entity.
        /// @param componentId Id of the component.
        /// @return Pointer to the component.
        [[nodiscard]]
        void *GetComponentRaw(EntityId componentId);

        /// Get a component from the entity.
        /// @param componentId Id of the component.
        /// @return Pointer to the component.
        [[nodiscard]]
        void *GetComponentRaw(EntityId componentId) const;

        /// Check if the entity has a component.
        /// @param componentId Id of the component.
        /// @return True if the entity has the component, false otherwise.
        [[nodiscard]]
        bool HasComponentRaw(EntityId componentId);

        /// Emplace a component to the entity.
        /// If the component is already added, it will return a reference to the existing component.
        /// If the component is new, the user is in charge of constructing it. Not constructing it is undefined
        /// behavior.
        ///
        /// @param componentId Id of the component.
        /// @param isNew Flag to indicate if the component is new. If it is new, user is in charge of constructing it.
        /// @return Pointer to the component.
        [[nodiscard]]
        void *EmplaceComponentRaw(EntityId componentId, bool &isNew);

        /// Remove a component from the entity.
        /// @param componentId Id of the component.
        /// @return True if the component was removed, false otherwise.
        bool RemoveComponentRaw(EntityId componentId);

        /// Destroy an entity. This will remove all components from the entity and destroy it.
        /// This consumes the entity, so it should not be used after this function is called.
        /// @param entity Entity to destroy.
        static void Destroy(Entity &&entity);

        [[nodiscard]]
        EntityId GetId() const
        {
            return m_entityId;
        }

        /// Get the name of the entity.
        /// @return Name of the entity.
        [[nodiscard]]
        std::optional<std::string_view> GetName() const;

    private:
        friend class Scene;
        friend class Query<>;

        /// Register a component if it is not registered.
        /// @tparam T Type of the component.
        /// @return Id of the component.
        template <typename T>
        [[nodiscard]]
        EntityId RegisterIfNeededSlow() const
        {
            // First, get the entity id, and check if the component is registered.
            auto [status, componentId] = ComponentTraits::detail::GetEntityId<T>(m_ownerScene);
            const ComponentTraits::ComponentInfo info = ComponentTraits::GetComponentInfo<T>();

            return InternalRegisterCppComponent(status, componentId, info);
        }

        [[nodiscard]]
        void *GetSceneWorld() const;

        /// Check if a component is registered.
        /// @param componentId Id of the component.
        /// @return True if the component is registered, false otherwise.
        bool IsComponentRegistered(EntityId componentId) const;

        /// Register a C++ component. C++ components are special because they use a cache in the scene to
        /// avoid registering the same component multiple times.
        /// @param desc Component description.
        /// @return ID of the component.
        [[nodiscard]]
        EntityId InternalRegisterCppComponent(ComponentTraits::detail::EEntityRegisterStatus registerStatus,
                                              std::uint64_t *id,
                                              const ComponentTraits::ComponentInfo &desc) const;

        /// Get the id of a component from the cache.
        /// @param name Name of the component.
        /// @return Id of the component, or std::nullopt if the component is not found.
        [[nodiscard]]
        std::optional<EntityId> InternalCachedComponentId(std::string_view name) const;

        /// Id of the entity
        EntityId m_entityId{};

        /// Scene that owns this entity
        Scene *m_ownerScene;
    };

} // namespace Hush