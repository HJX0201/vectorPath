#pragma once

#include <QColor>
#include <QString>
#include <cstdint>
#include <vector>

namespace smartCam
{

using SLayerId = std::uint64_t;

struct SLayerRecord
{
    SLayerId id = 0;
    QString name;
    QColor color{220, 228, 238};
    double line_width_mm = 0.25;
    bool is_visible = true;
    bool is_locked = false;
    bool is_plottable = true;
    bool is_frozen = false;
    QString line_type = QStringLiteral("Continuous");
    int transparency = 0;
};

struct SLayerStateRecord
{
    QString name;
    std::vector<SLayerRecord> layers;
    QString current_layer_name;
};

} // namespace smartCam
