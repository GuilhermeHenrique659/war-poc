#include "view/build/BuildingLayout.hpp"

#include <gtest/gtest.h>

namespace view::build_layout {
namespace {

TEST(BuildingLayoutTests, DummyIsClickableOnTheGroundAndOnItsRaisedSprite)
{
    // Given a 1x1 Training Dummy at tile (5, 5).
    const model::GridPos origin{5, 5};
    const Vector2 ground = iso::gridToWorld(5.5F, 5.5F);
    const Vector2 headTop{ground.x, ground.y - 38.0F};

    // Then both the footprint and the top of the post hit the dummy.
    EXPECT_TRUE(contains(model::BuildingType::TrainingDummy, origin, 1, ground));
    EXPECT_TRUE(contains(model::BuildingType::TrainingDummy, origin, 1, headTop));
}

TEST(BuildingLayoutTests, DummyDoesNotCaptureClicksFarFromItsSprite)
{
    // Given a Training Dummy at tile (5, 5).
    const model::GridPos origin{5, 5};
    const Vector2 ground = iso::gridToWorld(5.5F, 5.5F);

    // Then a point beside and above the post misses it.
    EXPECT_FALSE(contains(model::BuildingType::TrainingDummy, origin, 1,
        Vector2{ground.x + 200.0F, ground.y - 38.0F}));
    EXPECT_FALSE(contains(model::BuildingType::TrainingDummy, origin, 1,
        Vector2{ground.x, ground.y - 200.0F}));
}

} // namespace
} // namespace view::build_layout
