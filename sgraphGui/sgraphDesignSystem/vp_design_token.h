#pragma once

#include <QColor>

namespace Vp
{

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
