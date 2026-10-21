#pragma once

#include <raylib.h>

#include <cstddef>
#include <string>
#include <vector>

namespace view::ui {

struct TableRow {
    std::string label;
    std::string value;
    Color valueColor{211, 211, 211, 255};
};

struct TableStyle {
    int fontSize = 16;
    float rowHeight = 23.0F;
    Color labelColor{211, 211, 211, 255};
    Color separatorColor{70, 70, 90, 180};
};

class Table {
public:
    explicit Table(TableStyle style = {})
        : style_(style)
    {
    }

    void draw(Rectangle bounds, const std::vector<TableRow>& rows) const
    {
        for (std::size_t index = 0; index < rows.size(); ++index) {
            const TableRow& row = rows[index];
            const int y = static_cast<int>(bounds.y + static_cast<float>(index) * style_.rowHeight);
            DrawText(row.label.c_str(), static_cast<int>(bounds.x), y, style_.fontSize, style_.labelColor);

            const int valueWidth = MeasureText(row.value.c_str(), style_.fontSize);
            const int valueX = static_cast<int>(bounds.x + bounds.width) - valueWidth;
            DrawText(row.value.c_str(), valueX, y, style_.fontSize, row.valueColor);

            if (index + 1 < rows.size()) {
                const float separatorY = bounds.y + static_cast<float>(index + 1) * style_.rowHeight - 3.0F;
                DrawLineEx(Vector2{bounds.x, separatorY}, Vector2{bounds.x + bounds.width, separatorY}, 1.0F,
                    style_.separatorColor);
            }
        }
    }

private:
    TableStyle style_;
};

} // namespace view::ui
