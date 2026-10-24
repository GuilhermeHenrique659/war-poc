#include "view/world/ShotEffectViewModel.hpp"

namespace view::gameplay {

void ShotEffectViewModel::draw() const
{
    for (const Tracer& tracer : tracers_) {
        const float alpha = alphaFor(tracer.age);
        if (alpha <= 0.0F) {
            continue;
        }
        const Color color{255, 230, 140, static_cast<unsigned char>(255.0F * alpha)};
        DrawLineEx(tracer.from, tracer.to, 2.0F, color);
        DrawCircleV(tracer.from, 3.0F, color);
    }
}

} // namespace view::gameplay
