#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/build/BuildingSystem.hpp"
#include "model/combat/WeaponComponents.hpp"
#include "model/units/GenealogySystem.hpp"
#include "model/selection/SelectionSystem.hpp"
#include "model/units/UnitSystem.hpp"
#include "model/build/BuildingComponents.hpp"
#include "model/components/Owner.hpp"
#include "model/economy/InventoryComponents.hpp"
#include "model/economy/InventorySystem.hpp"
#include "model/economy/Item.hpp"
#include "model/player/PlayerComponents.hpp"
#include "model/units/UnitComponents.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace {

class GenealogySystemTest : public ::testing::Test {
protected:
    GenealogySystemTest()
        : buildings_(bus_, world_)
        , units_(bus_, world_, buildings_)
        , selection_(bus_, world_)
        , genealogy_(world_, units_, selection_)
    {
        bus_.subscribe<events::UnitSpawned>([this](const events::UnitSpawned& event) {
            spawns_.push_back(event);
        });
        const auto playerEntity = world_.create();
        world_.emplace<model::components::PlayerProfile>(playerEntity, kPlayer, "Tester");
        world_.emplace<model::components::Owner>(playerEntity, kPlayer);
        auto& inventory = world_.emplace<model::components::Inventory>(playerEntity, std::size_t{32});
        for (const auto item : {model::ItemId::Iron, model::ItemId::Coal, model::ItemId::Steel,
                 model::ItemId::Copper, model::ItemId::Electronics, model::ItemId::Tools}) {
            (void)model::InventorySystem::add(inventory, item, 100);
        }
    }

    model::ecs::Entity buildCruzador()
    {
        buildings_.select(kPlayer, model::BuildingType::CommandCenter);
        buildings_.place(kPlayer, {0, 0});
        buildings_.select(kPlayer, model::BuildingType::Cruzador);
        buildings_.place(kPlayer, {12, 12});
        model::ecs::Entity cruzador{};
        world_.each<model::components::Building>([&](model::ecs::Entity entity,
                                                     const model::components::Building& building) {
            if (building.type == model::BuildingType::Cruzador) {
                cruzador = entity;
            }
        });
        return cruzador;
    }

    core::EventBus bus_;
    model::ecs::Registry world_;
    model::BuildingSystem buildings_;
    model::UnitSystem units_;
    model::SelectionSystem selection_;
    model::GenealogySystem genealogy_;
    std::vector<events::UnitSpawned> spawns_;
    static constexpr model::PlayerId kPlayer = 1;
};

} // namespace

TEST_F(GenealogySystemTest, SendsExactlyTwoFriendlyUnitsToCruzadorAndUsesLegacyFallback)
{
    // Given a Cruzador and two nearby units of different types.
    const model::ecs::Entity cruzador = buildCruzador();
    const model::ecs::Entity soldier = units_.spawn(kPlayer, model::recruitStats(model::UnitType::Soldier), {5, 10});
    const model::ecs::Entity bat = units_.spawn(kPlayer, model::recruitStats(model::UnitType::Bat), {5, 11});
    const std::vector<model::ecs::Entity> parents{soldier, bat};

    // When the pair is sent and reaches the Cruzador.
    ASSERT_TRUE(genealogy_.send(kPlayer, cruzador, parents));
    for (int step = 0; step < 100 && world_.isAlive(soldier); ++step) {
        units_.update(0.25F);
        genealogy_.update();
    }

    // Then both parents are replaced by the legacy combined unit with inherited aggregate stats.
    EXPECT_FALSE(world_.isAlive(soldier));
    EXPECT_FALSE(world_.isAlive(bat));
    ASSERT_EQ(spawns_.size(), 3U);
    const auto* child = world_.tryGet<model::components::Unit>(spawns_.back().entity);
    const auto* health = world_.tryGet<model::components::Health>(spawns_.back().entity);
    const auto* combat = world_.tryGet<model::components::Weapon>(spawns_.back().entity);
    ASSERT_NE(child, nullptr);
    ASSERT_NE(health, nullptr);
    ASSERT_NE(combat, nullptr);
    EXPECT_EQ(child->type, model::UnitType::Combined);
    EXPECT_EQ(child->baseCount, 2);
    EXPECT_EQ(health->max, 160);
    EXPECT_EQ(combat->damage, 18);
    EXPECT_FLOAT_EQ(combat->range, 5.0F);
    EXPECT_EQ(combat->type, model::WeaponType::MarksmanRifle);
}

TEST_F(GenealogySystemTest, RejectsSendingAnythingOtherThanExactlyTwoUnits)
{
    // Given a Cruzador and three units owned by the player.
    const model::ecs::Entity cruzador = buildCruzador();
    const model::ecs::Entity first = units_.spawn(kPlayer, model::recruitStats(model::UnitType::Soldier), {5, 10});
    const model::ecs::Entity second = units_.spawn(kPlayer, model::recruitStats(model::UnitType::Soldier), {5, 11});
    const model::ecs::Entity third = units_.spawn(kPlayer, model::recruitStats(model::UnitType::Soldier), {5, 12});

    // When the three units are sent together.
    const bool accepted = genealogy_.send(kPlayer, cruzador, {first, second, third});

    // Then the order is rejected without changing the units.
    EXPECT_FALSE(accepted);
    EXPECT_EQ(spawns_.size(), 3U);
    EXPECT_TRUE(world_.isAlive(first));
    EXPECT_TRUE(world_.isAlive(second));
    EXPECT_TRUE(world_.isAlive(third));
}

TEST_F(GenealogySystemTest, CancelsTheCrossingWhenTheUnitsReceiveANewMoveOrder)
{
    // Given two units that have been sent to the Cruzador.
    const model::ecs::Entity cruzador = buildCruzador();
    const model::ecs::Entity first = units_.spawn(kPlayer, model::recruitStats(model::UnitType::Soldier), {5, 10});
    const model::ecs::Entity second = units_.spawn(kPlayer, model::recruitStats(model::UnitType::Bat), {5, 11});
    const std::vector<model::ecs::Entity> parents{first, second};
    ASSERT_TRUE(genealogy_.send(kPlayer, cruzador, parents));

    // When the player redirects those units elsewhere.
    units_.orderMove(kPlayer, parents, {0, 20});
    genealogy_.update();

    // Then the pending crossing is cancelled and neither unit is consumed.
    EXPECT_TRUE(world_.isAlive(first));
    EXPECT_TRUE(world_.isAlive(second));
    EXPECT_EQ(spawns_.size(), 2U);
}
