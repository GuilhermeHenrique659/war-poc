#pragma once

#include "model/SelectionSystem.hpp"
#include "model/UnitSystem.hpp"
#include "model/ecs/Registry.hpp"

#include <array>
#include <vector>

namespace model {

class GenealogySystem {
public:
    GenealogySystem(ecs::Registry& world, UnitSystem& units, SelectionSystem& selection);

    void reset();
    [[nodiscard]] bool send(PlayerId player, ecs::Entity cruzador, const std::vector<ecs::Entity>& units);
    void update();

private:
    struct PendingPair {
        PlayerId player;
        ecs::Entity cruzador;
        std::vector<ecs::Entity> units;
        std::array<GridPos, 2> destinations;
    };

    [[nodiscard]] bool arrived(const PendingPair& pair) const;
    [[nodiscard]] bool createChild(const PendingPair& pair);

    ecs::Registry& world_;
    UnitSystem& units_;
    SelectionSystem& selection_;
    std::vector<PendingPair> pending_;
};

} // namespace model
