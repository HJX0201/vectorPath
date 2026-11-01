#pragma once

#include <QColor>
#include <QString>
#include <cstdint>
#include <vector>

namespace Vp
{

using VpLayerId = std::uint64_t;

// 图层属性记录：颜色、线宽、可见性与锁定状态。
struct VpLayerRecord
{
    VpLayerId id = 0;
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

// 图层状态快照，用于保存和恢复图层配置。
struct VpLayerStateRecord
{
    QString name;
    std::vector<VpLayerRecord> layers;
    QString current_layer_name;
};

} // namespace Vp
