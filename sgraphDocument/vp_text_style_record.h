#pragma once

#include <QString>

namespace Vp
{

// 文字样式：字体、字高、宽度因子与倾斜角。
struct VpTextStyleRecord
{
    QString name = QStringLiteral("Standard");
    QString font_family = QStringLiteral("Segoe UI");
    double fixed_height = 0.0;
    double width_factor = 1.0;
    double oblique_angle = 0.0;
    bool is_bold = false;
    bool is_italic = false;
};

} // namespace Vp
