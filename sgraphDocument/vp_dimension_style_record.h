#pragma once

#include <QString>

namespace Vp
{

// 标注样式：箭头尺寸、文字高度与标注精度。
struct VpDimensionStyleRecord
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

} // namespace Vp
