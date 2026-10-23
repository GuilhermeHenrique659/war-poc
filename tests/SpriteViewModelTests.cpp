#include "raylib_win.h"
#include "view/AnimatedSpriteViewModel.hpp"
#include "view/StaticSpriteViewModel.hpp"
#include "view/TextureManager.hpp"
#include "view/build/BarracksViewModel.hpp"
#include "view/build/CommandCenterViewModel.hpp"
#include "view/build/FactoryViewModel.hpp"
#include "view/build/MineResourceViewModel.hpp"
#include "view/build/SteelworksViewModel.hpp"
#include "view/build/WarehouseViewModel.hpp"
#include "view/units/DroneViewModel.hpp"
#include "model/build/BuildingType.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

double currentTime = 0.0;
Rectangle drawnSource{};
Rectangle drawnDestination{};
Color drawnTint{};
unsigned int drawnTextureId = 0;

std::filesystem::path executableDirectory()
{
    std::vector<wchar_t> path(260);
    const DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    return std::filesystem::path(std::wstring(path.data(), length)).parent_path();
}

} // namespace

double GetTime()
{
    return currentTime;
}

void DrawTexturePro(Texture2D texture, Rectangle source, Rectangle destination, Vector2, float, Color tint)
{
    drawnTextureId = texture.id;
    drawnSource = source;
    drawnDestination = destination;
    drawnTint = tint;
}

namespace view {
namespace {

class SpriteViewModelTests : public ::testing::Test {
protected:
    void SetUp() override
    {
        TextureManager::instance().clear();
        currentTime = 0.0;
        drawnSource = {};
        drawnDestination = {};
        drawnTint = {};
        drawnTextureId = 0;
        texturePath_ = executableDirectory() / "sprite_view_model_test.png";
        std::ofstream file(texturePath_, std::ios::binary);
        file << "sprite view model test";
    }

    void TearDown() override
    {
        TextureManager::instance().clear();
        std::filesystem::remove(texturePath_);
    }

    std::filesystem::path texturePath_;
};

TEST_F(SpriteViewModelTests, AnimatedSpriteUsesConfiguredFrameSizeAndFramesPerSecond)
{
    const SpriteSheet sheet{"test/animated", texturePath_.filename().string(), 8, Vector2{443.5F, 443.5F}, 4.0F};
    const AnimatedSpriteViewModel sprite{sheet};
    constexpr Rectangle destination{10.0F, 20.0F, 128.0F, 64.0F};
    constexpr Color tint{255, 255, 255, 150};
    currentTime = 1.25;

    sprite.drawAt(destination, tint);

    EXPECT_EQ(drawnSource.x, 444.0F);
    EXPECT_EQ(drawnSource.y, 444.0F);
    EXPECT_EQ(drawnSource.width, 443.0F);
    EXPECT_EQ(drawnSource.height, 443.0F);
    EXPECT_EQ(drawnDestination.x, destination.x);
    EXPECT_EQ(drawnDestination.y, destination.y);
    EXPECT_EQ(drawnDestination.width, destination.width);
    EXPECT_EQ(drawnDestination.height, destination.height);
    EXPECT_EQ(drawnTint.a, tint.a);
}

TEST_F(SpriteViewModelTests, StaticSpriteDrawsConfiguredSource)
{
    const StaticSpriteViewModel sprite{
        "test/static", texturePath_.filename().string(), Rectangle{20.0F, 30.0F, 80.0F, 60.0F}, 80, 60
    };
    constexpr Rectangle destination{10.0F, 20.0F, 128.0F, 64.0F};

    sprite.drawAt(destination);

    EXPECT_EQ(drawnSource.x, 0.0F);
    EXPECT_EQ(drawnSource.y, 0.0F);
    EXPECT_EQ(drawnSource.width, 80.0F);
    EXPECT_EQ(drawnSource.height, 60.0F);
    EXPECT_EQ(drawnDestination.x, destination.x);
    EXPECT_EQ(drawnDestination.y, destination.y);
}

TEST_F(SpriteViewModelTests, AnimatedSpriteRejectsInvalidSheetConfiguration)
{
    EXPECT_THROW(SpriteSheet("test/empty", texturePath_.filename().string(), 0, Vector2{443.5F, 443.5F}, 4.0F),
        std::invalid_argument);
    EXPECT_THROW(SpriteSheet("test/invalid_fps", texturePath_.filename().string(), 8, Vector2{443.5F, 443.5F}, 0.0F),
        std::invalid_argument);
    EXPECT_THROW(SpriteSheet("test/overflow", texturePath_.filename().string(), 9, Vector2{443.5F, 443.5F}, 4.0F),
        std::invalid_argument);
}

TEST_F(SpriteViewModelTests, MineResourceViewModelDrawsIronCoalAndCopperAnimations)
{
    const gameplay::MineResourceViewModel mines;
    currentTime = 0.375;
    std::array<unsigned int, 3> mineTextureIds{};

    const std::array mineTypes{
        model::BuildingType::IronMine, model::BuildingType::CoalMine, model::BuildingType::CopperMine
    };
    for (std::size_t index = 0; index < mineTypes.size(); ++index) {
        const model::BuildingType type = mineTypes[index];
        EXPECT_TRUE(gameplay::MineResourceViewModel::handles(type));
        mines.drawAt(type, 2.0F, 4.0F, 2, 150);
        mineTextureIds[index] = drawnTextureId;

        EXPECT_EQ(drawnSource.x, 1331.0F);
        EXPECT_EQ(drawnSource.y, 0.0F);
        EXPECT_EQ(drawnSource.width, 443.0F);
        EXPECT_EQ(drawnSource.height, 444.0F);
        EXPECT_EQ(drawnDestination.x, -256.0F);
        EXPECT_EQ(drawnDestination.y, 64.0F);
        EXPECT_EQ(drawnDestination.width, 256.0F);
        EXPECT_EQ(drawnDestination.height, 256.0F);
        EXPECT_EQ(drawnTint.a, 150);
    }
    EXPECT_NE(mineTextureIds[0], mineTextureIds[1]);
    EXPECT_NE(mineTextureIds[1], mineTextureIds[2]);
    EXPECT_NE(mineTextureIds[0], mineTextureIds[2]);
    EXPECT_FALSE(gameplay::MineResourceViewModel::handles(model::BuildingType::Steelworks));
}

TEST_F(SpriteViewModelTests, SteelworksViewModelDrawsSevenByThreeAnimationAtFrameDuration)
{
    constexpr Vector2 frameSize{1774.0F / 7.0F, 887.0F / 3.0F};
    const SpriteSheet sheet{
        "buildings/steelworks_test", texturePath_.filename().string(), 21, frameSize, 8.0};
    const gameplay::SteelworksViewModel steelworks{sheet};
    currentTime = 0.125;

    steelworks.drawAt(2.0F, 4.0F, 3, 150);

    EXPECT_TRUE(gameplay::SteelworksViewModel::handles(model::BuildingType::Steelworks));
    EXPECT_FALSE(gameplay::SteelworksViewModel::handles(model::BuildingType::IronMine));
    EXPECT_EQ(drawnSource.x, 253.0F);
    EXPECT_EQ(drawnSource.y, 0.0F);
    EXPECT_EQ(drawnSource.width, 254.0F);
    EXPECT_EQ(drawnSource.height, 296.0F);
    EXPECT_EQ(drawnDestination.x, -320.0F);
    EXPECT_EQ(drawnDestination.y, 384.0F - 384.0F * frameSize.y / frameSize.x);
    EXPECT_EQ(drawnDestination.width, 384.0F);
    EXPECT_EQ(drawnDestination.height, 384.0F * frameSize.y / frameSize.x);
    EXPECT_EQ(drawnTint.a, 150);

    currentTime = 1.75;
    steelworks.drawAt(2.0F, 4.0F, 3, 150);

    EXPECT_EQ(drawnSource.x, 0.0F);
    EXPECT_EQ(drawnSource.y, 591.0F);
    EXPECT_EQ(drawnSource.width, 253.0F);
    EXPECT_EQ(drawnSource.height, 296.0F);
}

TEST_F(SpriteViewModelTests, CommandCenterViewModelDrawsTextureAtFootprintSouthEdge)
{
    const SpriteSheet sheet{
        "buildings/command_center_test", texturePath_.filename().string(), 1, Vector2{1774.0F, 887.0F}, 1.0};
    const gameplay::CommandCenterViewModel commandCenter{sheet};
    constexpr Color tint{255, 255, 255, 150};

    commandCenter.drawAt(2.0F, 4.0F, 5, tint.a);

    EXPECT_NE(drawnTextureId, 0U);
    EXPECT_EQ(drawnSource.x, 0.0F);
    EXPECT_EQ(drawnSource.y, 0.0F);
    EXPECT_EQ(drawnSource.width, 1774.0F);
    EXPECT_EQ(drawnSource.height, 887.0F);
    EXPECT_EQ(drawnDestination.x, -448.0F);
    EXPECT_EQ(drawnDestination.y, 192.0F);
    EXPECT_EQ(drawnDestination.width, 640.0F);
    EXPECT_EQ(drawnDestination.height, 320.0F);
    EXPECT_EQ(drawnTint.a, tint.a);
}

TEST_F(SpriteViewModelTests, WarehouseViewModelDrawsTextureAtFootprintSouthEdge)
{
    const SpriteSheet sheet{
        "buildings/warehouse_test", texturePath_.filename().string(), 1, Vector2{1774.0F, 887.0F}, 1.0};
    const gameplay::WarehouseViewModel warehouse{sheet};
    constexpr Color tint{255, 255, 255, 150};

    warehouse.drawAt(2.0F, 4.0F, 2, tint.a);

    EXPECT_NE(drawnTextureId, 0U);
    EXPECT_EQ(drawnSource.width, 1774.0F);
    EXPECT_EQ(drawnSource.height, 887.0F);
    EXPECT_EQ(drawnDestination.x, -256.0F);
    EXPECT_EQ(drawnDestination.y, 192.0F);
    EXPECT_EQ(drawnDestination.width, 256.0F);
    EXPECT_EQ(drawnDestination.height, 128.0F);
    EXPECT_EQ(drawnTint.a, tint.a);
}

TEST_F(SpriteViewModelTests, FactoryViewModelDrawsDistinctStaticSpriteForEachFactoryType)
{
    const auto sprite = [this](const char* key) {
        return StaticSpriteViewModel{
            key, texturePath_.filename().string(), Rectangle{0.0F, 0.0F, 1774.0F, 887.0F}, 1774, 887};
    };
    const gameplay::FactoryViewModel factories{std::array{sprite("buildings/electronics_factory_test"),
        sprite("buildings/tool_factory_test"), sprite("buildings/weapon_factory_test"),
        sprite("buildings/ammo_factory_test")}};
    const std::array factoryTypes{model::BuildingType::ElectronicsFactory, model::BuildingType::ToolFactory,
        model::BuildingType::WeaponFactory, model::BuildingType::AmmoFactory};
    std::array<unsigned int, 4> textureIds{};

    for (std::size_t index = 0; index < factoryTypes.size(); ++index) {
        EXPECT_TRUE(gameplay::FactoryViewModel::handles(factoryTypes[index]));
        factories.drawAt(factoryTypes[index], 2.0F, 4.0F, 3, 150);
        textureIds[index] = drawnTextureId;

        EXPECT_EQ(drawnDestination.x, -320.0F);
        EXPECT_EQ(drawnDestination.y, 128.0F);
        EXPECT_EQ(drawnDestination.width, 384.0F);
        EXPECT_EQ(drawnDestination.height, 256.0F);
        EXPECT_EQ(drawnTint.a, 150);
    }

    EXPECT_NE(textureIds[0], textureIds[1]);
    EXPECT_NE(textureIds[1], textureIds[2]);
    EXPECT_NE(textureIds[2], textureIds[3]);
    EXPECT_FALSE(gameplay::FactoryViewModel::handles(model::BuildingType::Barracks));
    EXPECT_FALSE(gameplay::FactoryViewModel::handles(model::BuildingType::Steelworks));
}

TEST_F(SpriteViewModelTests, BarracksViewModelDrawsStaticSpriteAtFootprintSouthEdge)
{
    const gameplay::BarracksViewModel barracks{StaticSpriteViewModel{
        "buildings/barracks_test", texturePath_.filename().string(), Rectangle{0.0F, 0.0F, 1774.0F, 887.0F},
        1774, 887}};

    EXPECT_TRUE(gameplay::BarracksViewModel::handles(model::BuildingType::Barracks));
    EXPECT_FALSE(gameplay::BarracksViewModel::handles(model::BuildingType::WeaponFactory));

    barracks.drawAt(2.0F, 4.0F, 3, 150);

    EXPECT_NE(drawnTextureId, 0U);
    EXPECT_EQ(drawnDestination.x, -320.0F);
    EXPECT_EQ(drawnDestination.y, 128.0F);
    EXPECT_EQ(drawnDestination.width, 384.0F);
    EXPECT_EQ(drawnDestination.height, 256.0F);
    EXPECT_EQ(drawnTint.a, 150);
}

TEST_F(SpriteViewModelTests, DroneViewModelDrawsAnimatedSpriteAboveItsGridPosition)
{
    const SpriteSheet loadedSheet{
        "units/drone_loaded_test", texturePath_.filename().string(), 8, Vector2{443.5F, 443.5F}, 8.0};
    const SpriteSheet unloadedSheet{
        "units/drone_unloaded_test", texturePath_.filename().string(), 8, Vector2{443.5F, 443.5F}, 8.0};
    gameplay::DroneViewModel drone{loadedSheet, unloadedSheet};
    currentTime = 0.375;

    drone.drawAt(2.0F, 4.0F);

    EXPECT_EQ(drawnTextureId, unloadedSheet.texture().id);
    EXPECT_EQ(drawnSource.x, 1331.0F);
    EXPECT_EQ(drawnSource.y, 0.0F);
    EXPECT_EQ(drawnSource.width, 443.0F);
    EXPECT_EQ(drawnSource.height, 444.0F);
    EXPECT_EQ(drawnDestination.x, -192.0F);
    EXPECT_EQ(drawnDestination.y, 64.0F);
    EXPECT_EQ(drawnDestination.width, 128.0F);
    EXPECT_EQ(drawnDestination.height, 128.0F);

    drone.setLoaded(true);
    drone.drawAt(2.0F, 4.0F);

    EXPECT_EQ(drawnTextureId, loadedSheet.texture().id);
}

} // namespace
} // namespace view
