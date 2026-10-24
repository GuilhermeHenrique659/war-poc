#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "view/world/ShotEffectViewModel.hpp"

#include <gtest/gtest.h>

namespace {

constexpr float kEpsilon = 0.0001F;

} // namespace

TEST(ShotEffectViewModelTests, ShotFiredCreatesATracerFromShooterToTarget)
{
    // Given a view model listening to the model.
    core::EventBus bus;
    view::gameplay::ShotEffectViewModel effects{bus};

    // When a direct shot is announced with continuous grid coordinates.
    bus.publish(events::ShotFired{1, 2, 1, model::WeaponType::Rifle, 0.5F, 0.5F, 2.5F, 0.5F});

    // Then one tracer spans the projected ground points, raised to shooting height.
    ASSERT_EQ(effects.tracers().size(), 1U);
    const view::gameplay::ShotEffectViewModel::Tracer& tracer = effects.tracers().front();
    EXPECT_FLOAT_EQ(tracer.from.x, 0.0F);
    EXPECT_FLOAT_EQ(tracer.from.y, 32.0F - view::gameplay::ShotEffectViewModel::kHeight);
    EXPECT_FLOAT_EQ(tracer.to.x, 128.0F);
    EXPECT_FLOAT_EQ(tracer.to.y, 96.0F - view::gameplay::ShotEffectViewModel::kHeight);
    EXPECT_FLOAT_EQ(tracer.age, 0.0F);
}

TEST(ShotEffectViewModelTests, TracerFadesAndIsRemovedAfterItsDuration)
{
    // Given an active tracer.
    core::EventBus bus;
    view::gameplay::ShotEffectViewModel effects{bus};
    bus.publish(events::ShotFired{1, 2, 1, model::WeaponType::Rifle, 0.5F, 0.5F, 2.5F, 0.5F});

    // When half of its lifetime passes.
    effects.update(view::gameplay::ShotEffectViewModel::kDuration / 2.0F);

    // Then it is still drawn, fading out.
    ASSERT_EQ(effects.tracers().size(), 1U);
    EXPECT_NEAR(view::gameplay::ShotEffectViewModel::alphaFor(effects.tracers().front().age), 0.5F, kEpsilon);

    // When the rest of the lifetime passes.
    effects.update(view::gameplay::ShotEffectViewModel::kDuration);

    // Then it is gone.
    EXPECT_TRUE(effects.tracers().empty());
}

TEST(ShotEffectViewModelTests, WorldResetClearsActiveTracers)
{
    // Given a tracer in flight.
    core::EventBus bus;
    view::gameplay::ShotEffectViewModel effects{bus};
    bus.publish(events::ShotFired{1, 2, 1, model::WeaponType::Rifle, 0.5F, 0.5F, 2.5F, 0.5F});
    ASSERT_EQ(effects.tracers().size(), 1U);

    // When the match world is reset.
    bus.publish(events::WorldReset{});

    // Then no tracer survives into the next match.
    EXPECT_TRUE(effects.tracers().empty());
}
