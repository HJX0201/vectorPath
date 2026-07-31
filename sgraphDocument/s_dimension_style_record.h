#pragma once

#include <QString>

namespace smartCam
{

struct SDimensionStyleRecord
{
    QString name = QStringLiteral("Standard");
    double text_height = 2.5;
    double arrow_size = 2.5;
    double overall_scale = 1.0;
    double linear_scale = 1.0;
    int linear_precision = 3;
    int angular_precision = 2;
    QString prefix;
    QString suffix;
    bool suppress_trailing_zeros = false;
};

} // namespace smartCam
