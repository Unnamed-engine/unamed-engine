/*! \file Query.hpp
    \author Alan Ramirez
    \date 2025-02-01
    \brief Query implementation
*/

#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <traits/EntityTraits.hpp>
#include <algorithm>

namespace Hush
{
    class Scene;

    class RawQuery
    {
    public:
        static constexpr std::uint32_t MAX_COMPONENTS = 32;

        // Special case, N == MAX_COMPONENTS, which requires checking the first 0 elements.
        RawQuery(Scene *scene, std::array<std::uint64_t, MAX_COMPONENTS> components)
            : m_scene(scene)
        {
            std::ranges::copy(components, m_components.begin());

            // Find the first 0 element
            for (std::uint8_t i = 0; i < MAX_COMPONENTS; ++i)
            {
                if (m_components[i] == 0)
                {
                    m_componentCount = i;
                    break;
                }
            }
        }

        RawQuery(Scene *scene, std::span<std::uint64_t> components);

        ~RawQuery() noexcept;

        [[nodiscard]]
        void *GetComponentAt(std::size_t index);

        [[nodiscard]]
        const void *const GetComponentAt(std::size_t index) const;

        [[nodiscard]]
        std::uint8_t GetComponentCount() const noexcept
        {
            return m_componentCount;
        }

        Scene *GetScene() const noexcept
        {
            return m_scene;
        }

    private:
        std::array<std::uint64_t, MAX_COMPONENTS> m_components{};
        Scene *m_scene;
        std::uint8_t m_componentCount{};
    };

    template <typename... Components>
    class Query
    {
        using EntityId = std::uint64_t;

    public:
        using ComponentTuple = std::tuple<std::add_lvalue_reference_t<Components>...>;
        using ConstComponentTuple = std::tuple<std::add_lvalue_reference_t<const Components>...>;

        Query(Scene *scene)
            : m_rawQuery(scene, {RegisterIfNeededSlow<Components>()...})
        {
        }

        /// Query iterator. This allows using range-based for loops with queries.
        /// @tparam Components Components to query
        struct QueryIterator
        {
            QueryIterator(Query &query, std::uint8_t index)
                : query(query),
                  m_index(index)
            {
            }

            QueryIterator &operator++()
            {
                ++m_index;
                return *this;
            }

            QueryIterator operator++(int)
            {
                QueryIterator copy = *this;
                ++m_index;
                return copy;
            }

            [[nodiscard]]
            ComponentTuple operator*()
            {
                return GetComponents(std::make_index_sequence<sizeof...(Components)>{});
            }

            [[nodiscard]]
            ConstComponentTuple operator*() const
            {
                return GetComponents(std::make_index_sequence<sizeof...(Components)>{});
            }

            [[nodiscard]]
            bool operator==(const QueryIterator &other) const
            {
                return m_index == other.m_index;
            }

            [[nodiscard]]
            bool operator!=(const QueryIterator &other) const
            {
                return m_index != other.m_index;
            }

        private:
            template <std::size_t... I>
            [[nodiscard]]
            ComponentTuple GetComponents(std::index_sequence<I...>)
            {
                return {query.m_rawQuery.GetComponentAt(m_index + I)...};
            }

            Query &query;
            std::uint8_t m_index{};
        };

        QueryIterator begin()
        {
            return QueryIterator(*this, 0);
        }

        QueryIterator end()
        {
            return QueryIterator(*this, m_rawQuery.GetComponentCount());
        }

        QueryIterator begin() const
        {
            return QueryIterator(*this, 0);
        }

        QueryIterator end() const
        {
            return QueryIterator(*this, m_rawQuery.GetComponentCount());
        }

    private:
        template <typename T>
        [[nodiscard]]
        EntityId RegisterIfNeededSlow() const
        {
            // First, get the entity id, and check if the component is registered.
            auto [status, componentId] = ComponentTraits::detail::GetEntityId<T>(m_rawQuery.GetScene());
            const ComponentTraits::ComponentInfo info = ComponentTraits::GetComponentInfo<T>();

            return InternalRegisterCppComponent(status, componentId, info);
        }

        EntityId InternalRegisterCppComponent(ComponentTraits::detail::EEntityRegisterStatus registerStatus,
                                              std::uint64_t *id,
                                              const ComponentTraits::ComponentInfo &desc);

        RawQuery m_rawQuery;
    };

} // namespace Hush