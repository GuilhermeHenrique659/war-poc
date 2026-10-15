#include "model/ecs/Registry.hpp"

#include <gtest/gtest.h>

namespace {

struct Health {
    int value;
};

struct Name {
    const char* value;
};

} // namespace

TEST(Registry, RemovesEveryComponentWhenAnEntityIsDestroyed)
{
    // Given an entity with components from different pools.
    model::ecs::Registry registry;
    const model::ecs::Entity entity = registry.create();
    registry.emplace<Health>(entity, 100);
    registry.emplace<Name>(entity, "Scout");

    // When the entity is destroyed.
    registry.destroy(entity);

    // Then it is no longer alive and none of its components remain.
    EXPECT_FALSE(registry.isAlive(entity));
    EXPECT_EQ(registry.tryGet<Health>(entity), nullptr);
    EXPECT_EQ(registry.tryGet<Name>(entity), nullptr);
}
