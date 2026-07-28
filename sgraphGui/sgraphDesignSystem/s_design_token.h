#pragma once

#include <QColor>

namespace smartGraphics
{

struct SDesignToken
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
    int small_spacing = 4;
    int spacing = 8;
    int corner_radius = 5;
    int animation_duration = 140;
};

enum class SThemeMode
{
    Light,
    Dark,
    HighContrast
};

} // namespace smartGraphics
