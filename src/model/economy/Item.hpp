#pragma once

#include <string_view>

namespace model {

enum class ItemId : unsigned char { Iron, Coal, Steel, Count };

struct ItemDefinition {
    ItemId id;
    std::string_view name;
    int maxStack;
};

[[nodiscard]] constexpr ItemDefinition itemDefinition(ItemId id)
{
    switch (id) {
    case ItemId::Iron: return {ItemId::Iron, "Ferro", 100};
    case ItemId::Coal: return {ItemId::Coal, "Carvao", 100};
    case ItemId::Steel: return {ItemId::Steel, "Aco", 100};
    case ItemId::Count: break;
    }
    return {ItemId::Count, "?", 0};
}

} // namespace model
