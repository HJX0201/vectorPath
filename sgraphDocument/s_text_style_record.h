#pragma once

#include <QString>

namespace smartCam
{

struct STextStyleRecord
{
    QString name = QStringLiteral("Standard");
    QString font_family = QStringLiteral("Segoe UI");
    double fixed_height = 0.0;
    double width_factor = 1.0;
    double oblique_angle = 0.0;
    bool is_bold = false;
    bool is_italic = false;
};

} // namespace smartCam
