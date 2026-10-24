#pragma once

#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "view/units/UnitLayout.hpp"

#include <raylib.h>

#include <algorithm>
#include <vector>

namespace view::gameplay {

// Short-lived tracer drawn for every direct shot. The model only announces the shot;
// the view owns the effect lifetime and fades it out.
class ShotEffectViewModel {
public:
    struct Tracer {
        Vector2 from;
        Vector2 to;
        float age = 0.0F;
    };

    static constexpr float kDuration = 0.12F;
    static constexpr float kHeight = 20.0F;

    explicit ShotEffectViewModel(core::EventBus& bus)
        : bus_(bus)
    {
        bus_.subscribe<events::WorldReset>([this](const events::WorldReset&) { tracers_.clear(); });
        bus_.subscribe<events::ShotFired>([this](const events::ShotFired& event) {
            const Vector2 from = unit_layout::groundPoint(event.sourceX, event.sourceY);
            const Vector2 to = unit_layout::groundPoint(event.targetX, event.targetY);
            tracers_.push_back(
                Tracer{Vector2{from.x, from.y - kHeight}, Vector2{to.x, to.y - kHeight}, 0.0F});
        });
    }

    void update(float deltaSeconds)
    {
        for (Tracer& tracer : tracers_) {
            tracer.age += deltaSeconds;
        }
        std::erase_if(tracers_, [](const Tracer& tracer) { return tracer.age >= kDuration; });
    }

    void draw() const;

    [[nodiscard]] const std::vector<Tracer>& tracers() const { return tracers_; }

    [[nodiscard]] static float alphaFor(float age)
    {
        return 1.0F - std::clamp(age / kDuration, 0.0F, 1.0F);
    }

private:
    core::EventBus& bus_;
    std::vector<Tracer> tracers_;
};

} // namespace view::gameplay
