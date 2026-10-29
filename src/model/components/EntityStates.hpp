#pragma once

namespace model {

// What a unit is doing in the match. Independent of the automatic resupply behaviour.
enum class UnitState { Idle, Moving, Attacking };

// What the player avatar is doing. Independent of the input that moved it.
enum class PlayerState { Idle, Moving, Attacking };

// Lifecycle of a building entity.
enum class BuildingState { Idle, UnderConstruction, Producing };

} // namespace model
