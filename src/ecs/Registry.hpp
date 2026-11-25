#pragma once
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include <SigmaSix/ecs/Entity.hpp>

// Registry ECS minimal : crée des entités et stocke des composants par type.
class Registry {
public:
    Entity create() { return m_next++; }

    void destroy(Entity e) {
        for (auto& [_, pool] : m_pools) pool->remove(e);
    }

    template <typename T, typename... Args>
    T& add(Entity e, Args&&... args) {
        return pool<T>().data[e] = T{std::forward<Args>(args)...};
    }

    template <typename T>
    bool has(Entity e) { return pool<T>().data.count(e) > 0; }

    template <typename T>
    T& get(Entity e) { return pool<T>().data.at(e); }

    // Appelle fn(Entity, T&) pour chaque entité possédant T.
    template <typename T, typename Fn>
    void each(Fn&& fn) {
        for (auto& [e, c] : pool<T>().data) fn(e, c);
    }

private:
    struct IPool {
        virtual ~IPool() = default;
        virtual void remove(Entity) = 0;
    };
    template <typename T>
    struct Pool : IPool {
        std::unordered_map<Entity, T> data;
        void remove(Entity e) override { data.erase(e); }
    };

    template <typename T>
    Pool<T>& pool() {
        auto& p = m_pools[std::type_index(typeid(T))];
        if (!p) p = std::make_unique<Pool<T>>();
        return static_cast<Pool<T>&>(*p);
    }

    Entity m_next = 1;
    std::unordered_map<std::type_index, std::unique_ptr<IPool>> m_pools;
};
