/*! \file Query.cpp
    \author Alan Ramirez
    \date 2025-02-01
    \brief Query implementation
*/

#include "Query.hpp"
#include <flecs.h>

Hush::RawQuery::RawQuery(Scene *scene, std::span<std::uint64_t> components)
    : m_scene(scene)
{
    std::ranges::copy(components, m_components.begin());

    ecs_query_desc_t desc = {};
    desc.cache_kind = EcsQueryCacheAll;
}