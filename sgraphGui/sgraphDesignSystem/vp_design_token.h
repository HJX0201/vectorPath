#pragma once

#include <QColor>

namespace Vp
{

// 主题设计令牌，集中定义界面各处的颜色。
struct VpDesignToken
{
    QColor window;
    QColor surface;
    QColor elevated_surface;
    QColor border;
    QColor text_primary;
    QColor text_secondary;
    QColor accent;
    QColor accent_hover;
    QColor danger;
    QColor canvas;
};

enum class VpThemeMode
{
    Light,
    Dark,
    HighContrast
};

} // namespace Vp
