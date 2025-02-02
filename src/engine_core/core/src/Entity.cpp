/*! \file Entity.cpp
    \author Alan Ramirez
    \date 2025-01-22
    \brief Scene entity
*/
#include "Entity.hpp"
#include "Scene.hpp"

#include <flecs.h>

Hush::Entity::EntityId Hush::Entity::RegisterComponentRaw(const ComponentTraits::ComponentInfo &desc) const
{
    struct ComponentInfo
    {
        std::size_t size{};
        std::size_t alignment{};
        std::string name;
        ComponentTraits::ComponentOps ops{};

        void *userCtx{};
        void (*userCtxFree)(void *){};
    };

    auto *componentInfo = new ComponentInfo();
    componentInfo->size = desc.size;
    componentInfo->alignment = desc.alignment;
    componentInfo->name = desc.name;
    componentInfo->ops = desc.ops;
    componentInfo->userCtx = desc.userCtx;
    componentInfo->userCtxFree = desc.userCtxFree;

    ecs_component_desc_t componentDesc = {};
    componentDesc.type.alignment = componentInfo->alignment;
    componentDesc.type.size = componentInfo->size;
    componentDesc.type.name = componentInfo->name.data();
    componentDesc.type.hooks.binding_ctx = componentInfo;
    componentDesc.type.hooks.binding_ctx_free = [](void *ctx) {
        auto *info = static_cast<ComponentInfo *>(ctx);
        if (info->userCtxFree != nullptr)
        {
            info->userCtxFree(info->userCtx);
        }
        delete info;
    };

    if (desc.ops.ctor != nullptr)
    {
        componentDesc.type.hooks.ctor = [](void *ptr, int32_t count, const ecs_type_info_t *type_info) {
            auto *info = static_cast<ComponentInfo *>(type_info->hooks.binding_ctx);

            ComponentTraits::ComponentInfo componentDesc = {
                .size = info->size,
                .alignment = info->alignment,
                .name = info->name,
                .ops = info->ops,
                .userCtx = info->userCtx,
                .userCtxFree = info->userCtxFree,
            };

            info->ops.ctor(ptr, count, componentDesc);
        };
    }

    if (desc.ops.dtor != nullptr)
    {
        componentDesc.type.hooks.dtor = [](void *ptr, int32_t count, const ecs_type_info_t *type_info) {
            auto *info = static_cast<ComponentInfo *>(type_info->hooks.binding_ctx);

            ComponentTraits::ComponentInfo componentDesc = {
                .size = info->size,
                .alignment = info->alignment,
                .name = info->name,
                .ops = info->ops,
                .userCtx = info->userCtx,
                .userCtxFree = info->userCtxFree,
            };

            info->ops.dtor(ptr, count, componentDesc);
        };
    }

    if (desc.ops.copy != nullptr)
    {
        componentDesc.type.hooks.copy = [](void *dst, const void *src, int32_t count,
                                           const ecs_type_info_t *type_info) {
            auto *info = static_cast<ComponentInfo *>(type_info->hooks.binding_ctx);

            ComponentTraits::ComponentInfo componentDesc = {
                .size = info->size,
                .alignment = info->alignment,
                .name = info->name,
                .ops = info->ops,
                .userCtx = info->userCtx,
                .userCtxFree = info->userCtxFree,
            };

            info->ops.copy(dst, src, count, componentDesc);
        };
    }

    if (desc.ops.move != nullptr)
    {
        componentDesc.type.hooks.move = [](void *dst, void *src, int32_t count, const ecs_type_info_t *type_info) {
            auto *info = static_cast<ComponentInfo *>(type_info->hooks.binding_ctx);

            ComponentTraits::ComponentInfo componentDesc = {
                .size = info->size,
                .alignment = info->alignment,
                .name = info->name,
                .ops = info->ops,
                .userCtx = info->userCtx,
                .userCtxFree = info->userCtxFree,
            };

            info->ops.move(dst, src, count, componentDesc);
        };
    }

    if (desc.ops.copyCtor != nullptr)
    {
        componentDesc.type.hooks.copy_ctor = [](void *dst, const void *src, int32_t count,
                                                const ecs_type_info_t *type_info) {
            auto *info = static_cast<ComponentInfo *>(type_info->hooks.binding_ctx);

            ComponentTraits::ComponentInfo componentDesc = {
                .size = info->size,
                .alignment = info->alignment,
                .name = info->name,
                .ops = info->ops,
                .userCtx = info->userCtx,
                .userCtxFree = info->userCtxFree,
            };

            info->ops.copyCtor(dst, src, count, componentDesc);
        };
    }

    if (desc.ops.moveCtor != nullptr)
    {
        componentDesc.type.hooks.move_ctor = [](void *dst, void *src, int32_t count, const ecs_type_info_t *type_info) {
            auto *info = static_cast<ComponentInfo *>(type_info->hooks.binding_ctx);

            ComponentTraits::ComponentInfo componentDesc = {
                .size = info->size,
                .alignment = info->alignment,
                .name = info->name,
                .ops = info->ops,
                .userCtx = info->userCtx,
                .userCtxFree = info->userCtxFree,
            };

            info->ops.moveCtor(dst, src, count, componentDesc);
        };
    }

    // Register the component
    auto *world = static_cast<ecs_world_t *>(m_ownerScene->GetWorld());
    ecs_entity_t componentId = ecs_component_init(world, &componentDesc);

    return componentId;
}

void *Hush::Entity::AddComponentRaw(EntityId componentId)
{
    auto *world = static_cast<ecs_world_t *>(m_ownerScene->GetWorld());

    ecs_add_id(world, m_entityId, componentId);

    return ecs_get_mut_id(world, m_entityId, componentId);
}

void *Hush::Entity::GetComponentRaw(EntityId componentId)
{
    auto *world = static_cast<ecs_world_t *>(m_ownerScene->GetWorld());

    return ecs_get_mut_id(world, m_entityId, componentId);
}

void *Hush::Entity::GetComponentRaw(EntityId componentId) const
{
    auto *world = static_cast<ecs_world_t *>(m_ownerScene->GetWorld());

    return ecs_get_mut_id(world, m_entityId, componentId);
}

bool Hush::Entity::HasComponentRaw(EntityId componentId)
{
    auto *world = static_cast<ecs_world_t *>(m_ownerScene->GetWorld());

    return ecs_has_id(world, m_entityId, componentId);
}

void *Hush::Entity::EmplaceComponentRaw(EntityId componentId, bool &is_new)
{
    auto *world = static_cast<ecs_world_t *>(m_ownerScene->GetWorld());

    void *component = ecs_emplace_id(world, m_entityId, componentId, &is_new);

    return component;
}

bool Hush::Entity::RemoveComponentRaw(EntityId componentId)
{
    auto world = static_cast<ecs_world_t *>(m_ownerScene->GetWorld());

    if (ecs_has_id(world, m_entityId, componentId))
    {
        ecs_remove_id(world, m_entityId, componentId);
        return true;
    }

    return false;
}

void Hush::Entity::Destroy(Entity &&entity)
{
    auto *world = static_cast<ecs_world_t *>(entity.GetSceneWorld());
    ecs_delete(world, entity.m_entityId);
}

void *Hush::Entity::GetSceneWorld() const
{
    return m_ownerScene->GetWorld();
}

bool Hush::Entity::IsComponentRegistered(EntityId componentId)
{
    return ecs_has_id(static_cast<ecs_world_t *>(m_ownerScene->GetWorld()), m_entityId, componentId);
}

Hush::Entity::EntityId Hush::Entity::InternalRegisterCppComponent(const ComponentTraits::ComponentInfo &desc) const
{
    const EntityId componentId = RegisterComponentRaw(desc);

    // Register the component id in the scene
    m_ownerScene->RegisterComponentId(desc.name, componentId);

    return componentId;
}

std::optional<Hush::Entity::EntityId> Hush::Entity::InternalCachedComponentId(const std::string_view name) const
{
    return m_ownerScene->GetRegisteredComponentId(name);
}
