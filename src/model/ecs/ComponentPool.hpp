#pragma once

#include "model/ecs/Entity.hpp"

#include <unordered_map>
#include <utility>

namespace model::ecs {

class IComponentPool {
public:
    virtual ~IComponentPool() = default;
    virtual void remove(Entity entity) = 0;
    virtual void clear() = 0;
};

template <typename Component>
class ComponentPool final : public IComponentPool {
public:
    template <typename... Args>
    Component& emplace(Entity entity, Args&&... args)
    {
        const auto it = components_.insert_or_assign(entity, Component{std::forward<Args>(args)...}).first;
        return it->second;
    }

    [[nodiscard]] Component* tryGet(Entity entity)
    {
        const auto it = components_.find(entity);
        if (it == components_.end()) {
            return nullptr;
        }
        return &it->second;
    }

    [[nodiscard]] bool has(Entity entity) const { return components_.contains(entity); }

    void remove(Entity entity) override { components_.erase(entity); }

    void clear() override { components_.clear(); }

    template <typename Func>
    void each(Func&& func)
    {
        for (auto& [entity, component] : components_) {
            func(entity, component);
        }
    }

private:
    std::unordered_map<Entity, Component> components_;
};

} // namespace model::ecs
