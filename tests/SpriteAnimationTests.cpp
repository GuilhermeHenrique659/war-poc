#include "view/SpriteAnimation.hpp"

#include <gtest/gtest.h>

namespace {

double currentTime = 0.0;
Rectangle drawnSource{};
Rectangle drawnDestination{};

} // namespace

double GetTime()
{
    return currentTime;
}

void DrawTexturePro(Texture2D, Rectangle source, Rectangle destination, Vector2, float, Color)
{
    drawnSource = source;
    drawnDestination = destination;
}

namespace view {
namespace {

class SpriteAnimationTests : public ::testing::Test {
protected:
    void SetUp() override
    {
        currentTime = 0.0;
        drawnSource = {};
        drawnDestination = {};
    }
};

TEST_F(SpriteAnimationTests, DrawsTheFirstFrameAtTheStart)
{
    const SpriteAnimation animation{Texture2D{1, 400, 200, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8}, 4, 2, 0.25F};
    const Rectangle destination{10.0F, 20.0F, 128.0F, 64.0F};

    animation.drawAt(destination);

    EXPECT_EQ(drawnSource.x, 0.0F);
    EXPECT_EQ(drawnSource.y, 0.0F);
    EXPECT_EQ(drawnSource.width, 100.0F);
    EXPECT_EQ(drawnSource.height, 100.0F);
    EXPECT_EQ(drawnDestination.x, destination.x);
    EXPECT_EQ(drawnDestination.y, destination.y);
    EXPECT_EQ(drawnDestination.width, destination.width);
    EXPECT_EQ(drawnDestination.height, destination.height);
}

TEST_F(SpriteAnimationTests, AdvancesThroughRowsAndLoops)
{
    const SpriteAnimation animation{Texture2D{1, 400, 200, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8}, 4, 2, 0.25F};

    currentTime = 1.25;
    animation.drawAt(Rectangle{});

    EXPECT_EQ(drawnSource.x, 100.0F);
    EXPECT_EQ(drawnSource.y, 100.0F);

    currentTime = 2.0;
    animation.drawAt(Rectangle{});

    EXPECT_EQ(drawnSource.x, 0.0F);
    EXPECT_EQ(drawnSource.y, 0.0F);
}

TEST_F(SpriteAnimationTests, RejectsInvalidAnimationConfiguration)
{
    const Texture2D texture{1, 400, 200, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};

    EXPECT_THROW(SpriteAnimation(texture, 0, 2, 0.25F), std::invalid_argument);
    EXPECT_THROW(SpriteAnimation(texture, 4, 2, 0.0F), std::invalid_argument);
    EXPECT_THROW(SpriteAnimation(Texture2D{}, 4, 2, 0.25F), std::invalid_argument);
}

} // namespace
} // namespace view
