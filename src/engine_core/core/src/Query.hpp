/*! \file Query.hpp
    \author Alan Ramirez
    \date 2025-02-01
    \brief Query implementation
*/

#pragma once

#include <Entity.hpp>
#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <traits/EntityTraits.hpp>

namespace Hush
{
    class Scene;
    class RawQuery;

    ///
    /// Low-level query for entities.
    /// This allows getting raw void pointers to components.
    class RawQuery
    {
        RawQuery(Scene *scene, void *query);

    public:
        /// Cache mode for the query.
        enum class ECacheMode
        {
            Default,
            Auto,
            All,
            None
        };

        /// Component access mode.
        enum class EComponentAccess
        {
            ReadOnly = 0,
            WriteOnly = 1,
            ReadWrite = 2,
            Default = ReadWrite
        };

        /// Query iterator.
        struct QueryIterator
        {
            QueryIterator() = default;

            QueryIterator(const QueryIterator &) = delete;
            QueryIterator &operator=(const QueryIterator &) = delete;

            QueryIterator(QueryIterator &&rhs) noexcept;
            QueryIterator &operator=(QueryIterator &&rhs) noexcept;

            ~QueryIterator();

            /// Move to the next entity.
            /// @return True if there is a next entity, false otherwise.
            [[nodiscard]]
            bool Next();

            /// Skip the current entity.
            void Skip();

            /// Check if the iterator has been finished.
            /// @return True if the iterator has been finished. False otherwise.
            [[nodiscard]]
            bool Finished() const
            {
                return m_hasBeenDestroyed;
            }

            /// Gives the number of entities in the current table.
            /// This number is updated when using \ref Next.
            ///
            /// @return The number of entities in the query.
            [[nodiscard]]
            std::size_t Size() const;

            /// Get a component at the given index.
            /// Index is given by the order of the components in the query. For instance,
            /// if the query was creating with a list of components [Position, Velocity], then
            /// Position would be at index 0 and Velocity at index 1.
            ///
            /// @param index Index of the component.
            /// @param size Size of the component.
            /// @return Pointer to the component.
            [[nodiscard]]
            void *const GetComponentAt(std::int8_t index, std::size_t size) const;

            /// Get the entity id at the given index.
            /// @param index Index of the entity. This must be in range [0, Size()).
            /// @return Entity id at the given index.
            [[nodiscard]]
            std::uint64_t GetEntityAt(std::size_t index) const;

        private:
            friend class RawQuery;

            /// Size of the ecs_query_iter_t struct.
            static constexpr std::size_t ECS_ITER_SIZE = 384;
            /// Alignment of the ecs_query_iter_t struct.
            static constexpr std::size_t ECS_ITER_ALIGNMENT = 8;

            alignas(ECS_ITER_ALIGNMENT) std::array<std::byte, ECS_ITER_SIZE> m_iterData{};
            bool m_hasBeenDestroyed = false;

            bool m_hasData = false;
        };

        static constexpr std::uint32_t MAX_COMPONENTS = 32;

        RawQuery() = delete;
        RawQuery(const RawQuery &) = delete;
        RawQuery &operator=(const RawQuery &) = delete;

        /// Move constructor.
        /// @param rhs Other query to move from.
        RawQuery(RawQuery &&rhs) noexcept;

        /// Move assignment operator.
        /// @param rhs Other query to move from.
        /// @return self
        RawQuery &operator=(RawQuery &&rhs) noexcept;

        ~RawQuery() noexcept;

        /// Get the scene where the query is running.
        /// @return Scene where the query is running.
        [[nodiscard]]
        Scene *GetScene() const noexcept
        {
            return m_scene;
        }

        QueryIterator GetIterator();

    private:
        friend class Scene;

        void *m_query;
        Scene *m_scene;
    };

    namespace impl
    {
        class QueryImpl
        {
            using EntityId = std::uint64_t;

        public:
            QueryImpl(RawQuery query) noexcept
                : m_rawQuery(std::move(query))
            {
            }

        protected:
            EntityId InternalRegisterCppComponent(ComponentTraits::detail::EEntityRegisterStatus registerStatus,
                                                  std::uint64_t *id,
                                                  const ComponentTraits::ComponentInfo &desc);

            RawQuery m_rawQuery;
        };
    } // namespace impl

    /// Query for entities. This wraps the raw query and allows getting components in a type-safe way.
    ///
    /// @tparam Components Components to query.
    template <typename... Components>
    class Query : public impl::QueryImpl
    {
        using EntityId = std::uint64_t;

    public:
        using ECacheMode = RawQuery::ECacheMode;

        using ComponentTuple = std::tuple<std::span<std::remove_reference_t<Components>>...>;
        using ConstComponentTuple = std::tuple<std::span<std::add_const_t<std::remove_reference_t<Components>>>...>;

        /// Constructor.
        /// @param query Raw query.
        Query(RawQuery query) noexcept
            : QueryImpl(std::move(query))
        {
        }

        /// This is a sentinel iterator that is used to signal the end of the query.
        struct SentinelQueryIterator
        {
        };

        /// Query iterator. This allows using range-based for loops with queries.
        struct QueryIterator
        {
            QueryIterator(RawQuery::QueryIterator iter)
                : m_iter(std::move(iter))
            {
            }

            QueryIterator(const QueryIterator &) = delete;
            QueryIterator &operator=(const QueryIterator &) = delete;

            QueryIterator(QueryIterator &&rhs) noexcept
                : m_iter(std::move(rhs.m_iter)),
                  m_hasData(std::exchange(rhs.m_hasData, false))
            {
            }

            QueryIterator &operator=(QueryIterator &&rhs) noexcept
            {
                if (this != &rhs)
                {
                    m_iter = std::move(rhs.m_iter);
                    m_hasData = std::exchange(rhs.m_hasData, false);
                }

                return *this;
            }

            QueryIterator &operator++()
            {
                if (m_iter.Finished())
                {
                    return *this;
                }
                m_hasData = m_iter.Next();
                return *this;
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

            bool operator==(const SentinelQueryIterator &) const
            {
                return m_iter.Finished();
            }

            bool operator!=(const SentinelQueryIterator &) const
            {
                return !m_iter.Finished();
            }

            RawQuery::QueryIterator &GetRawIterator()
            {
                return m_iter;
            }

            [[nodiscard]]
            std::size_t Size() const
            {
                return m_iter.Size();
            }

        private:
            friend struct SentinelQueryIterator;

            template <std::size_t... I>
            [[nodiscard]]
            ComponentTuple GetComponents(std::index_sequence<I...>)
            {
                return std::make_tuple(std::span<std::remove_reference_t<Components>>(
                    static_cast<std::remove_reference_t<Components> *>(
                        m_iter.GetComponentAt(I, sizeof(std::remove_reference_t<Components>))),
                    m_iter.Size())...);
            }

            RawQuery::QueryIterator m_iter;
            bool m_hasData = false;
        };

        [[nodiscard]]
        QueryIterator begin()
        {
            auto queryIter = m_rawQuery.GetIterator();
            // Force the first iteration to get the first entity.
            auto wrappedIter = QueryIterator(std::move(queryIter));
            ++wrappedIter;
            return wrappedIter;
        }

        SentinelQueryIterator end()
        {
            return SentinelQueryIterator{};
        }

        [[nodiscard]]
        QueryIterator begin() const
        {
            return QueryIterator(*this, 0);
        }

        SentinelQueryIterator end() const
        {
            return SentinelQueryIterator{};
        }

        template <typename Func>
            requires std::is_invocable_v<Func, std::add_lvalue_reference_t<Components>...>
        void Each(Func &&func)
        {
            auto it = begin();
            for (; it != end(); ++it)
            {
                const std::size_t size = it.Size();
                ComponentTuple components = *it;

                for (std::size_t i = 0; i < size; ++i)
                {
                    // Get the i-th component of each span. To do this, we convert it to a tuple of references and then
                    // apply it.
                    auto component = std::apply(
                        [&i](auto &&...components) {
                            return std::tie<std::add_lvalue_reference_t<Components>...>(components[i]...);
                        },
                        components);

                    std::apply(func, component);
                }
            }
        }

        template <typename Func>
            requires std::is_invocable_v<Func, EntityId, std::add_lvalue_reference_t<Components>...>
        void Each(Func &&func)
        {
            auto it = begin();
            for (; it != end(); ++it)
            {
                const std::size_t size = it.Size();
                ComponentTuple components = *it;

                for (std::size_t i = 0; i < size; ++i)
                {
                    // Get the i-th component of each span. To do this, we convert it to a tuple of references and then
                    // apply it.
                    auto entityId = it.GetRawIterator().GetEntityAt(i);
                    auto component = std::apply(
                        [&i](auto &&...components) {
                            return std::tie<std::add_lvalue_reference_t<Components>...>(components[i]...);
                        },
                        components);

                    auto finalTuple = std::tuple_cat(std::make_tuple(entityId), component);
                    std::apply(func, finalTuple);
                }
            }
        }

        template <typename Func>
            requires std::is_invocable_v<Func, Entity &, std::add_lvalue_reference_t<Components>...>
        void Each(Func &&func)
        {
            auto it = begin();
            for (; it != end(); ++it)
            {
                const std::size_t size = it.Size();
                ComponentTuple components = *it;

                for (std::size_t i = 0; i < size; ++i)
                {
                    // Get the i-th component of each span. To do this, we convert it to a tuple of references and then
                    // apply it.
                    auto entityId = it.GetRawIterator().GetEntityAt(i);
                    auto component = std::apply(
                        [&i](auto &&...components) {
                            return std::tie<std::add_lvalue_reference_t<Components>...>(components[i]...);
                        },
                        components);

                    auto finalTuple =
                        std::tuple_cat(std::make_tuple(Entity(m_rawQuery.GetScene(), entityId)), component);
                    std::apply(func, finalTuple);
                }
            }
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
    };

} // namespace Hush