#pragma once

namespace view::gameplay {

class WorldObjectViewModel {
public:
    virtual ~WorldObjectViewModel() = default;

    virtual void drawAt(float gridX, float gridY) const = 0;
};

} // namespace view::gameplay
