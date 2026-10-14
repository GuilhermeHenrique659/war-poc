#pragma once

#include "model/ecs/ComponentPool.hpp"
#include "model/ecs/Entity.hpp"

#include <memory>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace model::ecs {

// Owns entities and their components. Systems query it; it holds no game logic.
class Registry {
public:
    [[nodiscard]] Entity create()
    {
        const Entity entity = nextEntity_++;
        alive_.insert(entity);
        return entity;
    }

    void destroy(Entity entity)
    {
        if (!alive_.erase(entity)) {
            return;
        }
        for (auto& [type, pool] : pools_) {
            pool->remove(entity);
        }
    }

    [[nodiscard]] bool isAlive(Entity entity) const { return alive_.contains(entity); }

    template <typename Component, typename... Args>
    Component& emplace(Entity entity, Args&&... args)
    {
        return pool<Component>().emplace(entity, std::forward<Args>(args)...);
    }

    template <typename Component>
    [[nodiscard]] Component* tryGet(Entity entity)
    {
        return pool<Component>().tryGet(entity);
    }

    template <typename Component>
    [[nodiscard]] bool has(Entity entity)
    {
        return pool<Component>().has(entity);
    }

    template <typename Component>
    void remove(Entity entity)
    {
        pool<Component>().remove(entity);
    }

    template <typename Component, typename Func>
    void each(Func&& func)
    {
        pool<Component>().each(std::forward<Func>(func));
    }

    void clear()
    {
        alive_.clear();
        for (auto& [type, pool] : pools_) {
            pool->clear();
        }
        nextEntity_ = 0;
    }

private:
    template <typename Component>
    ComponentPool<Component>& pool()
    {
        const std::type_index type(typeid(Component));
        auto it = pools_.find(type);
        if (it == pools_.end()) {
            it = pools_.emplace(type, std::make_unique<ComponentPool<Component>>()).first;
        }
        return static_cast<ComponentPool<Component>&>(*it->second);
    }

    Entity nextEntity_ = 0;
    std::unordered_set<Entity> alive_;
    std::unordered_map<std::type_index, std::unique_ptr<IComponentPool>> pools_;
};

} // namespace model::ecs
